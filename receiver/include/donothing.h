#ifndef DO_NOTHING_H
#define DO_NOTHING_H	1

#include "context.h"
#include "state.h"

/**
 * 何もしない状態を初期化する
 */
void initDoNothing(enum STATE prevState, const struct Context *ctx);

/**
 * 何もしない状態のメイン処理（冷笑）
 */
void mainDoNothing(void);

/**
 * 何もしない状態の後片付け
 */
struct Context *exitDoNothing(enum STATE nextState);

#endif	// !DO_NOTHING_H
