#include <TimerTCC0.h>

#include "sysclock.h"

/** 実際にカウントアップするタイマの時間（μs）*/
#define DURATION	57

/** システム時刻の実体（μs）*/
static volatile uint64_t sysClock;

/**
 * タイマのハンドラ
 * システム時刻をカウントアップする
 */
static void
tccHandler(void)
{
	sysClock += DURATION;
}

/**
 * システム時刻のカウントを開始する
 */
void
startSysClock(void)
{
	TimerTcc0.initialize(DURATION);
	TimerTcc0.attachInterrupt(tccHandler);
}

/**
 * システム時刻を得る
 */
sysclock_t
getSysClock(void)
{
	return sysClock;
}
