#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include "syscall.h"

int pthread_sigmask(int how, const sigset_t *set, sigset_t *old)
{
	int ret;
	if ((unsigned)how - SIG_BLOCK > 2U) return EINVAL;
	ret = -__syscall(SYS_rt_sigprocmask, how, set, old, __SYSCALL_SSLEN);
	if (!ret && old) {
		if (sizeof old->__bits[0] == 8) {
			old->__bits[0] &= ~0x380000000ULL;
		} else {
			old->__bits[0] &= ~0x80000000UL;
			old->__bits[1] &= ~0x3UL;
		}
	}
	return ret;
}
