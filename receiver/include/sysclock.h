#ifndef SYSCLOCK_H
#define SYSCLOCK_H	1

#include <stdint.h>

typedef volatile uint64_t sysclock_t;

/**
 * システム時刻のカウントを開始する
 */
void startSysClock(void);

/**
 * システム時刻を得る
 */
sysclock_t getSysClock(void);

#endif	// !SYSCLOCK_H
