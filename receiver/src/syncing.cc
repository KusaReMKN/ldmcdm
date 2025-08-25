#include <Arduino.h>
#include <TimerTC3.h>

#include "context.h"
#include "inputs.h"
#include "state.h"

#include "syncing.h"

/**
 * システムクロックの同期状態
 *
 * 遷移元の状態
 * 	待ち状態（推定クロック周期と最終キャリア検出時刻を受け取る）
 *
 * 遷移先の状態
 * 	同期完了待ち状態（同期完了時）
 * 	待ち状態（同期失敗時）
 *
 * 開始時の処理
 * 	同期信号の時間切れタイマ割り込みを設定し、開始する。
 * 	キャリア信号検出時刻バッファを巻き戻す。
 * 	キャリア信号検出時の割り込みを設定する。
 *
 * 動作中の処理
 * 	キャリア信号を検出すると、
 * 		時間切れタイマを延命し、
 * 		キャリア検出時刻を記録する。
 * 		所定回数だけ記録したら同期完了状態に遷移する。
 * 	時間切れタイマが満了すると、
 * 		待ち状態に遷移する。
 *
 * 終了時の処理
 * 	キャリア信号検出時の割り込みを解除する。
 * 	時間切れタイマを停止し、割り込みを解除する。
 * 	推定クロック周期及び最終キャリア検出時刻を遷移先の状態に引き継ぐ。
 */

/**
 * 時間切れタイマのハンドラ
 */
static void
tcHandler(void)
{
	// 時間切れなので待ち状態に遷移する
	setState(STATE_WAITING);

	// 時間切れタイマを停止する
	TimerTc3.stop();
}

/** キャリア信号検出時刻用バッファ */
#define CLOCK_BUFLEN	64
static volatile sysclock_t csClocks[CLOCK_BUFLEN];
/** バッファの末尾位置 */
static volatile size_t bufTail;

/**
 * キャリア信号検出時のハンドラ
 */
static void
csHandler(void)
{
	// 時間切れタイマを延命する
	TimerTc3.restart();

	// キャリア信号検出時刻を記録する
	if (bufTail < CLOCK_BUFLEN)
		csClocks[bufTail++] = getSysClock();

	// バッファの末尾まで記録したら同期完了待ち状態に遷移する
	if (bufTail == CLOCK_BUFLEN)
		setState(STATE_SYNCED);
}

/**
 * 同期状態を初期化する
 */
void
initSyncing(enum STATE precState, const struct Context *ctx)
{
	// キャリア信号検出の時間切れタイマを設定する
	TimerTc3.setPeriod(ctx->period * 3/2);
	TimerTc3.attachInterrupt(tcHandler);
	//TimerTc3.start();	// ← 本当に必要か？

	// バッファの末尾位置を先頭まで巻き戻す
	bufTail = 0;

	// キャリア信号検出時の割り込みを設定する
	attachInterrupt(CSINPUT, csHandler, RISING);
}

/**
 * 同期状態のメイン処理
 */
void
mainSyncing(void)
{
	// なにもしない
	(void)0;
}

/**
 * 同期状態の後片付け
 */
struct Context *
exitSyncing(enum STATE nextState)
{
	static struct Context ctx;

	// キャリア信号検出時の割り込みを解除する
	detachInterrupt(CSINPUT);

	// 時間切れタイマを停止し、割り込みを解除する
	TimerTc3.stop();
	TimerTc3.detachInterrupt();

	// 待ち状態に移行するなら何も書き込まずに終了する
	if (nextState == STATE_WAITING) {
		ctx.size = sizeof(ctx);
		return (struct Context *)&ctx;
	}

	// キャリア信号検出時刻から計算される推定クロック周期を書き込む
	ctx.period = 0;
	for (size_t i = 1; i < CLOCK_BUFLEN; i++)
		ctx.period += csClocks[i-0] - csClocks[i-1];
	ctx.period /= CLOCK_BUFLEN - 1;

	// 最終キャリア信号検出時刻を書き込む
	ctx.lastCSClock = csClocks[CLOCK_BUFLEN-1];

	ctx.size = sizeof(ctx);
	return &ctx;
}
