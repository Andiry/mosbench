/*
 * Operate on per-process files in the same directory and use a pipe.
 */

#define TESTNAME "fops_pipe"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "bench.h"
#include "support/mtrace.h"

#define NPMC 3

#define MAX_PROC 256

#define XOP xopen

static uint64_t start;

static unsigned int ncores;
static unsigned int nprocs;
static unsigned int npairs;
static unsigned int the_time;
static unsigned int close_fd;
static char *the_file;

static uint64_t pmc_start[NPMC];
static uint64_t pmc_stop[NPMC];

static struct {
	union __attribute__((__aligned__(64))){
		volatile uint64_t v;
		char pad[64];
	} count[MAX_PROC];
	volatile int run;
} *shared;

static inline void xopen(const char *fn)
{
	int fd = open(fn, O_RDONLY);
	if (fd < 0)
		edie("open");
	close(fd);
}

static inline void xstat(const char *fn)
{
	struct stat st;
	if (stat(fn, &st))
		edie("stat");
}

static inline void xgettid(const char *fn)
{
	syscall(SYS_gettid);
}

static void sighandler(int x)
{
	float sec, rate, one;
	uint64_t stop, tot;
	unsigned int i;

	mtrace_enable_set(0, TESTNAME);

	for (i = 0; i < NPMC; i++)
		pmc_stop[i] = read_pmc(i);

	stop = usec();
	shared->run = 0;

	tot = 0;
	for (i = 0; i < nprocs; i++)
		tot += shared->count[i].v;

	sec = (float)(stop - start) / 1000000;
	rate = (float)tot / sec;
	one = (float)(stop - start) / (float)tot;

	printf("rate: %f per sec\n", rate);
	printf("lat: %f usec\n", one);

	for (i = 0; i < NPMC; i++) {
		rate = (float)(pmc_stop[i] - pmc_start[i]) / 
			(float) shared->count[0].v;
		printf("pmc(%u): %f per op\n", i, rate);
	}
}

static void xread(int fd, char *buffer, size_t count)
{
	while (count) {
		int r = read(fd, buffer, count);
		if (r < 0)
			edie("read");
		else if (r == 0)
			edie("read returned 0");
		buffer += r;
		count -= r;
	}
}

static void xwrite(int fd, const char *buffer, size_t count)
{
	while (count) {
		int r = write(fd, buffer, count);
		if (r < 0)
			edie("write");
		else if (r == 0)
			edie("write returned 0");
		buffer += r;
		count -= r;
	}
}

static void test(unsigned int proc, int pip, int read_pip)
{
	char buffer[1];
	char fn[128];

	setaffinity(proc % ncores);

	snprintf(fn, sizeof(fn), "%s.%d", the_file, 0);

	if (proc == 0) {
		unsigned int i;

		sleep(1);

		if (signal(SIGALRM, sighandler) == SIG_ERR)
			die("signal failed\n");
		alarm(the_time);
		start = usec();

		for (i = 0; i < NPMC; i++)
			pmc_start[i] = read_pmc(i);

		mtrace_enable_set(1, TESTNAME);
		shared->run = 1;
	} else {
		while (shared->run == 0)
			__asm __volatile ("pause");
	}

	while (shared->run) {
		XOP(fn);
		if (read_pip)
			xread(pip, buffer, sizeof(buffer));
		else
			xwrite(pip, buffer, sizeof(buffer));
		shared->count[proc].v++;
	}
}

static void parent_test(unsigned int proc)
{
	int pipes[2];
	pid_t p;

	if (pipe(pipes))
		edie("pipe");
	
	p = fork();
	if (p < 0)
		edie("fork");
	else if (p == 0) {
		test(proc + 1, pipes[0], 1);
		return;
	}
	test(proc, pipes[1], 0);
}

static void initshared(void)
{
	shared = mmap(0, sizeof(*shared), PROT_READ|PROT_WRITE, 
		      MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	if (shared == MAP_FAILED)
		die("mmap failed");
}

static void initfile(void)
{
	unsigned int i;
	char buf[128];
	int fd;

	for (i = 0; i < nprocs; i++) {
		snprintf(buf, sizeof(buf), "%s.%d", the_file, i);
		setaffinity(i % ncores);
		fd = creat(buf, S_IRUSR|S_IWUSR);
		if (fd < 0)
			edie("creat");

		if (close_fd)
			close(fd);
	}
}

int main(int ac, char **av)
{
	unsigned int i;

	if (ac < 4)
		die("usage: %s time npairs base-filename [close]", av[0]);

	the_time = atoi(av[1]);
	npairs = atoi(av[2]);
	nprocs = npairs * 2;
	the_file = av[3];
	ncores = sysconf(_SC_NPROCESSORS_CONF);

	if (ac > 4)
		close_fd = atoi(av[4]);

	initshared();
	initfile();

	for (i = 1; i < npairs; i++) {
		pid_t p;

		p = fork();
		if (p < 0)
			edie("fork");
		else if (p == 0) {
			parent_test(i * 2);
			return 0;
		}
	}

	parent_test(0);
	return 0;
}
