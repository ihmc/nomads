/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _TCL_DVM_WAVE_H_
#define _TCL_DVM_WAVE_H_

#include "tclDvmBasic.h"
#include "dvmbytegeom.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *---------------------------------------------------------------------
 *
 * This is the main Tcl interface header file for BYTEGEOM package.
 * It contains functions for common geometric operations on byte images
 *
 *---------------------------------------------------------------------
 */

/* scaling functions (ByteScaleCmd.c) */
    int ByteShrink4x4Cmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteShrink2x2Cmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteShrink1x2Cmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteShrink2x1Cmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteShrinkIntCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteExpand4x4Cmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteExpand2x2Cmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteExpand1x2Cmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteExpand2x1Cmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteExpandIntCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteScaleBilinearCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* Rotation functions (ByteRotateCmd.c) */
    int ByteRotateOrigCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteRotateCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteRotate90aCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteRotate90cCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* General affine transform (ByteAffineCmd.c) */
    int ByteAffineCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteAffineRectRegionCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* General Homogeneous transform (ByteHomoCmd.c) */
    int ByteHomoComputeMatrixCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteHomoInvertMatrixCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteHomoCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ByteHomoRectRegionCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
