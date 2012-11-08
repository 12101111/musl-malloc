#include "pthread_impl.h"

int pthread_cond_init(pthread_cond_t *restrict c, const pthread_condattr_t *restrict a)
{
	*c = (pthread_cond_t){0};
	if (a) {
		c->_c_clock = *a & 0x7fffffff;
		if (*a>>31) c->_c_mutex = (void *)-1;
	}
	return 0;
}
