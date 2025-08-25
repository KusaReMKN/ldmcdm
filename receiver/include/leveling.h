#ifndef LEVELING_H
#define LEVELING_H	1

#include "context.h"
#include "state.h"

/**
 * 強度推定状態を初期化する
 */
void initLeveling(enum STATE prevState, const struct Context *ctx);

/**
 * 強度推定状態のメイン処理
 */
void mainLeveling(void);

/**
 * 強度推定状態の後片付け
 */
struct Context *exitLeveling(enum STATE nextState);

#endif	// !LEVELING_H
