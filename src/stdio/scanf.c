#include <stdio.h>
#include <stdarg.h>

int scanf(const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vscanf(fmt, ap);
	va_end(ap);
	return ret;
}
