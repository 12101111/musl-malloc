#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "__dirent.h"
#include "libc.h"

int readdir_r(DIR *dir, struct dirent *buf, struct dirent **result)
{
	struct dirent *de;
	int errno_save = errno;
	int ret;
	
	LOCK(&dir->lock);
	errno = 0;
	de = readdir(dir);
	if ((ret = errno)) {
		UNLOCK(&dir->lock);
		return ret;
	}
	errno = errno_save;
	if (de) memcpy(buf, de, de->d_reclen);
	else buf = NULL;

	UNLOCK(&dir->lock);
	*result = buf;
	return 0;
}

LFS64_2(readdir_r, readdir64_r);
