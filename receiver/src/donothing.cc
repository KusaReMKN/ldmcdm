#include <Arduino.h>

#include "context.h"
#include "state.h"

#include "donothing.h"

/**
 * なにもしない状態
 *
 * 遷移元の状態
 * 	全ての状態
 *
 * 遷移先の状態
 * 	なし
 *
 * 開始時の処理
 * 	遷移元の状態から引き継いだコンテキスト情報をシリアル通信にダンプする。
 *
 * 動作中の処理
 * 	なにもしない。
 *
 * 終了時の処理
 * 	なにもしない。そもそも終了しない。
 */

/**
 * 何もしない状態を初期化する
 */
void
initDoNothing(enum STATE prevState, const struct Context *ctx)
{
	// 遷移前の状態を出力
	Serial.printf("From (%d)\r\n", (int)prevState);

	// コンテキスト情報を表示する
	Serial.printf("        period: %lu\r\n", (unsigned long)ctx->period);
	Serial.printf("   lastCSClock: %lu\r\n", (unsigned long)ctx->lastCSClock);
	Serial.printf("intensities[0]: %ld\r\n", (long)ctx->intensities[0]);
	Serial.printf("intensities[1]: %ld\r\n", (long)ctx->intensities[1]);
}

/**
 * 何もしない状態のメイン処理（冷笑）
 */
void
mainDoNothing(void)
{
	(void)0;
}

/**
 * 何もしない状態の後片付け
 */
struct Context *
exitDoNothing(enum STATE nextState)
{
	static struct Context ctx;

	ctx.size = sizeof(ctx);
	return &ctx;
}
