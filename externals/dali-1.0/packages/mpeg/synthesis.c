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
 *
 * mpegaudio.c
 *
 * Functions that manipulate Mpeg Audio 
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"
#include "dvmmpeg.h"
#include "audiotables.h"

extern double filter_table[64][32];

// extern double decode_window[512];
extern double decode_window[512];

void 
SubBandSynthesis(syndata, fraction, pcm)
    MpegAudioSynData *syndata;
    double *fraction;
    short **pcm;
{
    register int i, j;

    /*register long *curr_f; */
    register double *curr_f;
    register double *curr_fraction;
    register double *curr_filter;
    register double *curr_v;
    register double sum;
    register int offset;
    register double *v;
    int longpcm;

    syndata->offset = (syndata->offset - 64) & 0x3ff;
    offset = syndata->offset;
    v = syndata->syn;

    // end_v =  v + 1023;
    // curr_v = v + 959;
    /*memmove(end_v, curr_v, sizeof(double)*960); */

    // DO_N_TIMES(960,
    //  *end_v-- = *curr_v--;
    // );

    // sub band synthesis

    curr_filter = &(filter_table[0][0]);
    curr_v = &v[offset];

    for (i = 0; i < 64; i++) {
        double sumf = 0;

        curr_fraction = fraction;
        DO_N_TIMES(32,
            sumf += (*curr_fraction++) * (*curr_filter++);
            );
        *curr_v++ = sumf;
    }

    // calculate samples.
    // here is the mapping :
    // if  x % 64 <  32 : w[x] = v[(x/64)*128 + (x%64)] * decode_window[x]
    // if  x % 64 >= 32 : w[x] = v[(x/64)*128 + (x%64) + 96] * decode_window[x]

    for (j = 0; j < 32; j++) {

        // for (i = 0; i < 16; i++) {
        //   sum += decode_window[k]* v[k + (((i+1)>>1) << 6)];
        //   k += 32;
        // }
        // decode_window is the window given in the standard *4096*32768
        //
        curr_f = &(decode_window[j]);
        sum = *(curr_f) * v[(j + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 96 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 128 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 224 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 256 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 352 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 384 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 480 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 512 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 608 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 640 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 736 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 768 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 864 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 896 + offset) & 0x3ff];
        curr_f += 32;
        sum += *(curr_f) * v[(j + 992 + offset) & 0x3ff];
        /*
           if (sum > 134217728) // 4096*32768
           **pcm = 32767;
           else if (sum < -134217728)
           **pcm = -32768;
           else 
           **pcm = ((long)sum)/4096 ; // div by 4096
         */
        longpcm = (int) (sum * 32768);
        if (longpcm >= 32768)
            **pcm = 32767;
        else if (longpcm < -32768)
            **pcm = -32768;
        else
            **pcm = (short) longpcm;
        /*fprintf(stderr, "%d\n", **pcm); */
        (*pcm)++;
    }
}
