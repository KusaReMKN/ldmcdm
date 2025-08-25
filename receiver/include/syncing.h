#ifndef SYNCING_H
#define SYNCING_H	1

#include "context.h"
#include "state.h"

/**
 * 同期状態を初期化する
 */
void initSyncing(enum STATE precState, const struct Context *ctx);

/**
 * 同期状態のメイン処理
 */
void mainSyncing(void);

/**
 * 同期状態の後片付け
 */
struct Context *exitSyncing(enum STATE nextState);

#endif	// !SYNCING_H
