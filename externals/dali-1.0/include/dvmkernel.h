/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _DVM_KERNEL_
#define _DVM_KERNEL_

#include "dvmbasic.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *----------------------------------------------------------------------
 *
 * type Kernel
 *
 *----------------------------------------------------------------------
 */

    typedef struct Kernel {
        int width;
        int height;
        int **vals;
        int divfactor;
        int offset;
    } Kernel;

    Kernel *KernelNew(int width, int height);
    void KernelFree(Kernel * kern);
    void KernelApply(Kernel * kern, ByteImage * srcbuf, ByteImage * destbuf);
    void KernelCompose(Kernel * src1, Kernel * src2, Kernel * dest);
    void KernelSetValues(Kernel * k, int *table);

/*
 *----------------------------------------------------------------------
 *Kernel Query Macros
 *----------------------------------------------------------------------
 */

#define KernelGetWidth(kern) kern->width
#define KernelGetHeight(kern) kern->height
#define KernelGetDivFactor(kern) kern->divfactor
#define KernelGetOffset(kern) kern->offset

#define KernelSetDivFactor(kern, x) (kern)->divfactor = x
#define KernelSetOffset(kern, x) (kern)->offset = x

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
