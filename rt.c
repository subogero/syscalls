/* Register a signal-handler for SIGALRM with signal().
 * Generate SIGALRM periodically with setitimer() by a user-defined interval.
 * Measure timing accuracy with gettimeofday(),
 * evaluate it statistically after 1000 calls.
 */
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

int times[1000];
volatile int ticks = 0;
struct timeval t_start;

static void periodic(int signal);

int main(int argc, char *argv[])
{
	/* Get interval [us] from 1st argument */
	if (argc < 2) {
		write(2, "Please specify time period in [us]!\n", 29);
		return 1;
	}
	int period_us = atoi(argv[1]);
	struct itimerval period = {
		{	period_us / 1000000,
			period_us % 1000000,
		},
		{	period_us / 1000000,
			period_us % 1000000,
		},
	};
	/* Set up periodic SIGALRM and its handler */
	gettimeofday(&t_start, NULL);
	signal(SIGALRM, periodic);
	setitimer(ITIMER_REAL, &period, NULL);
	/* Wait for 1000 signals, then evaluate */
	while (ticks < 1000);
	int i;
	double mean = 0.0;
	int min = 0x7FFFFFFF;
	int max = 0;
	for (i = 1; i < 1000; ++i) {
		int dt = times[i] - times[i-1];
		if (dt < min) min = dt;
		if (dt > max) max = dt;
		mean += (double)dt / 999.0;
	}
	double sd = 0.0;
	for (i = 1; i < 1000; ++i) {
		double vari = (double)(times[i] - times[i-1]) - period_us;
		sd += vari * vari / 999.0;
	}
	sd = sqrt(sd);
	printf("Calling period time\n");
	printf("Mean [us] = %10.3f\n", mean);
	printf("SD   [us] = %10.3f %7.3f %%\n" , sd , sd  * 100.0 / period_us);
	printf("Min  [us] = %6d     %7.3f %%\n", min, (period_us - min) * 100.0 / period_us);
	printf("Max  [us] = %6d     %7.3f %%\n", max, (max - period_us) * 100.0 / period_us);
	return 0;
}

/* SIGALRM handler stores times and re-installs itself 1000 times */
static void periodic(int sig)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	times[ticks++] = 1000000 * (t.tv_sec - t_start.tv_sec) + t.tv_usec;
	if (ticks < 1000) signal(SIGALRM, periodic);
}
