#define _XOPEN_SOURCE	600
#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int
openPseudoTerminal(int flags, FILE *fpv[2])
{
	FILE *rfp, *wfp;
	int ptm;

	/* open a pseudo-terminal */
	ptm = posix_openpt(flags);
	if (ptm == -1)
		return -1;

	/* grant access to the slave pseudo-terminal */
	if (grantpt(ptm) == -1)
		return -1;

	/* unlock the pseudo-terminal master/slave pair */
	if (unlockpt(ptm) == -1)
		return -1;

	/* associate a readable stream with the pseudo-terminal */
	rfp = fdopen(ptm, "r");
	if (rfp == NULL)
		return -1;
	setbuf(rfp, NULL);

	/* associate a writable stream with the pseudo-terminal */
	wfp = fdopen(ptm, "w");
	if (wfp == NULL)
		return -1;
	setbuf(wfp, NULL);

	/* set FILE pointers */
	fpv[0] = rfp;
	fpv[1] = wfp;
	return ptm;
}

static char *
getLine(char *buf, size_t len, FILE *fp)
{
	size_t i;
	int c;

	for (i = 0; i < len-1; i++) {
		c = getc(fp);
		if (c == EOF && i == 0)
			return NULL;
		buf[i] = c;
		if (c == EOF || c == '\r' || c == '\n')
			break;
	}
	buf[i] = 0;

	return buf;
}

struct context {
	FILE *fp;
	volatile size_t bufTail;
	char *buf;
};

static void *
getter(void *arg)
{
	struct context *ctx = (struct context *)arg;

	for (;;)
		ctx->buf[ctx->bufTail++] = getc(ctx->fp);
	/* NOTREACHED */

	return NULL;
}

int
main(void)
{
	struct context ctx;
	pthread_t thread;
	FILE *rxfpv[2], *txfpv[2];
	useconds_t period;
	unsigned long chipRate;
	int rxptm, txptm;
	char *tmp, buf[1024];

	/* open a pseudo-terminal that acts as a receiver */
	rxptm = openPseudoTerminal(O_RDWR | O_NOCTTY, rxfpv);
	if (rxptm == -1)
		err(1, "openPseudoTerminal: (receiver)");

	/* show the name of receiver */
	tmp = ptsname(rxptm);
	if (tmp == NULL)
		err(1, "ptsname: (receiver)");
	fprintf(stderr, "receiver: %s\n", tmp);

	/* open a pseudo-terminal that acts as a transmitter */
	txptm = openPseudoTerminal(O_RDWR | O_NOCTTY, txfpv);
	if (txptm == -1)
		err(1, "openPseudoTerminal: (transmitter)");

	/* show the name of transmitter */
	tmp = ptsname(txptm);
	if (tmp == NULL)
		err(1, "ptsname: (transmitter)");
	fprintf(stderr, "transmitter: %s\n", tmp);

	/* emulate the behaviour of a pair of receiver and transmitter */
	for (;;) {
		do {
			(void)fprintf(txfpv[1], "?");
			if (getLine(buf, sizeof(buf), txfpv[0]) == NULL)
				err(1, "getLine: (transmitter)");
			chipRate = strtoul(buf, &tmp, 0);
		} while (*tmp != '\0' || chipRate == 0);
		(void)fprintf(txfpv[1], "%lu!", chipRate);

		ctx.fp = txfpv[0];
		ctx.buf = buf;
		ctx.bufTail = 0;
		(void)pthread_create(&thread, NULL, getter, &ctx);

		period = 1000000. / chipRate;
		usleep(period * 52);	/* delay due to the preamble */
		for (size_t i = 0; i < ctx.bufTail; i++) {
			(void)putc(buf[i], rxfpv[1]);
			usleep(period * 4);
		}
		(void)pthread_cancel(thread);
	}
	/* NOTREACHED */

	return -1;
}
