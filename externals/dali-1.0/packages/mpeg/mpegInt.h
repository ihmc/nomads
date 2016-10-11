/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "dvmbasic.h"
#include "dvmmpeg.h"
#include "bitparser.h"

 /*
    * General Error constant
  */
#define ERROR   (-1)
#define MY_ASSERT(cond, str)

/*

   uncomment this to turn on assert for debugging

   #define MY_ASSERT(cond, str) {\
   if (!(cond)) fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, str);\
   }
 */

/*
 * special Macroblock coeff
 */
#define MACRO_BLOCK_STUFFING 34
#define MACRO_BLOCK_ESCAPE 35
/* 
 * Interface to jrevdct.c routines (from JPEG decoder)
 */

#define DCTELEM short
#define DCTSIZE 8
#define DCTSIZE2 64
typedef DCTELEM DCTBLOCK[DCTSIZE2];

typedef struct AllocTableEntry {
    unsigned int steps;
    unsigned int bits;
    unsigned int group;
    unsigned int quant;
} AllocTableEntry;

typedef AllocTableEntry AllocTable[32][16];
typedef AllocTable *AllocTablePtr;

void SubBandSynthesis(MpegAudioSynData * v, double *fraction, short **pcm);
int NextStartCode(BitParser *, unsigned int *off);
int DumpUntilNextStartCode(BitParser *, BitParser *, unsigned int *off);
void ReadTimeStamp(BitParser *, double *);
void WriteTimeStamp(char, double, BitParser *);
