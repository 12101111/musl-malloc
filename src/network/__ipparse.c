#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "__dns.h"
#include <stdio.h>

int __ipparse(void *dest, int family, const char *s)
{
	unsigned char *d = dest;
	unsigned long a[16] = { 0 };
	const char *z;
	int i;

	if (family == AF_INET6) goto not_v4;

	for (i=0; i<4 && *s; i++) {
		a[i] = strtoul(s, (char **)&z, 0);
		if (z==s || (*z && *z != '.')) goto not_v4;
		s=z+1;
	}
	switch (i) {
	case 0:
		a[1] = a[0] & 0xffffff;
		a[0] >>= 24;
	case 1:
		a[2] = a[1] & 0xffff;
		a[1] >>= 16;
	case 2:
		a[3] = a[2] & 0xff;
		a[2] >>= 8;
	}
	((struct sockaddr_in *)d)->sin_family = AF_INET;
	d = (void *)&((struct sockaddr_in *)d)->sin_addr;
	for (i=0; i<4; i++) d[i] = a[i];
	return 0;

not_v4:
	return -1;
}
