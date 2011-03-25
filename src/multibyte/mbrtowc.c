/* 
 * This code was written by Rich Felker in 2010; no copyright is claimed.
 * This code is in the public domain. Attribution is appreciated but
 * unnecessary.
 */

#include <stdlib.h>
#include <inttypes.h>
#include <wchar.h>
#include <errno.h>

#include "internal.h"

size_t mbrtowc(wchar_t *wc, const char *src, size_t n, mbstate_t *st)
{
	static unsigned internal_state;
	unsigned c;
	const unsigned char *s = (const void *)src;
	const unsigned N = n;

	if (!st) st = (void *)&internal_state;
	c = *(unsigned *)st;
	
	if (!s) {
		s = (void *)"";
		wc = (void *)&wc;
		n = 1;
	} else if (!wc) wc = (void *)&wc;

	if (!n) return -2;
	if (!c) {
		if (*s < 0x80) return !!(*wc = *s);
		if (*s-SA > SB-SA) goto ilseq;
		c = bittab[*s++-SA]; n--;
	}

	if (n) {
		if (OOB(c,*s)) goto ilseq;
loop:
		c = c<<6 | *s++-0x80; n--;
		if (!(c&(1U<<31))) {
			*(unsigned *)st = 0;
			*wc = c;
			return N-n;
		}
		if (n) {
			if (*s-0x80u >= 0x40) goto ilseq;
			goto loop;
		}
	}

	*(unsigned *)st = c;
	return -2;
ilseq:
	*(unsigned *)st = FAILSTATE;
	errno = EILSEQ;
	return -1;
}
