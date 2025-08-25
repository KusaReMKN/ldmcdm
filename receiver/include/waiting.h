#ifndef WAITING_H
#define WAITING_H	1

#include "context.h"
#include "state.h"

/**
 * 待ち状態を初期化する
 */
void initWaiting(enum STATE prevState, const struct Context *ctx);

/**
 * 待ち状態のメイン処理
 */
void mainWaiting(void);

/**
 * 待ち状態の後片付け
 */
struct Context *exitWaiting(enum STATE nextState);

#endif	// !WAITING_H
