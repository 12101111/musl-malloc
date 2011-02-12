#include "stdio_impl.h"

FILE *__fdopen(int fd, const char *mode)
{
	FILE *f;
	struct termios tio;
	int plus = !!strchr(mode, '+');

	/* Check for valid initial mode character */
	if (!strchr("rwa", *mode)) return 0;

	/* Allocate FILE+buffer or fail */
	if (!(f=malloc(sizeof *f + UNGET + BUFSIZ))) return 0;

	/* Zero-fill only the struct, not the buffer */
	memset(f, 0, sizeof *f);

	/* Impose mode restrictions */
	if (!plus) f->flags = (*mode == 'r') ? F_NOWR : F_NORD;

	/* Set append mode on fd if opened for append */
	if (*mode == 'a') {
		int flags = __syscall_fcntl(fd, F_GETFL, 0);
		__syscall_fcntl(fd, F_SETFL, flags | O_APPEND);
	}

	f->fd = fd;
	f->buf = (unsigned char *)f + sizeof *f + UNGET;
	f->buf_size = BUFSIZ;

	/* Activate line buffered mode for terminals */
	f->lbf = EOF;
	if (!(f->flags & F_NOWR) && !__syscall_ioctl(fd, TCGETS, &tio))
		f->lbf = '\n';

	/* Initialize op ptrs. No problem if some are unneeded. */
	f->read = __stdio_read;
	f->write = __stdio_write;
	f->seek = __stdio_seek;
	f->close = __stdio_close;

	/* Add new FILE to open file list */
	OFLLOCK();
	f->next = ofl_head;
	if (ofl_head) ofl_head->prev = f;
	ofl_head = f;
	OFLUNLOCK();

	return f;
}

weak_alias(__fdopen, fdopen);
