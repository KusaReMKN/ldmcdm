#ifndef RECEIVING_H
#define RECEIVING_H	1

#include <stddef.h>

#include "sysclock.h"
#include "state.h"

/**
 * 受信状態を初期化する
 */
void initReceiving(enum STATE precState, const struct Context *ctx);

/**
 * 受信状態のメイン処理
 */
void mainReceiving(void);

/**
 * 受信状態の後片付け
 */
struct Context *exitReceiving(enum STATE nextState);

/**
 * 受信状態からの引き継ぎ情報
 */
struct ReceivingContext {
	size_t size;
	sysclock_t period;
	uint32_t l1amp;
	uint32_t l2amp;
};

#endif	// !RECEIVING_H
