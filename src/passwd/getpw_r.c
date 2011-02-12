#include "pwf.h"

#define FIX(x) (pw->pw_##x = pw->pw_##x-line+buf)

static int getpw_r(const char *name, uid_t uid, struct passwd *pw, char *buf, size_t size, struct passwd **res)
{
	FILE *f;
	char *line = 0;
	size_t len = 0;
	int rv = 0;

	f = fopen("/etc/passwd", "rb");
	if (!f) return errno;

	*res = 0;
	while (__getpwent_a(f, pw, &line, &len)) {
		if (name && !strcmp(name, pw->pw_name)
		|| !name && pw->pw_uid == uid) {
			if (size < len) {
				rv = ERANGE;
				break;
			}
			*res = pw;
			memcpy(buf, line, len);
			FIX(name);
			FIX(passwd);
			FIX(gecos);
			FIX(dir);
			FIX(shell);
			break;
		}
	}
 	free(line);
	fclose(f);
	return rv;
}

int getpwnam_r(const char *name, struct passwd *pw, char *buf, size_t size, struct passwd **res)
{
	return getpw_r(name, 0, pw, buf, size, res);
}

int getpwuid_r(uid_t uid, struct passwd *pw, char *buf, size_t size, struct passwd **res)
{
	return getpw_r(0, uid, pw, buf, size, res);
}
