#include <Arduino.h>
#include <TimerTC3.h>

#include "context.h"
#include "inputs.h"
#include "state.h"

#include "waiting.h"

/**
 * 同期信号の待ち状態
 *
 * 遷移元の状態
 * 	全ての状態（気にしない）
 *
 * 遷移先の状態
 * 	クロック同期状態
 *
 * 開始時の処理
 * 	キャリア信号検出時の割り込みを設定する。
 * 	同期開始中断のための時間切れタイマ割り込みを設定する（開始しない）。
 *
 * 動作中の処理
 * 	キャリア信号を検出すると、
 * 		同期開始中断のためのタイマを開始する。
 * 	時間切れタイマが満了すると、
 * 		キャリア信号検出時刻を忘れる。
 * 		→ ノイズを検出しただけだったと思う。
 * 	時間切れタイマの満了前に再びキャリア信号を検出すると、
 * 		同期状態に遷移する。
 *
 * 終了時の処理
 * 	キャリア信号検出時の割り込みを解除する。
 * 	時間切れタイマを停止し、割り込みを解除する。
 * 	推定クロック周期と最終キャリア検出時刻を遷移先の状態に引き継ぐ。
 */

/** 1 回目及び 2 回目のキャリアセンス時刻 */
static volatile sysclock_t lastCSClock, exitCSClock;

/**
 * 時間切れタイマのハンドラ
 */
static void
tcHandler(void)
{
	// 1 回目のキャリア信号検出時刻を忘れる
	lastCSClock = -1;

	// 時間切れタイマを停止する
	TimerTc3.stop();
}

/**
 * キャリア信号検出時のハンドラ
 */
static void
csHandler(void)
{
	// 1 回目のキャリア信号を検出していれば
	if (lastCSClock != (sysclock_t)-1) {
		// 2 回目のキャリア信号検出時刻を憶えて
		exitCSClock = getSysClock();

		// 同期状態へ移行する
		setState(STATE_SYNCING);
		return;
	}

	// 1 回目のキャリア信号検出時刻を記憶する
	lastCSClock = getSysClock();

	// 時間切れタイマを開始する
	TimerTc3.restart();
}

/**
 * 待ち状態を初期化する
 */
void
initWaiting(enum STATE prevState, const struct Context *ctx)
{
	// 1 回目のキャリア信号検出時刻を忘れる
	lastCSClock = -1;

	// キャリア信号検出時の割り込みを設定する
	attachInterrupt(CSINPUT, csHandler, RISING);

	// 時間切れタイマの割り込みを設定する
	TimerTc3.initialize(1000000L);
	TimerTc3.attachInterrupt(tcHandler);
	TimerTc3.stop();	// まだ開始しない
}

/**
 * 待ち状態のメイン処理
 */
void
mainWaiting(void)
{
	// なにもしない
	(void)0;
}

/**
 * 待ち状態の後片付け
 */
struct Context *
exitWaiting(enum STATE nextState)
{
	static struct Context ctx;

	// キャリア信号検出の割り込みを解除する
	detachInterrupt(CSINPUT);

	// 時間切れタイマを停止し、割り込みを解除する
	TimerTc3.stop();
	TimerTc3.detachInterrupt();

	// 推定クロック周期と最終キャリア信号検出時刻を書き込む
	ctx.period = exitCSClock - lastCSClock;
	ctx.lastCSClock = exitCSClock;

	ctx.size = sizeof(ctx);
	return &ctx;
}
