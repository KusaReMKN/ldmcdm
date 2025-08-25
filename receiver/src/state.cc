#include "state.h"

/**
 * 現在の状態
 */
static volatile enum STATE currentState;

/**
 * 現在の状態を取得する
 */
enum STATE
getState(void)
{
	return currentState;
}

/**
 * 現在の状態を設定する
 */
enum STATE
setState(enum STATE state)
{
	return currentState = state;
}
