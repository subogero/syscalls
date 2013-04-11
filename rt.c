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

struct timeval t_start;
int fds[2];
int period_us;

static void periodic(int sig);
static void evaluate(int sig);

int main(int argc, char *argv[])
{
	/* Get interval [us] from 1st argument */
	if (argc < 2) {
		write(2, "Please specify time period in [us]!\n", 29);
		return 1;
	}
	period_us = atoi(argv[1]);
	/* Open a pipe for signalhandler to send commands to main() */
	pipe(fds);
	int child_pid = fork();
	/* CHILD: Set up specified periodic SIGALRM for real-time task */
	if (!child_pid) {
		struct itimerval period = {
			{ period_us / 1000000, period_us % 1000000, },
			{ period_us / 1000000, period_us % 1000000, },
		};
		close(fds[0]);
		gettimeofday(&t_start, NULL);
		signal(SIGALRM, periodic);
		setitimer(ITIMER_REAL, &period, NULL);
	}
	/* PARENT: Set up 1000 times slower periodic SIGALRM for eval */
	else {
		struct itimerval period = {
			{ period_us / 1000, period_us % 1000, },
			{ period_us / 1000, period_us % 1000, },
		};
		close(fds[1]);
		signal(SIGALRM, evaluate);
		setitimer(ITIMER_REAL, &period, NULL);
	}
	while (1) {
		pause();
	}
	return 0;
}

/* SIGALRM handler stores times, re-installs for 60000 times, then quits */
static void periodic(int sig)
{
	struct timeval t;
	static int ticks = 0;
	gettimeofday(&t, NULL);
	int dt = (t.tv_sec - t_start.tv_sec) * 1000000
	       + t.tv_usec - t_start.tv_usec;
	t_start = t;
	write(fds[1], &dt, 4);
	ticks++;
	if (ticks < 6000) {
		signal(SIGALRM, periodic);
	} else {
		close(fds[1]);
		exit(0);
	}
}

/* Evaluate timing accuracy */
static void evaluate(int sig)
{
	int i;
	double mean = 0.0;
	double sd = 0.0;
	int min = 0x7FFFFFFF;
	int max = 0;
	int times[5000];
	int input = read(fds[0], times, 5000);
	int samples = input / 4;
	if (input % 4) {
		printf("Partial timestamp received: %d bytes\n", input % 4);
		exit(1);
	}
	if (input == 0) {
		exit(0);
	}
	signal(SIGALRM, evaluate);
	/* Average and Standard deviation */
	for (i = 1; i < samples; ++i) {
		int dt = times[i];
		if (dt < min) min = dt;
		if (dt > max) max = dt;
		mean += (double)dt / samples;
		double vari = (double)dt - period_us;
		sd += vari * vari / samples;
	}
	sd = sqrt(sd);
	/* Print evaluation */
	static char heading = 0;
	if (!heading) {
		printf("n     Mean [us]    SD [us]     [%%]  Min [us]     [%%]  Max [us]     [%%]\n");
		heading = 1;
	}
	printf("%4d ", samples);
	printf("%10.3f ", mean);
	printf("%10.3f %7.3f ", sd, sd  * 100.0 / period_us);
	printf("%9d %7.3f ", min, (period_us - min) * 100.0 / period_us);
	printf("%9d %7.3f\n", max, (max - period_us) * 100.0 / period_us);
}
