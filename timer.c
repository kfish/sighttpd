#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>		/* Definition of uint64_t */

#define handle_error(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct framerate {
	int fd;
};

static int
framerate_init (struct framerate * fr, double fps)
{
	struct itimerspec new_value;
	struct timespec now;
        long interval;

        interval = (long) (1000000000 / fps);
        printf ("Interval: %ld nanoseconds\n", interval);

	if (clock_gettime(CLOCK_REALTIME, &now) == -1)
		handle_error("clock_gettime");

	/* Create a CLOCK_REALTIME absolute timer with initial
	 * expiration and interval as specified in command line */

	new_value.it_value.tv_sec = now.tv_sec;
	new_value.it_value.tv_nsec = now.tv_nsec;
	new_value.it_interval.tv_sec = 0;
	new_value.it_interval.tv_nsec = interval;

	fr->fd = timerfd_create(CLOCK_REALTIME, 0);
	if (fr->fd == -1)
		handle_error("timerfd_create");

	if (timerfd_settime(fr->fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
		handle_error("timerfd_settime");

        return 0;
}

static uint64_t
framerate_wait (struct framerate * fr)
{
	ssize_t s;
        uint64_t exp;

	s = read(fr->fd, &exp, sizeof(uint64_t));
	if (s != sizeof(uint64_t))
		handle_error("read");

        return exp;
}

static void print_elapsed_time(void)
{
	static struct timespec start;
	struct timespec curr;
	static int first_call = 1;
	int secs, nsecs;

	if (first_call) {
		first_call = 0;
		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
			handle_error("clock_gettime");
	}

	if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1)
		handle_error("clock_gettime");

	secs = curr.tv_sec - start.tv_sec;
	nsecs = curr.tv_nsec - start.tv_nsec;
	if (nsecs < 0) {
		secs--;
		nsecs += 1000000000;
	}
	printf("%d.%03d: ", secs, (nsecs + 500000) / 1000000);
}

static int do_callback (struct framerate * fr)
{
        return framerate_wait (fr); 
}

int main(int argc, char *argv[])
{
        struct framerate fr;
	uint64_t exp, tot_exp;
        int max_exp;
        time_t start;
        double fps;

	if (argc != 3) {
		fprintf(stderr, "%s init-secs [interval-secs max-exp]\n",
			argv[0]);
		exit(EXIT_FAILURE);
	}

        fps = strtod (argv[1], NULL);
	max_exp = atoi(argv[2]);

        framerate_init (&fr, fps);

	print_elapsed_time();
	printf("timer started\n");

	for (tot_exp = 0; tot_exp < max_exp;) {
                exp = do_callback (&fr);
		tot_exp += exp;
		print_elapsed_time();
		printf("read: %llu; total=%llu\n",
	               (unsigned long long) exp,
	               (unsigned long long) tot_exp);
	}

	exit(EXIT_SUCCESS);
}
