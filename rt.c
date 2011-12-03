/* Register a signal-handler for SIGALRM with signal().
 * Generate SIGALRM periodically with setitimer() by a user-defined interval.
 * Measure timing accuracy with gettimeofday().
 * Set up a pipe to send commands from signal handler to main().
 * Send 'w' command to evaluate timing after every 1000 periodic calls.
 * Send 'q' command to quit after 1min.
 */
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

int times[1000];
struct timeval t_start;
int fds[2];
int period_us;

static void periodic(int signal);
static void evaluate(void);

int main(int argc, char *argv[])
{
	/* Get interval [us] from 1st argument */
	if (argc < 2) {
		write(2, "Please specify time period in [us]!\n", 29);
		return 1;
	}
	period_us = atoi(argv[1]);
	struct itimerval period = {
		{	period_us / 1000000,
			period_us % 1000000,
		},
		{	period_us / 1000000,
			period_us % 1000000,
		},
	};
	/* Open a pipe for signalhandler to send commands to main() */
	pipe(fds);
	/* Set up periodic SIGALRM and its handler */
	gettimeofday(&t_start, NULL);
	signal(SIGALRM, periodic);
	setitimer(ITIMER_REAL, &period, NULL);
	/* Main loop waiting for commands from periodic signal handler */
	while (1) {
		char cmd;
		read(fds[0], &cmd, 1);
		if (cmd == 'w')
			evaluate();
		if (cmd == 'q') {
			close(fds[0]);
			return 0;
		}
	}
}

/* SIGALRM handler stores times, re-installs for 1min, eval each 1000 cycles */
static void periodic(int sig)
{
	struct timeval t;
	static int ticks = 0;
	gettimeofday(&t, NULL);
	int dt = 1000000 * (t.tv_sec - t_start.tv_sec) + t.tv_usec;
	times[ticks++] = dt;
	if (ticks < 1000) {
		signal(SIGALRM, periodic);
	}
	else {
		ticks = 0;
		write(fds[1], "w", 1);
		if (dt >= 60000000) {
			write(fds[1], "q", 1);
			close(fds[1]);
		}
		else {
			signal(SIGALRM, periodic);
		}
	}
}

/* Evaluate timing accuracy */
static void evaluate(void)
{
	static char heading = 0;
	int i;
	double mean = 0.0;
	double sd = 0.0;
	int min = 0x7FFFFFFF;
	int max = 0;
	/* Average and Standard deviation */
	for (i = 1; i < 1000; ++i) {
		int dt = times[i] - times[i-1];
		if (dt < min) min = dt;
		if (dt > max) max = dt;
		mean += (double)dt / 999.0;
		double vari = (double)(times[i] - times[i-1]) - period_us;
		sd += vari * vari / 999.0;
	}
	sd = sqrt(sd);
	/* Print evaluation */
	if (!heading) {
		printf("Mean  [us]   SD  [us]     [%%]  Min [us]     [%%]  Max [us]     [%%]\n");
		heading = 1;
	}
	printf("%10.3f ", mean);
	printf("%10.3f %7.3f ", sd, sd  * 100.0 / period_us);
	printf("%9d %7.3f ", min, (period_us - min) * 100.0 / period_us);
	printf("%9d %7.3f\n", max, (max - period_us) * 100.0 / period_us);
}
