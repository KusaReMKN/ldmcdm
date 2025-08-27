#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define MIN(a, b)	((a) < (b) ? (a) : (b))

static const size_t popTab[] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

static void
usage(void)
{
	fprintf(stderr, "usage: driver receiver transmitter "
			"random chip-rate nByte\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct termios tos;
	unsigned long chipRate;
	ssize_t cnt, nByte, nRead, nWrite, tmp;
	int rfd, tfd, xfd;
	char *rbuf, *tbuf;

	/* 引数の数が合わなければ死ぬ */
	if (argc != 6)
		usage();

	/* 受信機側を開く */
	rfd = open(argv[1], O_RDONLY | O_NOCTTY);
	if (rfd == -1)
		err(1, "%s", argv[1]);
	/* 端末属性を取得する */
	if (tcgetattr(rfd, &tos) == -1)
		err(1, "%s", argv[1]);
	/* RAW モードにする（XXX: 本来であれば復旧用を用意する） */
	cfmakeraw(&tos);
	/* 適用する（XXX: エラーチェックが雑） */
	if (tcsetattr(rfd, TCSANOW, &tos) == -1)
		err(1, "%s", argv[1]);

	/* 送信機側を開く */
	tfd = open(argv[2], O_WRONLY | O_NOCTTY);
	if (tfd == -1)
		err(1, "%s", argv[2]);
	/* 端末属性を取得する */
	if (tcgetattr(tfd, &tos) == -1)
		err(1, "%s", argv[2]);
	/* RAW モードにする（XXX: 本来であれば復旧用を用意する） */
	cfmakeraw(&tos);
	/* 適用する（XXX: エラーチェックが雑） */
	if (tcsetattr(tfd, TCSANOW, &tos) == -1)
		err(1, "%s", argv[2]);

	/* 乱数デバイスを開く */
	xfd = open(argv[3], O_RDONLY | O_NOCTTY);
	if (xfd == -1)
		err(1, "%s", argv[3]);

	/* チップレートを受けとる */
	chipRate = strtoul(argv[4], NULL, 0);
	if (chipRate < 1)
		errx(1, "chip-rate must be >0");

	/* サンプルバイト数を受け取るl */
	nByte = strtoul(argv[5], NULL, 0);
	if (nByte < 1)
		errx(1, "nByte must be >0");

	/* 受信用バッファを確保する */
	rbuf = malloc(nByte);
	if (rbuf == NULL)
		err(1, "malloc");

	/* 送信用バッファを確保する */
	tbuf = malloc(nByte);
	if (tbuf == NULL)
		err(1, "malloc");

	/* 送信用バッファに乱数を用意する */
	nRead = 0;
	do {
		tmp = read(xfd, tbuf+nRead, nByte-nRead);
		if (tmp == -1)
			err(1, "read: %s", argv[3]);
		nRead += tmp;
	} while (nRead != nByte);

	/* 送信機にチップレートを送り付ける */
	if (dprintf(tfd, "\r%lu\r", chipRate) < 0)
		err(1, "dprintf: %s", argv[2]);
	/* データを少しずつ送り付ける */
	nWrite = 0;
	do {
		tmp = write(tfd, tbuf+nWrite, MIN(16, nByte-nWrite));
		if (tmp == -1)
			err(1, "write: %s:", argv[2]);
		/* デバッグ用に表示しておく */
		for (ssize_t i = nWrite; i < nWrite+tmp; i++) {
			if ((i & 0xF) == 0)
				(void)fprintf(stderr, "\n%06zX:", i);
			(void)fprintf(stderr, " %02x", tbuf[i] & 0xFF);
		}
		nWrite += tmp;
	} while (nWrite != nByte);
	(void)fprintf(stderr, "\n%06zX\n", nWrite);

	/* 受信機からデータを受け取る */
	nRead = 0;
	do {
		tmp = read(rfd, rbuf+nRead, nByte-nRead);
		if (tmp == -1)
			err(1, "read: %s", argv[1]);
		/* デバッグ用の表示しておく */
		for (ssize_t i = nRead; i < nRead+tmp; i++) {
			if ((i & 0xF) == 0)
				(void)fprintf(stderr, "\n%06zX:", i);
			(void)fprintf(stderr, " %02x", rbuf[i] & 0xFF);
		}
		nRead += tmp;
	} while (nRead != nByte);
	(void)fprintf(stderr, "\n%06zX\n", nWrite);

	/* ビット誤りを数えて結果を表示する */
	cnt = 0;
	for (ssize_t i = 0; i < nByte; i++)
		cnt += popTab[(tbuf[i] ^ rbuf[i]) & 0xFF];
	(void)printf("%lu %zd %zd %f\n", chipRate, nByte, cnt, cnt/8.0/nByte);

	return 0;
}
