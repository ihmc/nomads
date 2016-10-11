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
 * kernelconv.c
 *
 * Defines convolution operations on gray images
 *
 *----------------------------------------------
 */

#include "dvmkernel.h"

void
KernelApply(Kernel *kern, ByteImage *srcbuf, ByteImage *destbuf)
{
    int **normvals;
    unsigned char *src, *dest;
    int srcRowDif, destRowDif;
    int tmp;
    int *rowDif;
    int x, y, row, col;

    /*
     * Allocate and initialize normalized (fixed-point) kernel
     */
    normvals = NEWARRAY (int *, kern->height);
    for (row=0; row < kern->height; row++) {
        normvals[row] = NEWARRAY (int, kern->width);
        for (col=0; col < kern->width; col++) {
            normvals[row][col] = FIX((float) kern->vals[row][col] / kern->divfactor);
        }
    }

    /*
     * Set up auxiliary lookup array
     */
    rowDif = NEWARRAY (int, kern->height);
    for (rowDif[0]=0, row=1; row < kern->height; row++) {
        rowDif[row] = rowDif[row-1] + srcbuf->parentWidth;
    }

    src = srcbuf->firstByte;
    dest = destbuf->firstByte;

    srcRowDif = (kern->width - 1) + srcbuf->parentWidth - srcbuf->width;
    destRowDif = destbuf->parentWidth - destbuf->width;

    /*
     * Perform convolution.  Each destination pixel is calculated based on
     * multiplying each kernel value by the corresponding source pixel.
     */
    for (y=0; y< destbuf->height; y++) {
        for (x=0, tmp=0; x< destbuf->width; x++, dest++, src++) {

            for (row=0; row< kern->height; row++) {
                for (col=0; col< kern->width; col++) {
                    tmp += normvals[row][col] * (*(src + col + rowDif[row]));
                }
            }
            tmp = IUNFIX(tmp) + kern->offset;

            /*
             * Force values to valid range
             * Note: this could be more efficient with a lookup table,
             *       but is it worth that overhead?
             */

            if (tmp>=0 && tmp<=255) {
                *dest = (unsigned char) tmp;
            }
            else {
                if (tmp<0) {
                    *dest = (unsigned char) 0;
                }
                else if (tmp>255) {
                    *dest = (unsigned char) 255;
                }
            }
        }

        /*
         * Advance pointers to next row
         */
        src += srcRowDif;
        dest += destRowDif;
    }
}

void
KernelCompose(Kernel *src1, Kernel *src2, Kernel *dest)
{
    int row, col, row1, col1, row2, col2;
    int src2sum = 0;

    /*
     * Initialize dest values (necessary because of later use of +=)
     */
    for (row=0; row< dest->height; row++) {
        for (col=0; col< dest->width; col++) {
            dest->vals[row][col] = 0;
        }
    }

    /*
     * For each (src1,src2) pair of pixels, compute its contribution to dest
     */
    for (row1=0; row1<src1->height; row1++) {
        for (col1=0; col1<src1->width; col1++) {
            for (row2=0; row2<src2->height; row2++) {
                col2 = 0;
                DO_N_TIMES(src2->width,
                    dest->vals[row1+row2][col1+col2] +=
                        src1->vals[row1][col1] * src2->vals[row2][col2];
                    col2++;
                );
            }
        }
    }

    for (row2=0; row2<src2->height; row2++) {
        for (col2=0; col2<src2->width; col2++) {
            src2sum += src2->vals[row2][col2];
        }
    }

    dest->divfactor = src1->divfactor * src2->divfactor;
    dest->offset = src2sum * src1->offset / src2->divfactor + src2->offset;
}
