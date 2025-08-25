#ifndef SYNCED_H
#define SYNCED_H	1

#include "context.h"
#include "state.h"

/**
 * 同期完了状態を初期化する
 */
void initSynced(enum STATE prevState, const struct Context *ctx);

/**
 * 同期完了状態のメイン処理
 */
void mainSynced(void);

/**
 * 同期完了状態の後片付け
 */
struct Context *exitSynced(enum STATE nextState);

#endif	// !SYNCED_H
