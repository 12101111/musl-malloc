#include "stdio_impl.h"

static off_t retneg1(FILE *f, off_t off, int whence)
{
	return -1;
}

off_t __stdio_seek(FILE *f, off_t off, int whence)
{
	off_t ret;
#ifdef __NR__llseek
	if (syscall5(__NR__llseek, f->fd, off>>32, off, (long)&ret, whence)<0)
		ret = -1;
#else
	ret = syscall3(__NR_lseek, f->fd, off, whence);
#endif
	/* Detect unseekable files and optimize future failures out */
	if (ret < 0 && off == 0 && whence == SEEK_CUR)
		f->seek = retneg1;
	return ret;
}
