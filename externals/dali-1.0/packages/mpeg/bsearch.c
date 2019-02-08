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
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */


#include "mpegInt.h"

static void
BMotionSearchNoInterp(currentBlock, prev, next, interPrev, interNext, fStepSize, fSearchRange, bStepSize, bSearchRange,
    by, bx, height, width, fmy, fmx, forwardErr, bmy, bmx, backwardErr)
    LumBlock currentBlock;
    ByteImage *prev, *next;
    ByteImage **interPrev, **interNext;
    int fStepSize, fSearchRange;
    int bStepSize, bSearchRange;
    int by, bx;
    int height, width;
    int *fmx, *fmy;
    int *forwardErr;
    int *bmx, *bmy;
    int *backwardErr;
{
    *forwardErr = PLogarithmicSearch(currentBlock, prev, interPrev, fStepSize, fSearchRange, by, bx, height, width, fmy, fmx);
    *backwardErr = PLogarithmicSearch(currentBlock, next, interNext, bStepSize, bSearchRange, by, bx, height, width, bmy, bmx);
}

/*===========================================================================*
 *
 * BMotionSearchSimple
 *
 *      does a simple search for B-frame motion vectors
 *      see BMotionSearch for generic description
 *
 * DESCRIPTION:
 *      1)  find best backward and forward vectors
 *      2)  compute interpolated error using those two vectors
 *      3)  return the best of the three choices
 *
 *===========================================================================*/
static int
BMotionSearchSimple(currentBlock, prev, next, interPrev, interNext, fStepSize, fSearchRange,
    bStepSize, bSearchRange, by, bx, height, width, fmy, fmx, bmy, bmx, oldMode)
    LumBlock currentBlock;
    ByteImage *prev, *next;
    ByteImage **interPrev, **interNext;
    int fStepSize, fSearchRange;
    int bStepSize, bSearchRange;
    int by, bx;
    int height, width;
    int *fmx, *fmy;
    int *bmx, *bmy;
    int oldMode;
{
    long forwardErr, backwardErr, interpErr, bestSoFar;
    LumBlock interpBlock;

    /* STEP 1 */
    BMotionSearchNoInterp(currentBlock, prev, next, interPrev, interNext, fStepSize, fSearchRange, bStepSize, bSearchRange,
        by, bx, height, width, fmy, fmx, &forwardErr, bmy, bmx, &backwardErr);

    /* STEP 2 */
    ComputeBMotionLumBlock(prev, next, interPrev, interNext, bx, by, *fmx, *fmy, *bmx, *bmy, MOTION_INTERPOLATE, interpBlock);
    bestSoFar = min(backwardErr, forwardErr);
    interpErr = LumBlockMAD(currentBlock, interpBlock, bestSoFar);

    /* STEP 3 */
    if (interpErr <= forwardErr) {
        if (interpErr <= backwardErr) {
            return MOTION_INTERPOLATE;
        } else {
            return MOTION_BACKWARD;
        }
    } else if (forwardErr <= backwardErr) {
        return MOTION_FORWARD;
    } else {
        return MOTION_BACKWARD;
    }
}


/*===========================================================================*
 *
 * BMotionSearch
 *
 *      search for the best B-frame motion vectors
 *
 * RETURNS:     MOTION_FORWARD      forward motion should be used
 *              MOTION_BACKWARD     backward motion should be used
 *              MOTION_INTERPOLATE  both should be used and interpolated
 *
 * OUTPUTS:     *fmx, *fmy  =   TWICE the forward motion vector
 *              *bmx, *bmy  =   TWICE the backward motion vector
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:       The relevant block in 'current' is valid (it has not
 *                      been dct'd).  Thus, the data in 'current' can be
 *                      accesed through y_blocks, cr_blocks, and cb_blocks.
 *                      This is not the case for the blocks in 'prev' and
 *                      'next.'  Therefore, references into 'prev' and 'next'
 *                      should be done
 *                      through the struct items ref_y, ref_cr, ref_cb
 *
 * POSTCONDITIONS:      current, prev, next should be unchanged.
 *                      Some computation could be saved by requiring
 *                      the dct'd difference to be put into current's block
 *                      elements here, depending on the search technique.
 *                      However, it was decided that it mucks up the code
 *                      organization a little, and the saving in computation
 *                      would be relatively little (if any).
 *
 * NOTES:       the search procedure MAY return (0,0) motion vectors
 *
 *===========================================================================*/
int
BMotionSearch(currentBlock, prev, next, interPrev, interNext, fStepSize, fSearchRange,
    bStepSize, bSearchRange, by, bx, height, width, fmy, fmx, bmy, bmx, oldMode)
    LumBlock currentBlock;
    ByteImage *prev, *next;
    ByteImage **interPrev, **interNext;
    int fStepSize, fSearchRange;
    int bStepSize, bSearchRange;
    int by, bx;
    int height, width;
    int *fmx, *fmy;
    int *bmx, *bmy;
    int oldMode;
{
    /* simply call the appropriate algorithm, based on user preference */
    return BMotionSearchSimple(currentBlock, prev, next, interPrev, interNext, fStepSize, fSearchRange,
        bStepSize, bSearchRange, by, bx, height, width, fmy, fmx, bmy, bmx, oldMode);
    /*switch(bsearchAlg) {
       case BSEARCH_SIMPLE:
       return BMotionSearchSimple(currentBlock, prev, next, by, bx, fmy,
       fmx, bmy, bmx, oldMode);
       break;
       case BSEARCH_CROSS2:
       return BMotionSearchCross2(currentBlock, prev, next, by, bx, fmy,
       fmx, bmy, bmx, oldMode);
       break;
       case BSEARCH_EXHAUSTIVE:
       return BMotionSearchExhaust(currentBlock, prev, next, by, bx, fmy,
       fmx, bmy, bmx, oldMode);
       break;
       default:
       fprintf(stderr, "Illegal B-frame motion search algorithm:  %d\n",
       bsearchAlg);
       exit(1);
       } */
}
