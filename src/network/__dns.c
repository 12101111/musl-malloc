#include <stdint.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include "__dns.h"
#include "stdio_impl.h"

#define TIMEOUT 5
#define RETRY 1
#define PACKET_MAX 512
#define PTR_MAX (64 + sizeof ".in-addr.arpa")

int __dns_doqueries(unsigned char *dest, const char *name, int *rr, int rrcnt)
{
	time_t t0 = time(0);
	int fd;
	FILE *f, _f;
	unsigned char _buf[64];
	char line[64], *s, *z;
	union {
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} sa = {0}, ns[3] = {{0}};
	socklen_t sl;
	int nns = 0;
	int family = AF_UNSPEC;
	unsigned char q[280] = "", *r = dest;
	int ql;
	int rlen;
	int got = 0, failed = 0;
	int errcode = EAI_AGAIN;
	int i, j;
	struct timeval tv;
	fd_set fds;
	int id;

	/* Construct query template - RR and ID will be filled later */
	if (strlen(name)-1 >= 254U) return -1;
	q[2] = q[5] = 1;
	strcpy((char *)q+13, name);
	for (i=13; q[i]; i=j+1) {
		for (j=i; q[j] && q[j] != '.'; j++);
		if (j-i-1u > 62u) return -1;
		q[i-1] = j-i;
	}
	q[i+3] = 1;
	ql = i+4;

	/* Make a reasonably unpredictable id */
	gettimeofday(&tv, 0);
	id = tv.tv_usec + tv.tv_usec/256 & 0xffff;

	/* Get nameservers from resolv.conf, fallback to localhost */
	f = __fopen_rb_ca("/etc/resolv.conf", &_f, _buf, sizeof _buf);
	if (f) for (nns=0; nns<3 && fgets(line, sizeof line, f); ) {
		if (strncmp(line, "nameserver", 10) || !isspace(line[10]))
			continue;
		for (s=line+11; isspace(*s); s++);
		for (z=s; *z && !isspace(*z); z++);
		*z=0;
		if (__ipparse(ns+nns, family, s) < 0) continue;
		ns[nns].sin.sin_port = htons(53);
		family = ns[nns++].sin.sin_family;
		sl = family==AF_INET6 ? sizeof sa.sin6 : sizeof sa.sin;
	}
	if (f) __fclose_ca(f);
	if (!nns) {
		ns[0].sin.sin_family = AF_INET;
		ns[0].sin.sin_port = htons(53);
		nns=1;
		sl = sizeof sa.sin;
	}

	/* Get local address and open/bind a socket */
	sa.sin.sin_family = family;
	fd = socket(family, SOCK_DGRAM, 0);
	if (bind(fd, (void *)&sa, sl) < 0) {
		close(fd);
		return -1;
	}
	/* Nonblocking to work around Linux UDP select bug */
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

	/* Loop until we timeout; break early on success */
	for (; time(0)-t0 < TIMEOUT; ) {

		/* Query all configured namservers in parallel */
		for (i=0; i<rrcnt; i++) if (rr[i]) for (j=0; j<nns; j++) {
			q[0] = id+i >> 8;
			q[1] = id+i;
			q[ql-3] = rr[i];
			sendto(fd, q, ql, MSG_NOSIGNAL, (void *)&ns[j], sl);
		}

		/* Wait for a response, or until time to retry */
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = RETRY;
		tv.tv_usec = 0;
		if (select(fd+1, &fds, 0, 0, &tv) <= 0) continue;

		/* Process any and all replies */
		while (got+failed < rrcnt && (rlen = recvfrom(fd, r, 512, 0,
			(void *)&sa, (socklen_t[1]){sl})) >= 2)
		{
			/* Ignore replies from addresses we didn't send to */
			for (i=0; i<nns; i++) if (!memcmp(ns+i, &sa, sl)) break;
			if (i==nns) continue;

			/* Compute index of the query from id */
			i = r[0]*256+r[1] - id & 0xffff;
			if ((unsigned)i >= rrcnt || !rr[i]) continue;

			/* Interpret the result code */
			switch (r[3] & 15) {
			case 0:
				got++;
				break;
			case 3:
				if (1) errcode = EAI_NONAME; else
			default:
				errcode = EAI_FAIL;
				failed++;
			}

			/* Mark this record as answered */
			rr[i] = 0;
			r += 512;
		}

		/* Check to see if we have answers to all queries */
		if (got+failed == rrcnt) break;
	}
	close(fd);

	/* Return the number of results, or an error code if none */
	if (got) return got;
	return errcode;
}

static void mkptr4(char *s, const unsigned char *ip)
{
	sprintf(s, "%d.%d.%d.%d.in-addr.arpa",
		ip[3], ip[2], ip[1], ip[0]);
}

static void mkptr6(char *s, const unsigned char *ip)
{
	static const char xdigits[] = "0123456789abcdef";
	int i;
	for (i=15; i>=0; i--) {
		*s++ = xdigits[ip[i]&15]; *s++ = '.';
		*s++ = xdigits[ip[i]>>4]; *s++ = '.';
	}
	strcpy(s, "ip6.arpa");
}

int __dns_query(unsigned char *r, const void *a, int family, int ptr)
{
	char buf[PTR_MAX];
	int rr[2], rrcnt = 1;

	if (ptr) {
		if (family == AF_INET6) mkptr6(buf, a);
		else mkptr4(buf, a);
		rr[0] = RR_PTR;
		a = buf;
	} else if (family == AF_INET6) {
		rr[0] = RR_AAAA;
	} else {
		rr[0] = RR_A;
		if (family != AF_INET) rr[rrcnt++] = RR_AAAA;
	}

	return __dns_doqueries(r, a, rr, rrcnt);
}


#define BITOP(a,b,op) \
 ((a)[(size_t)(b)/(8*sizeof *(a))] op (size_t)1<<((size_t)(b)%(8*sizeof *(a))))

static int decname(char *s, const unsigned char *b, const unsigned char *p)
{
	/* Remember jump destinations to detect loops and abort */
	size_t seen[PACKET_MAX/8/sizeof(size_t)] = { 0 };
	char *sz = s + HOST_NAME_MAX;
	const unsigned char *pz = b+512;
	for (;;) {
		if (p>=pz) return -1;
		else if (*p&0xc0) {
			int j = (p[0]&1) | p[1];
			if (BITOP(seen, j, &)) return -1;
			BITOP(seen, j, |=);
			p = b + j;
		} else if (*p) {
			if (p+*p+1>=pz || s+*p>=sz) return -1;
			memcpy(s, p+1, *p);
			s += *p+1;
			p += *p+1;
			s[-1] = *p ? '.' : 0;
		} else return 0;
	}
}

int __dns_get_rr(void *dest, size_t stride, size_t maxlen, size_t limit, const unsigned char *r, int rr, int dec)
{
	int qdcount, ancount;
	const unsigned char *p;
	char tmp[256];
	int found = 0;
	int len;

	if ((r[3]&15)) return 0;
	p = r+12;
	qdcount = r[4]*256 + r[5];
	ancount = r[6]*256 + r[7];
	if (qdcount+ancount > 64) return -1;
	while (qdcount--) {
		while (p-r < 512 && *p-1U < 127) p++;
		if (*p>193 || (*p==193 && p[1]>254) || p>r+506)
			return -1;
		p += 5 + !!*p;
	}
	while (ancount--) {
		while (p-r < 512 && *p-1U < 127) p++;
		if (*p>193 || (*p==193 && p[1]>254) || p>r+506)
			return -1;
		p += 1 + !!*p;
		len = p[8]*256 + p[9];
		if (p+len > r+512) return -1;
		if (p[1]==rr && len <= maxlen) {
			if (dec && decname(tmp, r, p+10)<0) return -1;
			if (dest && limit) {
				if (dec) strcpy(dest, tmp);
				else memcpy(dest, p+10, len);
				dest = (char *)dest + stride;
				limit--;
			}
			found++;
		}
		p += 10 + len;
	}
	return found;
}

int __dns_count_addrs(const unsigned char *r, int cnt)
{
	int found=0, res, i;
	static const int p[2][2] = { { 4, RR_A }, { 16, RR_AAAA } };

	while (cnt--) {
		for (i=0; i<2; i++) {
			res = __dns_get_rr(0, 0, p[i][0], -1, r, p[i][1], 0);
			if (res < 0) return res;
			found += res;
		}
		r += 512;
	}
	return found;
}
