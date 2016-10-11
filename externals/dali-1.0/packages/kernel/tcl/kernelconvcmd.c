/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/* Documentation updated 10/10/98 by Jiesang */

/*
 *----------------------------------------------
 *
 * kernelconv.c
 *
 * Defines convolution operations on gray images
 *
 *----------------------------------------------
 */

#include "tclDvmKernel.h"

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_apply <kern> <srcbuf> <destbuf>
 *
 * precond
 *     Kernel _kern_ and buffers _srcbuf_ and _destbuf_ exist and
 *       destbuf->height == srcbuf->height - (kern->height - 1)
 *       destbuf->width == srcbuf->width - (kern->width - 1)
 *
 *
 * return
 *     None
 * 
 * side effect
 *     the values of _destbuf_ reflect the convolution of
 *     _srcbuf_ with kernel _kern_
 *
 *----------------------------------------------------------------------
 */

int
KernelApplyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;
    ByteImage *srcbuf, *destbuf;

    /*
     * Check args, retrieve kernels from table
     */

    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s kern srcbuf destbuf", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[1]);

    srcbuf = GetByteImage(argv[2]);
    if (srcbuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s", argv[0], argv[2]);
        return TCL_ERROR;
    }

    destbuf = GetByteImage(argv[3]);
    if (destbuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s", argv[0], argv[3]);
        return TCL_ERROR;
    }

    KernelApply(kern,srcbuf,destbuf); 
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_compose <src1> <src2> <dest>
 *
 * precond
 *     Kernels _src1_, _src2_ and _dest_ exist and
 *       dest->height == src1->height + src2->height -1
 *       dest->width == src1->width + src2->width -1
 *
 *
 * return
 *     None
 * 
 * side effect
 *     the values of _dest_ reflect the composition of _src1_ and _src2_
 *
 *----------------------------------------------------------------------
 */

int
KernelComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *src1, *src2, *dest;

    /*
     * Check args, retrieve kernels from table
     */

    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s src1 src2 dest", argv[0]);

    src1 = GetKernel(argv[1]);
    ReturnErrorIf2 (src1 == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[1]);

    src2 = GetKernel(argv[2]);
    ReturnErrorIf2 (src2 == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[2]);

    dest = GetKernel(argv[3]);
    ReturnErrorIf2 (dest == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[3]);

    KernelCompose(src1,src2,dest);
    return TCL_OK;
}
