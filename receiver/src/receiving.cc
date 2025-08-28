#include <Arduino.h>
#include <TimerTC3.h>

#include "context.h"
#include "inputs.h"
#include "state.h"
#include "sysclock.h"

#include "receiving.h"

/**
 * データの受信状態
 *
 * 遷移元の状態
 * 	強度推定状態
 *
 * 遷移先の状態
 * 	待ち状態
 *
 * 開始時の処理
 * 	チップ輝度バッファ及びデータバッファを巻き戻す。
 * 	推定受信強度を初期化する。
 * 	チップ読み込みタイマの割り込みを設定する。
 * 	キャリア信号検出時の割り込みを設定する。
 *
 * 動作中の処理
 * 	キャリア信号を検出すると、
 * 		キャリア信号の最終検出時刻を更新する。
 * 	チップ読み込みタイマが満了すると、
 * 		測定されたチップ輝度を記録し、
 * 		送信が終了しているか調べ、
 * 			そのようであれば待ち状態へ遷移する。
 * 		周期誤差を補正できそうなら、
 * 			信号が思ったよりも早いなら次のタイマを遅らせ、
 * 			信号が思ったよりも遅いなら次のタイマを早める。
 * 	フレームを構成する全てのチップが記録されていれば、
 * 		復号処理を行い、
 * 		情報信号を復号し、シリアル通信に出力する。
 *
 * 終了時の処理
 * 	チップ読み込みタイマを停止し、割り込みを解除する。
 * 	キャリア信号検出時の割り込みを解除する。
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

static void
tcHandler(void)
{
	// 測定されたチップ輝度を記録する
	pdInputs[bufTail++] = analogRead(PDINPUT);

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
		TimerTc3.setPeriod(timerPeriod * 11/8);
	} else if (diff > timerPeriod * 3/4) {
		// 0.675
		TimerTc3.setPeriod(timerPeriod * 5/8);
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

static uint8_t chbuf[2];
static size_t chTail = 0;

/**
 * 強度推定状態を初期化する
 */
void
initReceiving(enum STATE precState, const struct Context *ctx)
{
	// バッファを巻き戻す
	bufTail = 0;
	chTail = 0;

	// 推定受信強度を格納する
	intensities[0] = ctx->intensities[0] << 7;
	intensities[1] = ctx->intensities[1] << 7;
	nIntensities[0] = nIntensities[1] = 32;

	// チップ読み込みタイマを設定する
	timerPeriod = ctx->period;
	TimerTc3.initialize(timerPeriod);
	TimerTc3.attachInterrupt(tcHandler);
	TimerTc3.start();

	// キャリア信号検出時の割り込みを設定する
	lastCSClock = 0;
	attachInterrupt(CSINPUT, csHandler, RISING);

}

/**
 * 強度推定状態のメイン処理
 */
void
mainReceiving(void)
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

	// 情報信号を復号する
	chbuf[chTail++] = i22 << 3 | i12 << 2 | i21 << 1 | i11 << 0;
	if (chTail == 2) {
		Serial.print((char)(chbuf[0] | chbuf[1] << 4));
		chTail=0;
	}

	// バッファを巻き戻す
	bufTail = 0;
}

/**
 * 強度推定状態の後片付け
 */
struct Context *
exitReceiving(enum STATE nextState)
{
	static struct Context ctx;

	// チップ読み込みタイマを停止し、割り込みを解除する
	TimerTc3.stop();
	TimerTc3.detachInterrupt();

	// キャリア信号検出時の割り込みを解除する
	detachInterrupt(CSINPUT);

	ctx.size = sizeof(ctx);
	return &ctx;
}
