/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef TCL_DVM_KERNEL_
#define TCL_DVM_KERNEL_

#include "tclDvmBasic.h"
#include "dvmkernel.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Allocate a new ConvKernel (kernelcmd.c) */
    int KernelNewCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelFreeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelSetValuesCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelSetDivFactorCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelSetOffsetCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelGetHeightCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelGetWidthCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelGetValuesCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelGetDivFactorCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelGetOffsetCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelApplyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int KernelComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

#define CONV_KERN_PREFIX "dvmKernel"

#define GetKernel(s) (!strncmp(s,CONV_KERN_PREFIX,9)? GetBuf(s):NULL)
#define RemoveKernel(s) (!strncmp(s,CONV_KERN_PREFIX,9)? RemoveBuf(s):NULL)
#define PutKernel(interp, buf) PutBuf(interp, CONV_KERN_PREFIX, buf)
#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
