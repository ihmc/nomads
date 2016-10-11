/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _MACRO_H
#define _MACRO_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef MEM_DEBUG
    extern char *DebugMalloc(unsigned int size, char *file, int line);
    extern char *DebugRealloc(char *ptr, unsigned int size, char *file, int line);
    extern void DebugFree(char *ptr, char *file, int line);
    extern void DebugCheck(void);
#define MALLOC(s) DebugMalloc((s),__FILE__, __LINE__)
#define NEW(t) (t *)DebugMalloc(sizeof(t),__FILE__, __LINE__)
#define CHECKMEM() DebugCheck()
#define NEWARRAY(t, n) (t *)DebugMalloc(n*sizeof(t),__FILE__, __LINE__)
#define FREE(p) DebugFree((char *)(p),__FILE__, __LINE__)
#define REALLOC(p,s) DebugRealloc((char *)(p), (s),__FILE__, __LINE__)
#else
#define MALLOC(s) malloc((s))
#define NEW(t) (t *)malloc(sizeof(t))
#define CHECKMEM()
#define NEWARRAY(t, n) (t *)malloc(n*sizeof(t))
#define FREE(p) free((char *)(p))
#define REALLOC(p,s) realloc((char *)(p), (size_t)(s))
#endif

#define ROUNDDOWN(i, f) {i = (int)f; if (f < 0 && f != (float)i) i-= 1;}

/*
 *--------------------------------------------------------------------------
 * Execute _code_ _times_ number of times, using Duff Device.
 * Warning : Will not work correctly if times is 0 !
 *--------------------------------------------------------------------------
 */
#define DO_N_TIMES(times, code) {\
    register int _n; \
    _n = ((times) + 7)/8;\
    switch((times)%8) {\
        case 0 : do { code \
        case 7 : code\
        case 6 : code\
        case 5 : code\
        case 4 : code\
        case 3 : code\
        case 2 : code\
        case 1 : code\
     } while (--_n > 0);}}


/*
 * Fixed point routines
 * Brian Smith, June 97
 *
 * Converts floating point to/from fixed point.  The constants are
 * chosen to allow values in the range -255..255 to be safely
 * represented with these routines.  A fixed point number is a floating
 * point number times a constant, rounded to the nearest integer.
 * In other words, if F is a fixed point number and f is a floating
 * point number, then F=f*k always.  The advantage of this representation
 * is that arithmetic can be done using only integer routines.  Two
 * fixed point values can be added to produce a fixed point value:
 *      F = F1 + F2 = f1*k + f2*k = k*(f1 + f2) = FIX(f1+f2)
 *
 * When an integer is used to multiply or divide a fixed point,
 * the result is a fixed point representation of the product:
 *      F = F1*i = f*k*i = k*(f*i) = FIX(f*i)
 *
 *     FIX(float)   -> returns fixed point version of float
 *     IFIX(int)   -> returns fixed point version of int
 *     UNFIX(fixed) -> returns floating points version of fixed
 *     IUNFIX(fixed) -> returns truncated (integer) version of fixed
 *     IUNFIX_ROUND(fixed) -> returns rounded (integer) version of fixed
 */
#define FIX_PLACES       14
#define FIX_CONST        (1<<FIX_PLACES)
#define FIX_HALF         (1<<(FIX_PLACES-1))

#define FIX(f)          ((int)((f)*FIX_CONST))
#define IFIX(i)         ((i)<<FIX_PLACES)
#define UNFIX(x)        ((double)((x)/(double)FIX_CONST))
#define IUNFIX(x)       ((x)>>FIX_PLACES)
#define IUNFIX_ROUND(x) (((x)+FIX_HALF)>>FIX_PLACES)

#define FIXMUL(x,y) ((x) * (y) / FIX_CONST)
#define FIXDIV(x,y) ((x) / (y) * FIX_CONST)

/*
 * the usual min and max operations..
 */

#ifndef min
#define min(a,b)        ((a > b) ? b : a)
#endif
#ifndef max
#define max(a,b)        ((a < b) ? b : a)
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*
 * For SunOS4 or other platform that does not define these constant.
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

/*
 * Debugging aid, and error checking routines.
 * Wei Tsang August 97
 * 
 * If you want to turn off error checking totally to 
 * speed up execution,  compiled with NO_ERROR_CHECKING.  
 * The default is to check for errors and output the error 
 * messages as Tcl messages.
 *
 * RULE 1 : Must use this inside a Tcl command function
 * RULE 2 : The Tcl_Interp must be called "interp"
 * RULE 3 : Until most of the code is fixed, you can put
 *          execution in "cond" of ReturnErrorIf, but nowhere else !
 * If you need something that is no provided here, add you own..
 *
 */

#define ReturnErrorIf(cond) {\
       if (cond) {\
           return TCL_ERROR;\
       }}

#ifdef NO_ERROR_CHECKING

#define ReturnErrorIf0(cond, msg) { }
#define ReturnErrorIf1(cond, msg, arg1) { }
#define ReturnErrorIf2(cond, msg, arg1, arg2) { }
#define ReturnErrorIf3(cond, msg, arg1, arg2, arg3) { }
#define ReturnErrorIf4(cond, msg, arg1, arg2, arg3, arg4) { }
#else
#define ReturnErrorIf0(cond, msg) {\
       if (cond) {\
           sprintf(interp->result, msg);\
           return TCL_ERROR;\
       }}
#define ReturnErrorIf1(cond, msg, arg1) {\
       if (cond) {\
           sprintf(interp->result, msg, arg1);\
           return TCL_ERROR;\
       }}
#define ReturnErrorIf2(cond, msg, arg1, arg2) {\
       if (cond) {\
           sprintf(interp->result, msg, arg1, arg2);\
           return TCL_ERROR;\
       }}
#define ReturnErrorIf3(cond, msg, arg1, arg2, arg3) {\
       if (cond) {\
           sprintf(interp->result, msg, arg1, arg2, arg3);\
           return TCL_ERROR;\
       }}
#define ReturnErrorIf4(cond, msg, arg1, arg2, arg3, arg4) {\
       if (cond) {\
           sprintf(interp->result, msg, arg1, arg2, arg3, arg4);\
           return TCL_ERROR;\
       }}
#endif

/*
 * Take care of some system without memmove. (but with bcopy instead).
 */
#ifdef USE_BCOPY
#include <strings.h>
#define memmove(d, s, l) bcopy(s, d, l)
#endif
#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
