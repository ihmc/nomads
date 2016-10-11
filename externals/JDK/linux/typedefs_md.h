/*
 * @(#)typedefs_md.h	1.33 98/07/01
 *
 * Copyright 1995-1998 by Sun Microsystems, Inc.,
 * 901 San Antonio Road, Palo Alto, California, 94303, U.S.A.
 * All rights reserved.
 * 
 * This software is the confidential and proprietary information
 * of Sun Microsystems, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Sun.
 */

/*
 * Solaris-dependent types for Green threads
 */

/* sbb: Johan Vos, why isn't this #ifndef inside the solaris header guard? */
#ifndef BITSPERCHAR
#define BITSPERCHAR 8
#endif

#ifndef _SOLARIS_TYPES_MD_H_
#define _SOLARIS_TYPES_MD_H_

#include <sys/types.h>
#include <sys/stat.h>
#include "bool.h"

#if defined(__alpha__)
typedef unsigned long ptr_int;
#define PTR_IS_64 1
#define LONG_IS_64 1
#else
typedef unsigned int ptr_int;
#define PTR_IS_32 1
#endif

/* don't redefine typedef's on Solaris 2.6 or Later */

#if !defined(_ILP32) && !defined(_LP64)

#ifndef	_UINT64_T
#define	_UINT64_T
#ifdef LONG_IS_64
typedef unsigned long uint64_t;
#else
typedef unsigned long long uint64_t;
#endif
#define _UINT32_T
typedef unsigned int uint32_t;
#if defined(__linux__)
typedef unsigned int uint_t;
#endif
#endif

#ifndef __BIT_TYPES_DEFINED__
#if defined(__i386__)
/* that should get Linux, at least */
#ifndef	_INT64_T
#define	_INT64_T
#ifdef LONG_IS_64
typedef long int64_t;
#else
typedef long long int64_t;
#endif
#define _INT32_T
typedef int int32_t;
#if defined(__linux__)
typdef int int_t;
#endif
#endif
#endif /* i386 */
#endif /* __BIT_TYPES_DEFINED__ */

#endif	/* !defined(_ILP32) && !defined(_LP64) */

#ifndef BITSPERCHAR
#define BITSPERCHAR 8
#endif

/* use these macros when the compiler supports the long long type */

#define ll_high(a)	((int)((a)>>32))
#define ll_low(a)	((int)(a))
#define int2ll(a)	((int64_t)(a))
#define ll2int(a)	((int)(a))
#define ll_add(a, b)	((a) + (b))
#define ll_and(a, b)	((a) & (b))
#define ll_div(a, b)	((a) / (b))
#define ll_mul(a, b)	((a) * (b))
#define ll_neg(a)	(-(a))
#define ll_not(a)	(~(a))
#define ll_or(a, b)	((a) | (b))
#define ll_shl(a, n)	((a) << (n))
#define ll_shr(a, n)	((a) >> (n))
#define ll_sub(a, b)	((a) - (b))
#define ll_ushr(a, n)	((uint64_t)(a) >> (n))
#define ll_xor(a, b)	((a) ^ (b))
#define uint2ll(a)	((uint64_t)(unsigned int)(a))
#define ll_rem(a,b)	((a) % (b))

#define INT_OP(x,op,y)  ((x) op (y))
#define NAN_CHECK(l,r,x) x
#if defined(__solaris__)
#define IS_NAN(x) isnand(x)
#else
#define IS_NAN(x) isnan(x)
#endif


/* On Intel these conversions have to be method calls and not typecasts.
   See the win32 typedefs_md.h file */
#if ((defined(i386) || defined (__i386)) && defined(__solaris__)) || defined(__powerpc__)

extern int32_t float2l(float f);
extern int32_t double2l(double d);
extern int64_t float2ll(float f);
extern int64_t double2ll(double d);

#else /* not solaris x386 or linux powerpc*/

#define float2l(f)	(f)
#define double2l(f)	(f)
#define float2ll(f)	((int64_t) (f))
#define double2ll(f)	((int64_t) (f))

#endif /* i386 */


#define ll2float(a)	((float) (a))
#define ll2double(a)	((double) (a))

/* comparison operators */
#define ll_ltz(ll)	((ll)<0)
#define ll_gez(ll)	((ll)>=0)
#define ll_eqz(a)	((a) == 0)
#define ll_eq(a, b)	((a) == (b))
#define ll_ne(a,b)	((a) != (b))
#define ll_ge(a,b)	((a) >= (b))
#define ll_le(a,b)	((a) <= (b))
#define ll_lt(a,b)	((a) < (b))
#define ll_gt(a,b)	((a) > (b))

#define ll_zero_const	((int64_t) 0)
#define ll_one_const	((int64_t) 1)

extern void ll2str(int64_t a, char *s, char *limit);

#if defined(ppc) || defined(__ppc__) || defined(__alpha__) || defined(__sparc__)
#ifndef HAVE_ALIGNED_DOUBLES
#define HAVE_ALIGNED_DOUBLES
#endif
#ifndef HAVE_ALIGNED_LONGLONGS
#define HAVE_ALIGNED_LONGLONGS
#endif
#endif

#endif /* !_SOLARIS_TYPES_MD_H_ */
