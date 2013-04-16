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
#include <sched.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

struct timeval t_start;
int fds[2];
int period_us;

static void periodic(int sig);
static void evaluate(int sig);

struct dataset {
	int samples;
	int min;
	int max;
	double mean;
	double variance;
	double sd;
};
static void dataset_fill(struct dataset *this, int samples, int value);
static void dataset_merge(struct dataset *this, const struct dataset *src);
static void dataset_print(struct dataset *this, int heading);

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
	/* PARENT: Set up 1000 times slower periodic SIGALRM for eval */
	if (!child_pid) {
		/* Set scheduling prio if specified */
		if (argc >= 3) {
			int prio = atoi(argv[2]);
			if (prio > 0) {
				struct sched_param schedp;
				schedp.sched_priority = prio;
				sched_setscheduler(0, SCHED_FIFO, &schedp);
			}
		}
		struct itimerval period = {
			{ period_us / 1000000, period_us % 1000000, },
			{ period_us / 1000000, period_us % 1000000, },
		};
		close(fds[0]);
		gettimeofday(&t_start, NULL);
		signal(SIGALRM, periodic);
		setitimer(ITIMER_REAL, &period, NULL);
	} else {
		printf("Real time child process PID %d\n", child_pid);
		int eval_period = period_us * 1000;
		struct itimerval period = {
			{ eval_period / 1000000, eval_period % 1000000, },
			{ eval_period / 1000000, eval_period % 1000000, },
		};
		close(fds[1]);
		signal(SIGALRM, evaluate);
		setitimer(ITIMER_REAL, &period, NULL);
	}
	/* Main idle loop in both: everything done by signal handlers */
	while (1) {
		pause();
	}
	return 0;
}

/* SIGALRM handler stores times, re-installs for 60000 times, then quits */
static void periodic(int sig)
{
	struct timeval t;
	static int time_us = 0;
	gettimeofday(&t, NULL);
	int dt = (t.tv_sec - t_start.tv_sec) * 1000000
	       + t.tv_usec - t_start.tv_usec;
	t_start = t;
	write(fds[1], &dt, 4);
	time_us += dt;
	if (time_us < 30000000) {
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
	struct dataset last = { 0, };
	static struct dataset all = { 0, };
	int times[5000];
	int input = read(fds[0], times, 5000);
	int samples = input / 4;
	if (input % 4) {
		printf("Partial timestamp received: %d bytes\n", input % 4);
		exit(1);
	}
	signal(SIGALRM, evaluate);
	/* Average and Standard deviation */
	for (i = 0; i < samples; ++i) {
		dataset_fill(&last, samples, times[i]);
	}
	dataset_merge(&all, &last);
	/* Print evaluation */
	static char heading = 1;
	dataset_print(&last, heading);
	heading = 0;
	if (input < 3000) {
		dataset_print(&all, 1);
		exit(0);
	}
}

static void dataset_fill(struct dataset *this, int samples, int value)
{
	/* Init min/max, assumes all fields zero */
	if (this->samples == 0) {
		this->min = 0x7FFFFFFF;
		this->max = 0x80000000;
	}
	/* Process a sample */
	if (value < this->min) this->min = value;
	if (value > this->max) this->max = value;
	this->mean += (double)value / samples;
	double vari = (double)value - period_us;
	this->variance += vari * vari / samples;
	this->sd = sqrt(this->variance);
	this->samples = samples;
}

static void dataset_merge(struct dataset *this, const struct dataset *src)
{
	/* this our source empty: use other one */
	if (src->samples == 0) {
		return;
	}
	if (this->samples == 0) {
		*this = *src;
		return;
	}
	int sum_samples = this->samples + src->samples;
	this->mean = (this->samples * this->mean +
	              src->samples * src->mean)
	           / sum_samples;
	this->variance = (this->samples * this->variance +
	                 src->samples * src->variance)
	               / sum_samples;
	this->sd = sqrt(this->variance);
	if (src->max > this->max) {
		this->max = src->max;
	}
	if (src->min < this->min) {
		this->min = src->min;
	}
	this->samples = sum_samples;
}

static void dataset_print(struct dataset *this, int heading)
{
	/* Skip printing if dataset empty */
	if (this->samples == 0) {
		return;
	}
	if (heading) {
		printf("------------------------------------------------------------------------\n");
		printf("     n  Mean [us]       [us] SD  [%%]      [us] Min [%%]      [us] Max [%%]\n");
		printf("------------------------------------------------------------------------\n");
	}
	printf("%6d ", this->samples);
	printf("%10.3f ", this->mean);
	printf("%10.3f %7.3f ", this->sd, this->sd  * 100.0 / period_us);
	printf("%9d %7.3f ", this->min, (period_us - this->min) * 100.0 / period_us);
	printf("%9d %7.3f ", this->max, (this->max - period_us) * 100.0 / period_us);
	printf("\n");
}
