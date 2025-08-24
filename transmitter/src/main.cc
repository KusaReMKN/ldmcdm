#include <Arduino.h>
#include <TimerTCC0.h>

/** 端子指定 */
#define LED_L1 D4
#define LED_L2 D5

/** 出力値 */
#define ON	HIGH
#define OFF	LOW

/**
 * 2 bit のかたまりに対応する同強度のチップパターンを得る
 * パターンは Layer 1 用であるから、Layer 2 の場合は b2 を反転してから入力する
 */
static uint16_t
bit2ToChips(uint8_t b2)
{
#define PATTERN(b00, b01, b02, b03, b10, b11, b12, b13, \
			b20, b21, b22, b23, b30, b31, b32, b33) \
		(0B ## b33 ## b32 ## b31 ## b30 ## b23 ## b22 ## b21 ## b20 \
			## b13 ## b12 ## b11 ## b10 ## b03 ## b02 ## b01 ## b00)
	/**
	 * チップパターン（2 チャネル多重済）
	 *
	 * n-th item | Code 1 | Code 2
	 * ----------|--------|--------
	 *      0    |    0   |    0
	 *      1    |    1   |    0
	 *      2    |    0   |    1
	 *      3    |    1   |    1
	 */
	constexpr uint16_t convTab[] = {
		PATTERN(1, 1, 0, 0,  0, 0, 1, 1,  0, 0, 1, 1,  1, 1, 0, 0),
		PATTERN(0, 1, 1, 0,  1, 0, 0, 1,  0, 1, 1, 0,  1, 0, 0, 1),
		PATTERN(1, 0, 0, 1,  0, 1, 1, 0,  1, 0, 0, 1,  0, 1, 1, 0),
		PATTERN(0, 0, 1, 1,  1, 1, 0, 0,  1, 1, 0, 0,  0, 0, 1, 1),
	};
#undef PATTERN

	return convTab[b2 & 0x03];
}

/**
 * 4 bit のかたまりに対応するチップパターンを得る
 * p[0] が Layer 1 用、p[1] が Layer 2 用
 */
static void
bit4ToChips(uint8_t b4, uint16_t p[2])
{
	p[0] = bit2ToChips( b4 >> 0 & 0x03);
	p[1] = bit2ToChips(~b4 >> 2 & 0x03);
}

/**
 * 文字に対応するチップパターンを得る
 * p[0][0] が最初の Layer 1 用、p[0][1] が最初の Layer 2 用
 * p[1][0] がつぎの Layer 1 用、p[1][1] がつぎの Layer 2 用
 */
static void
charToChips(uint8_t c, uint16_t p[2][2])
{
	bit4ToChips(c >> 0 & 0x0F, p[0]);
	bit4ToChips(c >> 4 & 0x0F, p[1]);
}

/**
 * チップパターンを送信パターンに変換する
 * 0b10100101 → 0b1100110000110011
 */
static uint32_t
chipsToPattern(uint16_t p)
{
	uint32_t r = 0;

	for (size_t i = 0; i < 16; i++)
		r |= (p & (1 << i)) << i;
	r |= r << 1;

	return r;
}

/** 送信バッファ */
#define BUFLEN	1024
static volatile uint32_t buffer[2][1024];
/** バッファの先頭位置及び末尾位置 */
static volatile size_t bufHead = 0, bufTail = 0;
/** バッファのビット位置 */
static volatile size_t bit = 0;
/** 送信中フラグ */
static volatile bool sending = false;

/**
 * プレアンブルパターンを送信する
 */
static void
sendPreamble(void)
{
	/** プレアンブルパターン */
	constexpr uint32_t PREAMBLE = 0x55555555UL;
	constexpr uint32_t PREAMBLE_STOP = 0xD5555555UL;

	// プレアンブルパターンを送信バッファに詰める
	constexpr int nPreamble = 4;
	for (int i = 0; i < nPreamble-1; i++) {
		buffer[0][bufTail] = buffer[1][bufTail] = PREAMBLE;
		bufTail++;
		bufTail %= BUFLEN;
	}
	buffer[0][bufTail] = buffer[1][bufTail] = PREAMBLE_STOP;
	bufTail++;
	bufTail %= BUFLEN;
}

/**
 * 文字を送信する
 */
static void
sendChar(uint8_t c)
{
	uint16_t tmp[2][2];

	charToChips(c, tmp);
	for (size_t i = 0; i < 2; i++) {
		for (size_t j = 0; j < 2; j++)
			buffer[j][bufTail] = chipsToPattern(tmp[i][j]);
		bufTail++;
		bufTail %= BUFLEN;
	}
}

/**
 * レベルチェックパターンを送信する
 */
static void
sendLevelCheck(void)
{
	for (size_t t = 0; t < 3; t++) {
		sendChar(0xFF);
	}
	sendChar(0x00);
}

/**
 * 送信を開始する
 */
static void
sendStart(void)
{
	sending = true;
}

/**
 * 送信処理（タイマのハンドラ関数）
 */
void
tcHandler(void)
{
	// XXX: デバッグ用のクロック出力
	static int CLOCK = 0;
	digitalWrite(D0, CLOCK++ & 2 ? ON : OFF);


	// 送信中でなければ何もしない
	if (!sending)
		return;

	// バッファの末尾にいたら送信終了
	if (bufHead == bufTail) {
		digitalWrite(LED_L1, OFF);
		digitalWrite(LED_L2, OFF);

		sending = false;
		return;
	}

	//* ピンに出力
	digitalWrite(LED_L1, (buffer[0][bufHead] & (1UL << bit)) ? ON : OFF);
	digitalWrite(LED_L2, (buffer[1][bufHead] & (1UL << bit)) ? ON : OFF);

	// 1 bit 分進み、必要に応じて 1 ワード進む
	if (++bit == 32) {
		bit = 0;
		bufHead++;
		bufHead %= BUFLEN;
	}
}

/**
 * シルアル通信から一行読み込む
 */
static char *
getLine(char *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len - 1; i++) {
		while (!Serial.available())
			;
		buf[i] = Serial.read();
		if (buf[i] == '\r')
			break;
	}
	buf[i] = '\0';
	if (Serial.peek() == '\n')
		(void)Serial.read();

	return buf;
}

void
setup(void)
{
	// 出力ピンを設定する
	pinMode(LED_L1, OUTPUT);
	pinMode(LED_L2, OUTPUT);
	digitalWrite(LED_L1, OFF);
	digitalWrite(LED_L2, OFF);

	// XXX: デバッグ用クロックピンを設定する
	pinMode(D0, OUTPUT);

	// シリアル通信を設定する
	while (!Serial)
		;
	Serial.begin(115200);
}

void
loop(void)
{
	// 送信開始前であれば
	if (!sending) {
		// 送信チップレートを受け取る
		unsigned long chipRate;
		char buf[256], *endp;
		do {
			// プロンプトを表示する
			Serial.print('?');

			// 一行受け取って数値に変換
			(void)getLine(buf, sizeof(buf));
			chipRate = strtoul(buf, &endp, 0);
		} while (*endp != '\0' || chipRate == 0);

		Serial.print(chipRate);

		// チップレートをクロック周期に変換してタイマーを開始する
		TimerTcc0.initialize(1.0 / chipRate * 500000L);
		TimerTcc0.attachInterrupt(tcHandler);

		// プレアンブルを準備する
		sendPreamble();
		sendLevelCheck();

		// プロンプトを表示する
		Serial.print('!');

		// 送信を開始する
		sendStart();
	}

	// 受信可能な文字があれば読み込んで送信する
	if (Serial.available())
		sendChar(Serial.read());
}
