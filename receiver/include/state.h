#ifndef STATE_H
#define STATE_H	1

/**
 * 状態識別子
 */
enum STATE {
	STATE_XXX = -1,
	STATE_DO_NOTHING,
	STATE_WAITING,
	STATE_SYNCING,
	STATE_SYNCED,
	STATE_LEVELING,
	STATE_RECEIVING,
	STATE_MAX,
};

/**
 * 現在の状態を取得する
 */
enum STATE getState(void);

/**
 * 現在の状態を設定する
 */
enum STATE setState(enum STATE state);

#endif	// !STATE_H
