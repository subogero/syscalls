#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

int period_us = 1000;
int ticks = 0;
char filename[] = "channel0";
#define T_MAX_US 10000

static void periodic(int signal);

int main(int argc, char *argv[])
{
	/* Set up periodic SIGALRM and its handler */
	struct itimerval period = {
		{	period_us / 1000000,
			period_us % 1000000,
		},
		{	period_us / 1000000,
			period_us % 1000000,
		},
	};
	signal(SIGALRM, periodic);
	setitimer(ITIMER_REAL, &period, NULL);

	int fdr = -1;
	static char buffer[] = "    \n";
	while (ticks < T_MAX_US) {
		if (fdr < 0) {
			fdr = open(filename, O_RDONLY);
		}
		if (fdr >= 0) {
			lseek(fdr, 0, SEEK_SET);
			read(fdr, buffer, 4);
			write(1, buffer, 5);
		}
	}
	close(fdr);
	return 0;
}

/* SIGALRM handler stores times, re-installs for 1min, eval each 1000 cycles */
static void periodic(int sig)
{
	int fdw;
	static char buffer[4];
	if (!ticks) {
		fdw = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0600);
	}
	if (ticks++ < T_MAX_US) {
		memset(buffer, '0' + ticks % 10, 4);
		lseek(fdw, 0, SEEK_SET);
		write(fdw, buffer, 4);
		signal(SIGALRM, periodic);
	}
	else {
		close(fdw);
		unlink(filename);
	}
}
