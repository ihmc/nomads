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
 *----------------------------------------------
 *
 * kernel.c
 *
 * Defines allocation and initialization of
 * convolution kernels.
 *
 *----------------------------------------------
 */

#include "dvmkernel.h"

Kernel *
KernelNew(int width, int height)
{
    Kernel *kern;
    int row, col;

    /*
     * Allocate kernel structure and initialize size fields
     */
    kern = NEW (Kernel);
    kern->width = width;
    kern->height = height;
    kern->divfactor = 1;
    kern->offset = 0;

    /*
     * allocate pointers to rows,
     * as well as individual rows
     */
    kern->vals = NEWARRAY (int *, kern->height);
    for (row=0; row < kern->height; row++) {
        kern->vals[row] = NEWARRAY (int, kern->width);
        col = 0;
        DO_N_TIMES(kern->width,
            kern->vals[row][col++] = 0;
        );
    }
    return kern;
}


void
KernelFree(Kernel *kern)
{
    int row = 0;

    /*
     * Free memory associated with this kernel
     */
    DO_N_TIMES(kern->height,
        FREE(kern->vals[row++]);
    );
    FREE(kern->vals);
    FREE(kern);
}

/*
 * KernelSet and KernelGetValues has no parallel in the C domain, 
 * since the values can be directly set and read in C
 * KernelGetHeight, KernelGetWidth, KernelGetOffset, KernelGetDivFactor are 
 * macros defined in dvmkernel.h
 */

void KernelSetValues(k, table)
    Kernel *k;
    int *table;
{
    int i, j;
    for (i = 0; i < k->height; i++) {
        for (j = 0; j < k->width; j++) {
            k->vals[i][j] = table[i*k->width + j];
        }
    }
}
