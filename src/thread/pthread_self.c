#include "pthread_impl.h"

static struct pthread main_thread;

#undef errno
static int *errno_location()
{
	return __pthread_self()->errno_ptr;
}

static int init_main_thread()
{
	main_thread.self = &main_thread;
	if (__set_thread_area(&main_thread) < 0)
		return -1;
	syscall1(__NR_set_tid_address, (long)&main_thread.tid);
	main_thread.errno_ptr = __errno_location();
	libc.errno_location = errno_location;
	main_thread.tid = main_thread.pid = getpid();
	return 0;
}

pthread_t pthread_self()
{
	static int init, failed;
	if (!init) {
		if (failed) return 0;
		if (init_main_thread() < 0) failed = 1;
		if (failed) return 0;
		init = 1;
	}
	return __pthread_self();
}
