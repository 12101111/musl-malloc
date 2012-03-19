/* origin: FreeBSD /usr/src/lib/msun/src/s_atanl.c */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
/*
 * See comments in atan.c.
 * Converted to long double by David Schultz <das@FreeBSD.ORG>.
 */

#include "libm.h"

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double atanl(long double x)
{
	return atan(x);
}
#elif (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
#include "__invtrigl.h"
static const long double huge = 1.0e300;

long double atanl(long double x)
{
	union IEEEl2bits u;
	long double w,s1,s2,z;
	int id;
	int16_t expsign, expt;
	int32_t expman;

	u.e = x;
	expsign = u.xbits.expsign;
	expt = expsign & 0x7fff;
	if (expt >= ATAN_CONST) { /* if |x| is large, atan(x)~=pi/2 */
		if (expt == BIAS + LDBL_MAX_EXP &&
		    ((u.bits.manh&~LDBL_NBIT)|u.bits.manl)!=0)  /* NaN */
			return x+x;
		if (expsign > 0)
			return  atanhi[3]+atanlo[3];
		else
			return -atanhi[3]-atanlo[3];
	}
	/* Extract the exponent and the first few bits of the mantissa. */
	/* XXX There should be a more convenient way to do this. */
	expman = (expt << 8) | ((u.bits.manh >> (MANH_SIZE - 9)) & 0xff);
	if (expman < ((BIAS - 2) << 8) + 0xc0) {  /* |x| < 0.4375 */
		if (expt < ATAN_LINEAR) {   /* if |x| is small, atanl(x)~=x */
			/* raise inexact */
			if (huge+x > 1.0)
				return x;
		}
		id = -1;
	} else {
		x = fabsl(x);
		if (expman < (BIAS << 8) + 0x30) {  /* |x| < 1.1875 */
			if (expman < ((BIAS - 1) << 8) + 0x60) { /*  7/16 <= |x| < 11/16 */
				id = 0;
				x = (2.0*x-1.0)/(2.0+x);
			} else {                                 /* 11/16 <= |x| < 19/16 */
				id = 1;
				x = (x-1.0)/(x+1.0);
			}
		} else {
			if (expman < ((BIAS + 1) << 8) + 0x38) { /* |x| < 2.4375 */
				id = 2;
				x = (x-1.5)/(1.0+1.5*x);
			} else {                                 /* 2.4375 <= |x| < 2^ATAN_CONST */
				id = 3;
				x = -1.0/x;
			}
		}
	}
	/* end of argument reduction */
	z = x*x;
	w = z*z;
	/* break sum aT[i]z**(i+1) into odd and even poly */
	s1 = z*T_even(w);
	s2 = w*T_odd(w);
	if (id < 0)
		return x - x*(s1+s2);
	z = atanhi[id] - ((x*(s1+s2) - atanlo[id]) - x);
	return expsign < 0 ? -z : z;
}
#endif
