/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "bitparser.h"
#include "dvmpnm.h"
#include <ctype.h>

PnmHdr *
PnmHdrNew()
{
    return NEW(PnmHdr);
}

void
PnmHdrFree(hdr) 
    PnmHdr* hdr;
{
    FREE(hdr);
}

void
PnmHdrCopy(src, dest) 
    PnmHdr* src;
    PnmHdr* dest;
{
    dest->width = src->width;
    dest->height = src->height;
    dest->type = src->type;
    dest->maxVal = src->maxVal;
}

int
PnmHdrParse(bp, hdr)
    BitParser *bp;
    PnmHdr *hdr;
{
    unsigned char *curr;
    int count;
    int limit;
    int total;

    curr = bp->offsetPtr;
    count = 0;
    limit = 4;  /* only 3 for pbm */
    while (count < limit) {
        if (*curr == '#') {
            while (*curr != '\n') {
                curr++; /* skip comments */
            } 
            if (Bp_Underflow(bp)) {
                return DVM_PNM_BS_UNDERFLOW;
            }
        } else if (!isspace(*curr)) {
            if (count == 0) {
                if (*curr != 'P') {
                    return DVM_PNM_INVALID_HDR;
                } else {
                    curr++;
                    if (!isdigit(*curr)) {
                        return DVM_PNM_INVALID_HDR;
                    }
                    hdr->type = *curr - '0';
                    if (hdr->type == PBM_BIN || hdr->type == PBM_TEXT) {
                        limit = 3;
                        hdr->maxVal = 1;
                    }
                    count++;
                    curr++;
                    if (Bp_Underflow(bp)) {
                        return DVM_PNM_BS_UNDERFLOW;
                    }
                }
            } else if (count == 1) {
                hdr->width = 0;
                while (!isspace(*curr)) {
                    if (!isdigit(*curr)) {
                        return DVM_PNM_INVALID_HDR;
                    }
                    hdr->width = hdr->width*10 + (*curr - '0');
                    curr++;
                }
                count++;
                if (Bp_Underflow(bp)) {
                    return DVM_PNM_BS_UNDERFLOW;
                }
            } else if (count == 2) {
                hdr->height = 0;
                while (!isspace(*curr)) {
                    if (!isdigit(*curr)) {
                        return DVM_PNM_INVALID_HDR;
                    }
                    hdr->height = hdr->height*10 + (*curr - '0');
                    curr++;
                }
                count++;
                if (Bp_Underflow(bp)) {
                    return DVM_PNM_BS_UNDERFLOW;
                }
            } else {
                hdr->maxVal = 0;
                while (!isspace(*curr)) {
                    if (!isdigit(*curr)) {
                        return DVM_PNM_INVALID_HDR;
                    }
                    hdr->maxVal = hdr->maxVal*10 + (*curr - '0');
                    curr++;
                }
                count++;
                if (Bp_Underflow(bp)) {
                    return DVM_PNM_BS_UNDERFLOW;
                }
            }
        } else {
            curr++;
        }
    }
    curr++; /* put the cursor at the beginning of pgm data */
    total = curr - bp->offsetPtr;
    bp->offsetPtr = curr;
    return total;
}


int
PnmHdrEncode(hdr, bp)
    PnmHdr *hdr;
    BitParser *bp;
{
    int size;
    if (hdr->type == PBM_BIN || hdr->type == PBM_TEXT) {
        sprintf((char *)bp->offsetPtr, 
            "P%d %d %d\n", hdr->type, hdr->width, hdr->height);
    } else {
        sprintf((char *)bp->offsetPtr, 
            "P%d %d %d %d\n", hdr->type, hdr->width, hdr->height, hdr->maxVal);
    }
    size = strlen((char *)bp->offsetPtr);

    bp->offsetPtr += size;
    bp->bs->endDataPtr = bp->offsetPtr;
    return size;
}
