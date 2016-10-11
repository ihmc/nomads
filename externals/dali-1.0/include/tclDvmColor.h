/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef TCL_DVM_COLOR_H
#define TCL_DVM_COLOR_H

#include "tclDvmBasic.h"
#include "tclDvmImap.h"
#include "dvmcolor.h"

#ifdef __cplusplus
extern "C" {
#endif
    int ColorHashTableNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int ColorHashTableClearCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int ColorHashTableFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int ColorHashTableGetSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int ColorHashTableGetNumOfEntryCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int RgbToYuv420Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int RgbToYuv411Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int RgbToYuv422Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int RgbToYuv444Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int RgbToYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int YuvTo1Rgb420Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int YuvToRgb420Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int YuvToRgb411Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int YuvToRgb422Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int YuvToRgb444Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int RgbTo256Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int RgbQuantWithHashTableCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int RgbQuantWithVpTreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int VpTreeNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int VpTreeFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int VpTreeInitCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

#define COLOR_HASH_TABLE_PREFIX "dvmClrHTb"
#define FindColorHashTable(s) (!strncmp(s, COLOR_HASH_TABLE_PREFIX, 9)? FindBuf(s) : NULL)
#define GetColorHashTable(s) (!strncmp(s, COLOR_HASH_TABLE_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveColorHashTable(s) (!strncmp(s, COLOR_HASH_TABLE_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutColorHashTable(interp, buf) PutBuf(interp, COLOR_HASH_TABLE_PREFIX, buf)

#define VP_TREE_PREFIX "dvmVpTree"
#define FindVpTree(s) (!strncmp(s, VP_TREE_PREFIX, 9)? FindBuf(s) : NULL)
#define GetVpTree(s) (!strncmp(s, VP_TREE_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveVpTree(s) (!strncmp(s, VP_TREE_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutVpTree(interp, buf) PutBuf(interp, VP_TREE_PREFIX, buf)
#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
