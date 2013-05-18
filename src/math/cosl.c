/* origin: FreeBSD /usr/src/lib/msun/src/s_cosl.c */
/*-
 * Copyright (c) 2007 Steven G. Kargl
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Limited testing on pseudorandom numbers drawn within [-2e8:4e8] shows
 * an accuracy of <= 0.7412 ULP.
 */

#include "libm.h"

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double cosl(long double x) {
	return cos(x);
}
#elif (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
long double cosl(long double x)
{
	union IEEEl2bits z;
	unsigned n;
	long double y[2];
	long double hi, lo;

	z.e = x;
	z.bits.sign = 0;

	/* If x = NaN or Inf, then cos(x) = NaN. */
	if (z.bits.exp == 0x7fff)
		return (x - x) / (x - x);

	/* |x| < (double)pi/4 */
	if (z.e < M_PI_4) {
		/* |x| < 0x1p-64 */
		if (z.bits.exp < 0x3fff - 64)
			/* raise inexact if x!=0 */
			return 1.0 + x;
		return __cosl(z.e, 0);
	}

	n = __rem_pio2l(x, y);
	hi = y[0];
	lo = y[1];
	switch (n & 3) {
	case 0:
		hi = __cosl(hi, lo);
		break;
	case 1:
		hi = -__sinl(hi, lo, 1);
		break;
	case 2:
		hi = -__cosl(hi, lo);
		break;
	case 3:
		hi = __sinl(hi, lo, 1);
		break;
	}
	return hi;
}
#endif
