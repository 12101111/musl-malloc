#include <sys/socket.h>
#include "syscall.h"
#include "libc.h"

int accept(int fd, struct sockaddr *addr, socklen_t *len)
{
	int ret;
	CANCELPT_BEGIN;
	ret = socketcall(accept, fd, addr, len, 0, 0, 0);
	CANCELPT_END;
	return ret;
}
