#include "pthread_impl.h"

int pthread_attr_getdetachstate(pthread_attr_t *a, int *state)
{
	*state = a->_a_detach;
	return 0;
}
