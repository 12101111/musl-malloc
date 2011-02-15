#include <inttypes.h>
#include <errno.h>
#include <ctype.h>

intmax_t strtoimax(const char *s1, char **p, int base)
{
	const unsigned char *s = (const void *)s1;
	int sign = 0;
	uintmax_t x;

	/* Initial whitespace */
	for (; isspace(*s); s++);

	/* Optional sign */
	if (*s == '-') sign = *s++;
	else if (*s == '+') s++;

	x = strtoumax((const void *)s, p, base);
	if (x > INTMAX_MAX) {
		if (!sign || -x != INTMAX_MIN)
			errno = ERANGE;
		return sign ? INTMAX_MIN : INTMAX_MAX;
	}
	return sign ? -x : x;
}
