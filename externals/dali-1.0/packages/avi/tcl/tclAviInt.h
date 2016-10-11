/*
 *----------------------------------------------------------------------
 *
 * tclAviInt.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _TCL_AVI_INT_
#define _TCL_AVI_INT_

#include <tclDvmAvi.h>

/* Clientdata defs for the field info commands */

#define AVISHDR_WIDTH      1 
#define AVISHDR_HEIGHT     2
#define AVISHDR_FPS        3
#define AVISHDR_LENGTH     4
#define AVISHDR_START      5
#define AVISHDR_TYPE       6
#define AVISHDR_COMP       7

#define AVIHDR_LENGTH      1
#define AVIHDR_NUMSTREAMS  2

#define AVIFRAME_SKIP      1
#define AVIFRAME_TELL      2
#define AVIFRAME_REWIND    3

#define AVIPARM_QUALITY    1
#define AVIPARM_KEYINT     2
#define AVIPARM_BITRATE    3

#endif
