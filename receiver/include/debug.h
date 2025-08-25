#ifndef DEBUG_H
#define DEBUG_H	1

#include "sysclock.h"

#if 0

#define DEBUG_PRINTF(...)	\
	do { \
		Serial.printf("[%20lu] ", (unsigned long)sysClock); \
		Serial.printf("%s(%d): ", __FILE__, __LINE__); \
		Serial.printf(__VA_ARGS__); \
	} while (0)

#else

#define DEBUG_PRINTF(...)	(void)0

#endif

#endif	// !DEBUG_H
