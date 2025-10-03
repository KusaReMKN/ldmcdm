#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define BUFLEN	16
#define DEVICE	"/dev/modem"
#define GIGA	1000000000
#define SPEED	"300"

#define MAX(a, b)	((a) > (b) ? (a) : (b))

static void
usage(void)
{
	fprintf(stderr, "usage: tdrive [-d transmitter] [-s speed]"
			" [file ...]\n");
	exit(EXIT_FAILURE);
}

static void
handler(int sig)
{
	/* Nothing to do */
}

int
main(int argc, char *argv[])
{
	struct itimerspec it;
	struct sigaction sa;
	struct sigevent sigev;
	struct termios tos;
	struct timespec tv;
	fd_set rfds;
	ssize_t bytes;
	timer_t timerid;
	unsigned long period, pnprev, pnsec;
	int c, dev, fd, i, nfds, psec, reqSpeed;
	char *device, *endp, *filename, *speed;
	char buf[BUFLEN];

	device = DEVICE;
	speed = SPEED;
	while ((c = getopt(argc, argv, "d:s:")) != -1)
		switch (c) {
		case 'd':
			device = optarg;
			break;
		case 's':
			speed = optarg;
			if (strtoul(speed, &endp, 0) == 0 || *endp != '\0')
				errx(EXIT_FAILURE, "invalid speed");
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* シグナルハンドラの設定 */
	sa.sa_handler = handler;
	(void)sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) == -1)
		err(EXIT_FAILURE, "sigaction");

	/* タイマを作る */
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = SIGRTMIN;
	if (timer_create(CLOCK_MONOTONIC, &sigev, &timerid) == -1)
		err(EXIT_FAILURE, "timer_create");
	/* 16 周期分のタイマ時間を用意しておく */
	period = GIGA / strtoul(speed, NULL, 0);
	pnprev = pnsec = psec = 0;
	for (i = 0; i < 16; i++) {
		pnsec = (pnprev + period) % GIGA;
		if (pnsec < pnprev)
			psec++;
		pnprev = pnsec;
	}
	it.it_interval.tv_sec = it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = period / GIGA * 16 + psec;
	it.it_value.tv_nsec = pnsec;

	/* 待ち時間用の設定（1 bit は 32 周期） */
	pnprev = pnsec = psec = 0;
	for (i = 0; i < 32 * BUFLEN; i++) {
		pnsec = (pnprev + period) % GIGA;
		if (pnsec < pnprev)
			psec++;
		pnprev = pnsec;
	}
	tv.tv_sec = period / GIGA * 32 * BUFLEN + psec;
	tv.tv_nsec = pnsec;

	/* 送信機を開く */
	dev = open(device, O_RDWR | O_NOCTTY);
	if (dev == -1)
		err(EXIT_FAILURE, "%s", device);
	/* 送信機を RAW モードにする（XXX） */
	if (tcgetattr(dev, &tos) == -1)
		err(EXIT_FAILURE, "%s", device);
	cfmakeraw(&tos);
	if (tcsetattr(dev, TCSANOW, &tos) == -1)
		err(EXIT_FAILURE, "%s", device);
	/* 次の送信時にスピードを送る必要がある */
	reqSpeed = 1;

	/* 各ファイルを送信する */
	for (i = 0; argv[i] != NULL || i == 0; i++) {
		/* 引数がないか '-' のときは標準入力を相手にする */
		if (argv[i] == NULL || strcmp(argv[i], "-") == 0) {
			filename = "stdin";
			fd = STDIN_FILENO;
		} else {
			filename = argv[i];
			fd = open(filename, O_RDONLY | O_NOCTTY);
		}
		if (fd == -1) {
			warn("%s", filename);
			continue;
		}

		do {
			/* 送信機とファイルとを同時に監視する */
			FD_ZERO(&rfds);
			FD_SET(dev, &rfds);
			FD_SET(fd, &rfds);
			nfds = MAX(dev, fd) + 1;
			if (select(nfds, &rfds, NULL, NULL, NULL) == -1)
				err(EXIT_FAILURE, "select");

			/* 送信機が何か言っているなら読み込む */
			if (FD_ISSET(dev, &rfds)) {
				bytes = read(dev, buf, sizeof(buf));
				if (bytes == -1)
					err(EXIT_FAILURE, "%s", device);
				/* 送信終了でなければ無視する */
				if (strncmp(buf, "?", bytes) != 0)
					continue;
				/* 受信機が全てを忘却するまで待つ */
				if (timer_settime(timerid, 0, &it, NULL) == -1)
					err(EXIT_FAILURE, "timer_settime");
				(void)pause();
				/* 速度を再度送信する */
				reqSpeed = 1;
				continue;
			}

			/* ファイルから読んで送信機に送る */
			bytes = read(fd, buf, sizeof(buf));
			if (bytes <= 0)
				break;
			if (reqSpeed && dprintf(dev, "\r%s\r", speed) < 0)
					err(EXIT_FAILURE, "%s", device);
			if (write(dev, buf, bytes) == -1)
				err(EXIT_FAILURE, "%s", device);
			reqSpeed = 0;

			/* 送信機のバッファが溢れないように少し待つ */
			if (nanosleep(&tv, NULL) == -1)
				err(EXIT_FAILURE, "nanosleep");
		} while (bytes > 0);
		if (bytes == -1)
			warn("%s", filename);

		/* ファイルを閉じる */
		if (fd != STDIN_FILENO)
			(void)close(fd);

		/* 引数なしの場合はここでおしまい */
		if (argv[i] == NULL)
			break;
	}

	/* 送信機を閉じる */
	(void)close(dev);

	return 0;
}
