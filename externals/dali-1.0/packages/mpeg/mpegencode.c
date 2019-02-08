/*
 *--------------------------------------------------------------------
 * Functions for encoding ScImages into bitstream
 *
 * Jiesang Song     Aug 98
 *---------------------------------------------------------------------
 */

#include "mpegInt.h"

#define SET_BLOCK_POINTERS(pBlock, i, j, scY, scU, scV) {\
    pBlock[0] = (ScBlock *)&(scY->firstBlock[i]);\
    pBlock[1] = (ScBlock *)&(scY->firstBlock[i+1]);\
    pBlock[2] = (ScBlock *)&(scY->firstBlock[i+scY->parentWidth]);\
    pBlock[3] = (ScBlock *)&(scY->firstBlock[i+1+scY->parentWidth]);\
    pBlock[4] = (ScBlock *)&(scU->firstBlock[j>>1]);\
    pBlock[5] = (ScBlock *)&(scV->firstBlock[j>>1]);\
}\

#define GET_PATTERN(pBlock, pattern) {\
    pattern = 63 - 32 * (pBlock[0])->skipBlock - 16 * (pBlock[1])->skipBlock \
             - 8 * (pBlock[2])->skipBlock - 4 * (pBlock[3])->skipBlock \
             - 2 * (pBlock[4])->skipBlock - (pBlock[5])->skipBlock;\
}

/* encode 4 Y blocks, 1 U(Cr) block, and 1 V(Cb) block */
#define I_MBLOCK_ENCODE(bp, pBlock, y_dc_pred, cr_dc_pred, cb_dc_pred) {\
    EncodeYDC(bp, pBlock[0]->dc, &y_dc_pred);\
    IBlockHuffEncode(bp, pBlock[0]);\
    EncodeYDC(bp, pBlock[1]->dc, &y_dc_pred);\
    IBlockHuffEncode(bp, pBlock[1]);\
    EncodeYDC(bp, pBlock[2]->dc, &y_dc_pred);\
    IBlockHuffEncode(bp, pBlock[2]);\
    EncodeYDC(bp, pBlock[3]->dc, &y_dc_pred);\
    IBlockHuffEncode(bp, pBlock[3]);\
    EncodeCDC(bp, pBlock[4]->dc, &cr_dc_pred);\
    IBlockHuffEncode(bp, pBlock[4]);\
    EncodeCDC(bp, pBlock[5]->dc, &cb_dc_pred);\
    IBlockHuffEncode(bp, pBlock[5]);\
}


void
MpegPicIEncode(picHdr, scY, scU, scV, qScale, sliceInfo, sliceInfoLen, bp)
    MpegPicHdr *picHdr;
    ScImage *scY, *scU, *scV;
    ByteImage *qScale;
    int *sliceInfo;
    int sliceInfoLen;
    BitParser *bp;
{
    int x, y, i = 0, j = 0;     /* hack, must remove i and j and use pointers */
    int skip = 2 * scY->parentWidth - scY->width;
    int qScaleSkip = qScale->parentWidth - qScale->width;
    int curSliceNum = 0;        /* index into sliceInfo array */
    int numMBInSlice = 0;       /* number of Macroblocks currently in the slice */
    short y_dc_pred = 128, cr_dc_pred = 128, cb_dc_pred = 128;
    unsigned char *curQScale = qScale->firstByte;
    ScBlock *pBlock[6];         /* pointers to the 6 blocks that form a Macroblock */

    MpegSliceHdrEncode(bp, 1, *curQScale, NULL, 0);

    for (y = 0; y < scY->height / 2; y++) {
        for (x = 0; x < scY->width / 2; x++) {
            SET_BLOCK_POINTERS(pBlock, i, j, scY, scU, scV);

            if (numMBInSlice++ == sliceInfo[curSliceNum]) {
                /* create a new slice */
                MpegSliceHdrEncode(bp, 1 + y, *curQScale, NULL, 0);
                MpegIMacroBlockHdrEncode(bp, picHdr, 1 + x, *curQScale);

                y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

                if (++curSliceNum == sliceInfoLen) {
                    curSliceNum = 0;    /* wrap around in the sliceInfo array */
                }
                numMBInSlice = 0;
            } else {
                MpegIMacroBlockHdrEncode(bp, picHdr, 1, *curQScale);
            }

            I_MBLOCK_ENCODE(bp, pBlock, y_dc_pred, cr_dc_pred, cb_dc_pred);
            curQScale++;
            i += 2;
            j += 2;
        }
        i += skip;
        curQScale += qScaleSkip;
    }
    Bp_OutByteAlign(bp);
}


void
MpegPicPEncode(picHdr, scY, scU, scV, vec, qScale, sliceInfo, sliceInfoLen, bp)
    MpegPicHdr *picHdr;
    ScImage *scY, *scU, *scV;
    VectorImage *vec;
    ByteImage *qScale;
    int *sliceInfo;
    int sliceInfoLen;
    BitParser *bp;
{
    int fullPixelSearch = picHdr->full_pel_forward_vector;
    char forw_f = picHdr->forward_f;
    char pattern;
    short y_dc_pred, cr_dc_pred, cb_dc_pred;
    int x, y, i = 0, j = 0, k;
    int oldMotionX = 0, oldMotionY = 0;
    int offsetX, offsetY;
    int mbAddrInc = 1;
    int lastBlockX = scY->width;
    int lastBlockY = scY->height;
    int lastX, lastY;
    int curSliceNum = 0;        /* index into sliceInfo array */
    int numMBInSlice = 0;       /* number of Macroblocks currently in the slice */
    unsigned char *curQScale = qScale->firstByte;
    ScBlock *pBlock[6];         /* pointers to the 6 blocks that form a Macroblock */
    Vector *curVector = vec->firstVector;

    int skipMB;                 /* TEMPORARY maybe uncessary for encoding - compatible with decoding? */

    MpegSliceHdrEncode(bp, 1, *curQScale, NULL, 0);

    /* for I-blocks */
    y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

    lastX = lastBlockX - 2;
    lastY = lastBlockY - 2;

    for (y = 0; y < lastBlockY; y += 2) {
        for (x = 0; x < lastBlockX; x += 2) {

            SET_BLOCK_POINTERS(pBlock, i, j, scY, scU, scV);

            if (numMBInSlice == sliceInfo[curSliceNum]) {
                /* create a new slice */
                MpegSliceHdrEncode(bp, 1 + (y / 2), *curQScale, NULL, 0);

                /* reset everything */
                oldMotionX = 0;
                oldMotionY = 0;
                y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

                mbAddrInc = 1 + (x / 2);
                if (++curSliceNum == sliceInfoLen) {
                    curSliceNum = 0;    /* wrap around in the sliceInfo array */
                }
                numMBInSlice = 0;
            }
            if (!curVector->exists) {
                /* ENCODE INTRA BLOCK */
                /* reset because intra-coded */
                oldMotionX = 0;
                oldMotionY = 0;
/* intra */ MpegIMacroBlockHdrEncode(bp, picHdr, mbAddrInc, *curQScale);
                mbAddrInc = 1;
                I_MBLOCK_ENCODE(bp, pBlock, y_dc_pred, cr_dc_pred, cb_dc_pred);
                for (k = 0; k < 6; k++) {
                    pBlock[k]->skipBlock = 0;
/* necessary? */ pBlock[k]->skipMB = 0;
                }
            } else {
                /* USE MOTION VECTORS */

                /* begin TEMPORARY */
                skipMB = 1;
                for (k = 0; k < 6; k++) {
                    if (!(pBlock[k]->skipBlock)) {
                        skipMB = 0;
                    }
                }
                for (k = 0; k < 6; k++) {
/* necessary? */ pBlock[k]->skipMB = skipMB;
                }
                /* end TEMPORARY */

                /* Encode the MB header */
                if ((curVector->down == 0) && (curVector->right == 0)) {
                    if (pBlock[0]->skipMB == 1) {
                        /* can only skip if:
                         *     1)  not the last block in frame
                         *     2)  not the last block in slice
                         *     3)  not the first block in slice
                         */
/* skip */ if (((y < lastY) || (x < lastX)) && (numMBInSlice < sliceInfo[curSliceNum]) && (numMBInSlice != 0)) {
                            mbAddrInc++;
                        } else {
/* pred-c */ MpegMacroBlockHdrEncode(bp, picHdr, mbAddrInc, 0, 0, 0, forw_f, 0, 0, 0, 0, 0);
                            mbAddrInc = 1;
                        }
                    } else {
/* pred-c */ GET_PATTERN(pBlock, pattern);
                        MpegMacroBlockHdrEncode(bp, picHdr, mbAddrInc, 0, 0, pattern, 0, 0, 0, 0, 0, 0);
                        mbAddrInc = 1;
                    }
                } else {
                    /* pred-mc *//* transform the motion vector into the appropriate values */
/* pred-m */ offsetX = (curVector->right - oldMotionX) >> fullPixelSearch;
                    offsetY = (curVector->down - oldMotionY) >> fullPixelSearch;

                    GET_PATTERN(pBlock, pattern);
                    MpegMacroBlockHdrEncode(bp, picHdr, mbAddrInc, 0, 0, pattern, forw_f, 0, offsetX, offsetY, 0, 0);
                    mbAddrInc = 1;
                }
                /* Huffman Encode Blocks */
                if (!pBlock[0]->skipMB) {
                    for (k = 0; k < 6; k++) {
                        if (!(pBlock[k]->skipBlock)) {
                            NonIBlockHuffEncode(bp, pBlock[k]);
                        }
                    }
                }
                oldMotionX = curVector->right;
                oldMotionY = curVector->down;

                /* reset because non-intra-coded */
                y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

            }                   /* USE MOTION VECTORS */

            i += 2;
            j += 2;
            numMBInSlice++;     /* must be down here because used to check if MB can be skipped */
            curVector++;
            curQScale++;
        }                       /* for x */

        i += 2 * scY->parentWidth - scY->width;
        curVector += vec->parentWidth - vec->width;
        curQScale += qScale->parentWidth - qScale->width;
    }                           /* for y */

    Bp_OutByteAlign(bp);
}

void
MpegPicBEncode(picHdr, scY, scU, scV, fmv, bmv, qScale, sliceInfo, sliceInfoLen, bp)
    MpegPicHdr *picHdr;
    ScImage *scY, *scU, *scV;
    VectorImage *fmv, *bmv;
    ByteImage *qScale;
    int *sliceInfo;
    int sliceInfoLen;
    BitParser *bp;
{
    int searchPixWidth = picHdr->full_pel_forward_vector + 1;   /* 1 (== 0.5) or 2 (== 1) */
    char forw_f = picHdr->forward_f;
    char back_f = picHdr->backward_f;

    short y_dc_pred, cr_dc_pred, cb_dc_pred;
    int x, y, i = 0, j = 0, k;
    int fmx = 0, fmy = 0;
    int bmx = 0, bmy = 0;
    int oldFmx = 0, oldFmy = 0;
    int oldBmx = 0, oldBmy = 0;
    int offset_f_x = 0, offset_f_y = 0;
    int offset_b_x = 0, offset_b_y = 0;

    int pattern;
    int mbAddrInc = 1;
    int lastBlockX = scY->width;
    int lastBlockY = scY->height;
    int lastX, lastY;
    int curSliceNum = 0;        /* index into sliceInfo array */
    int numMBInSlice = 0;       /* number of Macroblocks currently in the slice */
    unsigned char *curQScale = qScale->firstByte;
    ScBlock *pBlock[6];         /* pointers to the 6 blocks that form a Macroblock */
    Vector *curFVec = fmv->firstVector;
    Vector *curBVec = bmv->firstVector;

    MpegSliceHdrEncode(bp, 1, *curQScale, NULL, 0);

    /* for I-blocks */
    y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

    lastX = lastBlockX - 2;
    lastY = lastBlockY - 2;

    for (y = 0; y < lastBlockY; y += 2) {
        for (x = 0; x < lastBlockX; x += 2) {

            SET_BLOCK_POINTERS(pBlock, i, j, scY, scU, scV);

            if (numMBInSlice++ == sliceInfo[curSliceNum]) {
                /* create a new slice */
                MpegSliceHdrEncode(bp, 1 + (y / 2), *curQScale, NULL, 0);

                /* reset everything */
                oldFmx = 0;
                oldFmy = 0;
                oldBmx = 0;
                oldBmy = 0;
                y_dc_pred = cr_dc_pred = cb_dc_pred = 128;

                mbAddrInc = 1 + (x / 2);
                if (++curSliceNum == sliceInfoLen) {
                    curSliceNum = 0;    /* wrap around in the sliceInfo array */
                }
                numMBInSlice = 0;
            }
            if ((curFVec->exists == 2) && (curBVec->exists == 2)) {
                /* skipped macro block */
                mbAddrInc++;
                for (k = 0; k < 6; k++) {
                    pBlock[k]->skipBlock = 1;
/* necessary? */ pBlock[k]->skipMB = 1;
                }
            } else if ((curFVec->exists == 0) && (curBVec->exists == 0)) {
                /* ENCODE INTRA BLOCK */
                /* reset because intra-coded */
                oldFmx = 0;
                oldFmy = 0;
                oldBmx = 0;
                oldBmy = 0;

                MpegIMacroBlockHdrEncode(bp, picHdr, mbAddrInc, *curQScale);
                mbAddrInc = 1;
                I_MBLOCK_ENCODE(bp, pBlock, y_dc_pred, cr_dc_pred, cb_dc_pred);
                for (k = 0; k < 6; k++) {
                    pBlock[k]->skipBlock = 0;
/* necessary? */ pBlock[k]->skipMB = 0;
                }
            } else {
                /* USE MOTION VECTORS */
                /* make special cases for (0,0) motion???? */

                /* reset because non-intra-coded */
                y_dc_pred = cr_dc_pred = cb_dc_pred = 128;


                /*  VECTOR STUFF */
                fmx = curFVec->right;
                fmy = curFVec->down;
                bmx = curBVec->right;
                bmy = curBVec->down;
                if (searchPixWidth == 2) {
                    fmx >>= 1;
                    fmy >>= 1;
                    bmx >>= 1;
                    bmy >>= 1;
                }
                /* should really check to see if same motion as previous block, and see if pattern is 0, then skip it! */

                if (curFVec->exists) {
                    /* transform the fMotion vector into the appropriate values */
                    offset_f_x = fmx - oldFmx;
                    offset_f_y = fmy - oldFmy;
                    oldFmx = fmx;
                    oldFmy = fmy;
                }
                if (curBVec->exists) {
                    /* transform the bMotion vector into the appropriate values */
                    offset_b_x = bmx - oldBmx;
                    offset_b_y = bmy - oldBmy;
                    oldBmx = bmx;
                    oldBmy = bmy;
                }
                if (searchPixWidth == 2) {
                    fmx <<= 1;
                    fmy <<= 1;
                    bmx <<= 1;
                    bmy <<= 1;
                }
                /* Encode MB header */
                GET_PATTERN(pBlock, pattern);
                MpegMacroBlockHdrEncode(bp, picHdr, mbAddrInc, 0, 0, pattern, (char) (forw_f * (curFVec->exists)),
                    (char) (back_f * (curBVec->exists)), offset_f_x, offset_f_y, offset_b_x, offset_b_y);
                mbAddrInc = 1;

                /* Huffman Encode Blocks */
                for (k = 0; k < 6; k++) {
                    if (!(pBlock[k]->skipBlock)) {
                        NonIBlockHuffEncode(bp, pBlock[k]);
                    }
                }
            }

            i += 2;
            j += 2;
            curFVec++;
            curBVec++;
            curQScale++;
        }                       /* for x */
        i += scY->parentWidth;
        curFVec += fmv->parentWidth - fmv->width;
        curBVec += bmv->parentWidth - bmv->width;
        curQScale += qScale->parentWidth - qScale->width;
    }                           /* for y */

    Bp_OutByteAlign(bp);
}
