/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/*
 *----------------------------------------------------------------------
 * tclDvmImap.h                                                             
 *                                                                              
 *      contains the function prototypes for the TCL command interface 
 *      and the hash table access macros.                                               
 *                                                                        
 *----------------------------------------------------------------------
 */

#ifndef _TCL_DVM_IMAGE_MAP_H_
#define _TCL_DVM_IMAGE_MAP_H_

#include "tclDvmBasic.h"
#include "dvmimap.h"
#ifdef __cplusplus
extern "C" {
#endif

/* all functions contained in imapcmd.c */
    int ImageMapNewCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapInitCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapFreeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapCopyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapGetValuesCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapApplyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapInitHistoEqualCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapInitIdentityCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int ImageMapInitInverseCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

#define IMAGE_MAP_PREFIX "dvmImgMap"
#define GetImageMap(s) (!strncmp(s,IMAGE_MAP_PREFIX,9)? GetBuf(s):NULL)
#define RemoveImageMap(s) (!strncmp(s,IMAGE_MAP_PREFIX,9)? RemoveBuf(s):NULL)
#define PutImageMap(interp, buf) PutBuf(interp, IMAGE_MAP_PREFIX, buf)

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
