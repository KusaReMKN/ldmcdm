#include <Arduino.h>
#include <TimerTC3.h>

#include "context.h"
#include "inputs.h"
#include "state.h"

#include "synced.h"

/**
 * システムクロックの同期待ち状態
 *
 * 遷移元の状態
 * 	同期状態（推定クロック周期と最終キャリア検出時刻を受け取る）
 *
 * 遷移先の状態
 * 	強度推定状態
 *
 * 開始時の処理
 * 	同期信号の終端検知タイマの割り込みを設定し、開始する。
 * 	キャリア信号検出時の割り込みを設定する。
 *
 * 動作中の処理
 * 	キャリア信号を検出すると、
 * 		現在のシステム時刻を記録し、
 * 		タイマをキャリア信号に同期させる（再開する）。
 * 	終端検知タイマが満了すると、強度推定状態に遷移する。
 * 		→ 直前のスロットにキャリア信号がなかった（立ち下がらなかった）
 *
 * 終了時の処理
 * 	キャリア信号検出時の割り込みを解除する。
 * 	終端検知タイマの割り込みを停止し、解除する。
 * 	推定クロック周期及び最終スロットにキャリア信号があった場合の時刻を
 * 		遷移先の状態に引き継ぐ。
 */

/** タイマの周期 */
static sysclock_t timerPeriod;
/** 最終キャリア検出時刻 */
static volatile sysclock_t lastCSClock;

/**
 * 終端検知タイマのハンドラ
 */
static void
tcHandler(void)
{
	// 直前のスロットにキャリア検出信号がなければ強度推定状態に遷移する
	if (getSysClock() - lastCSClock > timerPeriod)
		setState(STATE_LEVELING);
}

/**
 * キャリア信号検出時のハンドラ
 */
static void
csHandler(void)
{
	// キャリアセンス信号の時刻を記録する
	lastCSClock = getSysClock();

	// タイマを同期する
	TimerTc3.restart();
}

/**
 * 同期完了状態を初期化する
 */
void
initSynced(enum STATE prevState, const struct Context *ctx)
{
	// 同期信号の終端検知タイマ（やや長めに）を設定する
	timerPeriod = ctx->period;
	lastCSClock = ctx->lastCSClock;
	TimerTc3.initialize(timerPeriod * 9/8);
	TimerTc3.attachInterrupt(tcHandler);
	//TimerTc3.start();	// ← 本当に必要か？

	// キャリア信号検出時の割り込みを設定する
	attachInterrupt(CSINPUT, csHandler, RISING);
}

/**
 * 同期完了状態のメイン処理
 */
void
mainSynced(void)
{
	// なにもしない
	(void)0;
}

/**
 * 同期完了状態の後片付け
 */
struct Context *
exitSynced(enum STATE nextState)
{
	static struct Context ctx;

	// キャリア信号検出時の割り込みを解除する
	detachInterrupt(CSINPUT);

	// 終端検知タイマを停止し、割り込みを解除する
	TimerTc3.stop();
	TimerTc3.detachInterrupt();

	// 推定クロック周期及びキャリア信号があった場合の時刻を書き込む
	ctx.period = timerPeriod;
	ctx.lastCSClock = lastCSClock + timerPeriod;

	ctx.size = sizeof(ctx);
	return &ctx;
}
