#include "stdio_impl.h"
#include "intscan.h"
#include "shgetc.h"

static unsigned long long strtox(const char *s, char **p, int base, unsigned long long lim)
{
	/* FIXME: use a helper function or macro to setup the FILE */
	FILE f;
	f.flags = 0;
	f.buf = f.rpos = (void *)s;
	if ((size_t)s > (size_t)-1/2)
		f.rend = (void *)-1;
	else
		f.rend = (unsigned char *)s+(size_t)-1/2;
	f.lock = -1;
	shlim(&f, 0);
	unsigned long long y = __intscan(&f, base, 1, lim);
	if (p) {
		size_t cnt = shcnt(&f);
		*p = (char *)s + cnt;
	}
	return y;
}

unsigned long long strtoull(const char *s, char **p, int base)
{
	return strtox(s, p, base, ULLONG_MAX);
}

long long strtoll(const char *s, char **p, int base)
{
	return strtox(s, p, base, LLONG_MIN);
}

unsigned long strtoul(const char *s, char **p, int base)
{
	return strtox(s, p, base, ULONG_MAX);
}

long strtol(const char *s, char **p, int base)
{
	return strtox(s, p, base, 0UL+LONG_MIN);
}

intmax_t strtoimax(const char *s, char **p, int base)
{
	return strtoll(s, p, base);
}

uintmax_t strtoumax(const char *s, char **p, int base)
{
	return strtoull(s, p, base);
}
