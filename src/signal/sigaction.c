#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "syscall.h"
#include "pthread_impl.h"
#include "libc.h"
#include "ksigaction.h"

void __restore(), __restore_rt();

static pthread_t dummy(void) { return 0; }
weak_alias(dummy, __pthread_self_def);

int __libc_sigaction(int sig, const struct sigaction *sa, struct sigaction *old)
{
	struct k_sigaction ksa;
	if (sa) {
		ksa.handler = sa->sa_handler;
		ksa.flags = sa->sa_flags | SA_RESTORER;
		ksa.restorer = (sa->sa_flags & SA_SIGINFO) ? __restore_rt : __restore;
		memcpy(&ksa.mask, &sa->sa_mask, sizeof ksa.mask);
	}
	__pthread_self_def();
	if (syscall(SYS_rt_sigaction, sig, sa?&ksa:0, old?&ksa:0, sizeof ksa.mask))
		return -1;
	if (old) {
		old->sa_handler = ksa.handler;
		old->sa_flags = ksa.flags;
		memcpy(&old->sa_mask, &ksa.mask, sizeof ksa.mask);
	}
	return 0;
}

int __sigaction(int sig, const struct sigaction *sa, struct sigaction *old)
{
	if (sig-32U < 3) {
		errno = EINVAL;
		return -1;
	}
	return __libc_sigaction(sig, sa, old);
}

weak_alias(__sigaction, sigaction);
