/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "visionInt.h"

static float *CreateMask(float s);
static void FreeMask(float *mask);

#ifndef M_PI
#define M_PI 3.141592654
#endif

int ByteSmoothGaussian (ByteImage *img, ByteImage *smooth, float sigma )
{
    float *mask;
    float *scratch, *tbuf, *coltbuf; /* For temporary storage of the float image */
    int w, h, size, stride, i, j, z;
    unsigned char *row, *out, *colout;
    float tmp;

    w = img->width;
    h = img->height;

    if ((w != smooth->width) ||
        (h != smooth->height)) {
        return DVM_DIFFERENT_SIZES;
    }

    scratch = (float *)MALLOC(w*h*sizeof(float));
    if (scratch == NULL) {
        return DVM_ALLOC_ERROR;
    }
    mask = CreateMask(sigma);
    size = (int) ceil(sigma*4);


    /* First, the horizontal convolution */
    stride = img->parentWidth - w;
    row = img->firstByte;
    tbuf = scratch;
    for (i=0; i<h; i++) {

        /* First elements */
        for (j=0; j<size; j++) {
            tmp = (*row)*mask[0];
            for (z=1; z<=j; z++)
                tmp += (*(row-z) + *(row+z))*mask[z];
            for (z=j+1; z<=size; z++)
                tmp += (*(row-j) + *(row+z))*mask[z];
            *tbuf++ = tmp;
            row++;
        }


        /* Middle elements */
        for (j=0; j<(w-2*size);j++) {
            tmp = (*row)*mask[0];
            for (z = 1; z <= size; z++) 
                tmp += (*(row-z) + *(row+z))*mask[z];
            *tbuf++ = tmp;
            row++;
        }

        /* Last elements */
        for (j=size-1; j>=0; j--) {
            tmp = (*row)*mask[0];
            for (z=1; z<=j; z++)
                tmp += (*(row-z) + *(row+z))*mask[z];
            for (z=j+1; z<=size; z++)
                tmp += (*(row-z) + *(row+j))*mask[z];
            *tbuf++ = tmp;
            row++;
        }

        row += stride;
    }

    /* And the vertical convolution */
    stride = smooth->parentWidth;
    colout = smooth->firstByte;
    coltbuf  = scratch;
    for (i=0; i<w; i++) {

        out = colout++;
        tbuf = coltbuf++;

        /* First elements */
        for (j=0; j<size; j++) {
            tmp = (*tbuf)*mask[0];
            for (z=1; z<=j; z++)
                tmp += (*(tbuf-w*z) + *(tbuf+w*z))*mask[z];
            for (z=j+1; z<=size; z++)
                tmp += (*(tbuf-w*j) + *(tbuf+w*z))*mask[z];
            *out = (int)tmp;
            tbuf += w;
            out += stride;
        }


        /* Middle elements */
        for (j=0; j<(h-2*size);j++) {
            tmp = (*tbuf)*mask[0];
            for (z = 1; z <= size; z++) 
                tmp += (*(tbuf-w*z) + *(tbuf+w*z))*mask[z];
            *out = (int)tmp;
            tbuf += w;
            out += stride;
        }

        /* Last elements */
        for (j=size-1; j>=0; j--) {
            tmp = (*tbuf)*mask[0];
            for (z=1; z<=j; z++)
                tmp += (*(tbuf-w*z) + *(tbuf+w*z))*mask[z];
            for (z=j+1; z<=size; z++)
                tmp += (*(tbuf-w*z) + *(tbuf+w*j))*mask[z];

            *out = (int)tmp;
            tbuf += w;
            out += stride;
        }
    }

    free(scratch);
    FreeMask(mask);
    return DVM_VISION_OK;
}


float *CreateMask(float s)
{
    float *gmask, sum;
    int i, len;

    if (s < 0.0)
        s = 1.0;

    len  = (int) ceil(s*4) + 1;
    gmask = (float *)MALLOC(len*sizeof(float));

    /* Make the mask have the gaussian values */
    sum = 0.0;
    for (i = 0; i < len; i++) {
        gmask[i] = (float)((exp((-0.5*i*i)/(s*s)))/(s*sqrt(M_PI*2)));
        sum += (float)(2.0*gmask[i]);
    }
    /* we have counted the central term twice */
    sum -= gmask[0];

    /* Now normalize it */
    for (i = 0; i < len; i++)
        gmask[i] = gmask[i]/sum;
    
  return gmask;
}

void FreeMask(float *mask)
{
    free(mask);
}
