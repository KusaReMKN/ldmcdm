#include <Arduino.h>

#include "context.h"
#include "inputs.h"
#include "state.h"
#include "sysclock.h"

#include "donothing.h"
#include "waiting.h"
#include "syncing.h"
#include "synced.h"
#include "leveling.h"
#include "receiving.h"

static void (* const initState[])(enum STATE, const struct Context *) = {
	initDoNothing,
	initWaiting,
	initSyncing,
	initSynced,
	initLeveling,
	initReceiving,
	initDoNothing,
};

static void (* const mainState[])(void) = {
	mainDoNothing,
	mainWaiting,
	mainSyncing,
	mainSynced,
	mainLeveling,
	mainReceiving,
	mainDoNothing,
};

static struct Context *(* const exitState[])(enum STATE) = {
	exitDoNothing,
	exitWaiting,
	exitSyncing,
	exitSynced,
	exitLeveling,
	exitReceiving,
	exitDoNothing,
};

void
setup(void)
{
	// システム時刻のカウントを開始する
	startSysClock();

	// 入力ピンを設定する
	pinMode(CSINPUT, INPUT);
	pinMode(PDINPUT, INPUT);

	// XXX: デバッグ用クロックピンを設定する
	pinMode(D0, OUTPUT);

	// シリアル通信を設定する
	while (!Serial)
		;
	Serial.begin(115200);

	delay(1000);

	// 最初は待ち状態
	setState(STATE_WAITING);
}

void
loop(void)
{
	/** 前回の状態 */
	static enum STATE prevState = STATE_XXX;
	/** 現在の状態 */
	const enum STATE lastState = getState();

	// 前回の状態と現在の状態とが異なるなら
	if (prevState != lastState) {
		// 前回の状態を片付けてもらう
		struct Context *ctx = NULL;
		if (prevState != STATE_XXX)
			ctx = exitState[prevState](lastState);
		// 現在の状態の初期化を行う
		initState[lastState](prevState, ctx);
		prevState = lastState;
	}
	// 現在の状態の仕事をする
	mainState[lastState]();
}
