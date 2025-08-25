#ifndef CONTEXT_H
#define CONTEXT_H	1

#include <stddef.h>

#include "sysclock.h"

/**
 * コンテキスト型の基底構造体
 */
struct Context {
	size_t size;
	sysclock_t period;
	sysclock_t lastCSClock;
#define LEVELS	2
	int32_t intensities[LEVELS];
};

#endif	// !CONTEXT_H
