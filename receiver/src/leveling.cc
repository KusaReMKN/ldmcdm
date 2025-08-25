#include <Arduino.h>
#include <TimerTC3.h>

#include "context.h"
#include "inputs.h"
#include "state.h"
#include "sysclock.h"

#include "leveling.h"

/**
 * 受信信号の強度推定状態
 *
 * 遷移元の状態
 * 	同期待ち状態（推定クロック周期と最終キャリア検出時刻を受け取る）
 *
 * 遷移先の状態
 * 	受信状態
 *
 * メモ
 * 	この状態の開始時点は、強度推定のための疑似信号が開始する 1 スロット前
 * 		そのため、次のスロットからチップを読み込んで復号すればよい。
 *
 * 開始時の処理
 * 	チップ輝度バッファを巻き戻す。
 * 	推定受信強度をすべて忘れる。
 * 	チップ読み込みタイマの割り込みを設定する。
 * 	キャリア信号検出時の割り込みを設定する。
 *
 * 動作中の処理
 *
 * 終了時の処理
 *
 * 入ってきた時刻その瞬間は強度推定用信号を受信していない。
 * 入ってきた時刻の 1 周期後が強度推定用信号の最初のチップになる。
 * そこからデコードしなさい。
 */

/** タイマの周期 */
static sysclock_t timerPeriod;
/** 最終キャリア検出時刻 */
static volatile sysclock_t lastCSClock;

/** チップ輝度用バッファ */
#define INPUT_BUFLEN	16
static volatile int32_t pdInputs[16];
/** バッファの末尾位置 */
static volatile size_t bufTail;

static void dtcHandler(void);

/**
 * チップ読み込みタイマのハンドラ
 */
static void
tcHandler(void)
{
	// 測定されたチップ輝度を記録する
	pdInputs[bufTail++] = (int32_t)analogRead(PDINPUT);

	// XXX: デバッグ用のクロック信号を出力する
	static bool on = false;
	digitalWrite(D0, on = !on);

	// 送信終了してそうならおしまい
	sysclock_t diff = getSysClock() - lastCSClock;
	if (lastCSClock > 0 && diff > 16*timerPeriod)
		setState(STATE_WAITING);

	// 周期誤差を補正できないっぽい
	if (diff > timerPeriod)
		return;

	// 周期誤差補正
	if (diff < timerPeriod * 1/4) {
		TimerTc3.setPeriod(timerPeriod * 11/8);	// 1.375
	} else if (diff > timerPeriod * 3/4) {
		TimerTc3.setPeriod(timerPeriod * 5/8);	// 0.625
	} else {
		return;
	}
	TimerTc3.attachInterrupt(dtcHandler);
	TimerTc3.restart();
}

static void
dtcHandler(void)
{
	TimerTc3.setPeriod(timerPeriod);
	TimerTc3.attachInterrupt(tcHandler);
	TimerTc3.restart();
	tcHandler();
}

static void
csHandler(void)
{
	// キャリア信号の最終検出時刻を記録する
	lastCSClock = getSysClock();
}

/**
 * 長さ n の系列 a と b との相関 Γ(a, b) を求める
 */
static int32_t
gamma(const int32_t *a, const int32_t *b, size_t n)
{
	int32_t s;

	s = 0;
	for (size_t i = 0; i < n; i++)
		s += a[i] * b[i];

	return s;
}

/** 推定受信強度 */
static uint32_t intensities[2];
static size_t nIntensities[2];

/**
 * 強度推定状態を初期化する
 */
void
initLeveling(enum STATE prevState, const struct Context *ctx)
{
	bufTail = 0;

	// 推定強度を忘れる
	intensities[0] = intensities[1] = 0;
	nIntensities[0] = nIntensities[1] = 0;

	// チップ読み込みタイマを設定する
	timerPeriod = ctx->period;
	TimerTc3.initialize(timerPeriod);
	TimerTc3.attachInterrupt(tcHandler);
	TimerTc3.start();

	lastCSClock = 0;
	attachInterrupt(CSINPUT, csHandler, RISING);
}

/**
 * 強度推定状態のメイン処理
 */
void
mainLeveling(void)
{
	// フレームが揃っていないなら何もしない
	if (bufTail != INPUT_BUFLEN)
		return;

	/** 符号語テーブル w[k,l,0] - w[k,l,1] */
	constexpr int32_t decodeTab[2][16] = {
		{ 1, 0,-1, 0, -1, 0, 1, 0,  0,-1, 0, 1,  0, 1, 0,-1 },
		{ 0, 1, 0,-1,  0,-1, 0, 1, -1, 0, 1, 0,  1, 0,-1, 0 },
	};

	// 第 1 層を復号する
	const int32_t y11 = gamma(decodeTab[0], (int32_t *)pdInputs, 16);
	const int32_t y21 = gamma(decodeTab[1], (int32_t *)pdInputs, 16);
	const int i11 = y11 > 0 ? 0 : 1;
	const int i21 = y21 > 0 ? 0 : 1;

	// 第 1 層の推定強度を更新する
	intensities[0] += abs(y11) + abs(y21);
	nIntensities[0] += 2;

	// 第 1 層の信号を差し引く
	for (int i = 0; i < 16; i++) {
		int32_t t = 0;
		t += i11 == 0 ? (decodeTab[0][i] > 0) : (decodeTab[0][i] < 0);
		t += i21 == 0 ? (decodeTab[1][i] > 0) : (decodeTab[1][i] < 0);
		pdInputs[i] -= (intensities[0] / nIntensities[0] / 4) * t;
	}

	// 第 2 層を復号する
	const int32_t y12 = gamma(decodeTab[0], (int32_t *)pdInputs, 16);
	const int32_t y22 = gamma(decodeTab[1], (int32_t *)pdInputs, 16);
	const int i12 = y12 < 0 ? 0 : 1;	// 第 2 層は符号が逆
	const int i22 = y22 < 0 ? 0 : 1;	// 第 2 層は符号が逆

	// 第 2 層の推定強度を更新する
	intensities[1] += abs(y12) + abs(y22);
	nIntensities[1] += 2;

	// 情報信号を復号する（4 bit）
	const int d = i22 << 3 | i12 << 2 | i21 << 1 | i11 << 0;

	// 強度推定を終わっていいか確かめる（簡単のため最後三つだけ見る）
	static int last[3];
	last[0] = last[1];
	last[1] = last[2];
	last[2] = d;
	if (last[0] == 0x0C && last[1] == 0x08 && last[2] == 0x00)
		setState(STATE_RECEIVING);

	// バッファを巻き戻す
	bufTail = 0;
}

/**
 * 強度推定状態の後片付け
 */
struct Context *
exitLeveling(enum STATE nextState)
{
	static struct Context ctx;

	// 時間切れの割り込みをキャンセルする
	TimerTc3.stop();
	TimerTc3.detachInterrupt();

	detachInterrupt(CSINPUT);

	ctx.period = timerPeriod;
	ctx.intensities[0] = intensities[0] / nIntensities[0] / 4;
	ctx.intensities[1] = intensities[1] / nIntensities[1] / 4;

	ctx.size = sizeof(ctx);
	return &ctx;
}
