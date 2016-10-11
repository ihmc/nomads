/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmBasic.h"

int
BitStreamNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    int status, size;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s size", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &size);
    ReturnErrorIf(status != TCL_OK);

    bs = BitStreamNew(size);

    PutBitStream(interp, bs);
    return TCL_OK;
}

int
BitStreamFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s bitStream", argv[0]);

    bs = RemoveBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);
    BitStreamFree(bs);

    return TCL_OK;
}

#ifdef HAVE_MMAP
int
BitStreamMmapReadNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s <filename>", argv[0]);

    bs = BitStreamMmapReadNew(argv[1]);

    ReturnErrorIf1(bs == NULL,
        "%s: failed to create mmapped bitstream", argv[0]);

    PutBitStream(interp, bs);
    return TCL_OK;
}

int
BitStreamMmapReadFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s <bs>", argv[0]);

    bs = RemoveBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);
    BitStreamMmapReadFree(bs);

    return TCL_OK;
}
#endif

int
BitStreamResizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    int status, size;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s bitStream size", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &size);
    ReturnErrorIf(status != TCL_OK);

    BitStreamResize(bs, size);

    return TCL_OK;
}

int
BitStreamShareBufferCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s src dest", argv[0]);

    src = GetBitStream(argv[1]);
    ReturnErrorIf2(src == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);
    dest = GetBitStream(argv[1]);
    ReturnErrorIf2(dest == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    BitStreamShareBuffer(src, dest);

    return TCL_OK;
}

int
BitStreamBytesLeftCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    int status, off, left;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s bitStream offset", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &off);
    ReturnErrorIf(status != TCL_OK);

    left = BitStreamBytesLeft(bs, off);
    sprintf(interp->result, "%d", left);

    return TCL_OK;
}


int
BitStreamShiftCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    int status, off;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s bitStream offset", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &off);
    ReturnErrorIf(status != TCL_OK);

    BitStreamShift(bs, off);

    return TCL_OK;
}

int
BitParserNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bs;

    ReturnErrorIf1(argc != 1,
        "wrong # args : should be %s", argv[0]);

    bs = BitParserNew();

    PutBitParser(interp, bs);
    return TCL_OK;
}

int
BitParserFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s bitStream", argv[0]);

    bp = RemoveBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s : no such bitparser %s", argv[0], argv[1]);
    BitParserFree(bp);

    return TCL_OK;
}

int
BitParserSeekCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int status, off;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s bitParser offset", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s : no such bitparser %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &off);
    ReturnErrorIf(status != TCL_OK);

    BitParserSeek(bp, off);

    return TCL_OK;
}

int
BitParserTellCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int off;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s bitParser", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s : no such bitparser %s", argv[0], argv[1]);
    off = BitParserTell(bp);

    sprintf(interp->result, "%d", off);

    return TCL_OK;
}

int
BitParserWrapCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    BitStream *bs;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s bitParser bitStream", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s : no such bitparser %s", argv[0], argv[1]);
    bs = GetBitStream(argv[2]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[2]);

    BitParserWrap(bp, bs);

    return TCL_OK;
}

int
BitParserGetBitStreamCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    char *bsName;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s bitParser", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s : no such bitparser %s", argv[0], argv[1]);

    ReturnErrorIf2(bp->bs == NULL,
        "%s : bitparser %s has no attached bitstream", argv[0], argv[1]);

    bsName = FindBitStream((char *) bp->bs);
    ReturnErrorIf1(bsName == NULL,
        "The bitstream associated with bitparser %s has been freed", argv[1]);
    Tcl_SetResult(interp, bsName, TCL_VOLATILE);

    return TCL_OK;
}


int
BitStreamChannelReadCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Tcl_Channel chan;
    int status, off, bytes;

    ReturnErrorIf1(argc != 4,
        "wrong # args : should be %s bitStream channel offset", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such channel %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &off);
    ReturnErrorIf(status != TCL_OK);

    bytes = BitStreamChannelRead(bs, chan, off);
    sprintf(interp->result, "%d", bytes);

    return TCL_OK;
}

int
BitStreamChannelReadSegmentCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Tcl_Channel chan;
    int status, off, bytes, length;

    ReturnErrorIf1(argc != 5,
        "wrong # args : should be %s bitStream channel offset length", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such channel %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &off);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &length);
    ReturnErrorIf(status != TCL_OK);

    bytes = BitStreamChannelReadSegment(bs, chan, off, length);
    sprintf(interp->result, "%d", bytes);

    return TCL_OK;
}


int
BitStreamChannelReadSegmentsCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Tcl_Channel chan;
    int status, off, bytes, length, skip, times;

    ReturnErrorIf1(argc != 7,
        "wrong # args : should be %s bitStream channel offset length skip times", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such channel %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &off);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &length);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &skip);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[6], &times);
    ReturnErrorIf(status != TCL_OK);

    bytes = BitStreamChannelReadSegments(bs, chan, off, length, skip, times);
    sprintf(interp->result, "%d", bytes);

    return TCL_OK;
}


int
BitStreamChannelWriteCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Tcl_Channel chan;
    int status, off, bytes;

    ReturnErrorIf1(argc != 4,
        "wrong # args : should be %s bitStream channel offset", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such channel %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &off);
    ReturnErrorIf(status != TCL_OK);

    bytes = BitStreamChannelWrite(bs, chan, off);
    sprintf(interp->result, "%d", bytes);

    return TCL_OK;
}

int
BitStreamChannelWriteSegmentCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Tcl_Channel chan;
    int status, off, bytes, length;

    ReturnErrorIf1(argc != 5,
        "wrong # args : should be %s bitStream channel offset length", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such channel %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &off);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &length);
    ReturnErrorIf(status != TCL_OK);

    bytes = BitStreamChannelWriteSegment(bs, chan, off, length);
    sprintf(interp->result, "%d", bytes);

    return TCL_OK;
}


int
BitStreamChannelWriteSegmentsCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Tcl_Channel chan;
    int status, off, bytes, length, skip, times;

    ReturnErrorIf1(argc != 7,
        "wrong # args : should be %s bitStream channel offset length skip times",
        argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such channel %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &off);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &length);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &skip);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[6], &times);
    ReturnErrorIf(status != TCL_OK);

    bytes = BitStreamChannelWriteSegments(bs, chan, off, length, skip, times);
    sprintf(interp->result, "%d", bytes);

    return TCL_OK;
}


int
BitStreamChannelFilterInCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Tcl_Channel chan;
    int status, off, bytes;
    BitStreamFilter *index;

    ReturnErrorIf1(argc != 5,
        "wrong # args : should be %s bitStream channel offset filter", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s : no such bitstream %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such tcl channel %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &off);
    ReturnErrorIf(status != TCL_OK);

    index = GetBitStreamFilter(argv[4]);
    ReturnErrorIf2(index == NULL,
        "%s : no such bitstream index %s", argv[0], argv[4]);

    bytes = BitStreamChannelFilterIn(bs, chan, off, index);
    sprintf(interp->result, "%d", bytes);

    return TCL_OK;
}


int
BitStreamDumpCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *inbs, *outbs;
    int status, inoff, outoff, size;

    ReturnErrorIf1(argc != 6,
        "wrong # args: should be %s inBitStream inOffset outBitStream outOffset len", argv[0]);

    inbs = GetBitStream(argv[1]);
    ReturnErrorIf2(inbs == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &inoff);
    ReturnErrorIf(status != TCL_OK);

    outbs = GetBitStream(argv[3]);
    ReturnErrorIf2(outbs == NULL,
        "%s: no such bitstream %s", argv[0], argv[3]);

    status = Tcl_GetInt(interp, argv[4], &outoff);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &size);
    ReturnErrorIf(status != TCL_OK);

    BitStreamDump(inbs, inoff, outbs, outoff, size);

    return TCL_OK;
}


int
BitStreamDumpSegmentsCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *inbs, *outbs;
    int status, inoff, outoff, size, skip, times;

    ReturnErrorIf1(argc != 8,
        "wrong # args: should be %s inBitStream inOffset outBitStream outOffset size skip times", argv[0]);

    inbs = GetBitStream(argv[1]);
    ReturnErrorIf2(inbs == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &inoff);
    ReturnErrorIf(status != TCL_OK);

    outbs = GetBitStream(argv[3]);
    ReturnErrorIf2(outbs == NULL,
        "%s: no such bitstream %s", argv[0], argv[3]);

    status = Tcl_GetInt(interp, argv[4], &outoff);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &size);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[6], &skip);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[7], &times);
    ReturnErrorIf(status != TCL_OK);

    BitStreamDumpSegments(inbs, inoff, outbs, outoff, size, skip, times);

    return TCL_OK;
}
int
BitStreamFilterNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{

    BitStreamFilter *filter;
    int status, size;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s numOfEntry", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &size);
    ReturnErrorIf(status != TCL_OK);

    filter = BitStreamFilterNew(size);
    PutBitStreamFilter(interp, filter);

    return TCL_OK;
}


int
BitStreamFilterFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStreamFilter *filter;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s filter", argv[0]);

    filter = GetBitStreamFilter(argv[1]);
    ReturnErrorIf2(filter == NULL,
        "%s: no such bitstream filter %s", argv[0], argv[1]);

    BitStreamFilterFree(filter);

    return TCL_OK;
}


int
BitStreamFilterAddCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStreamFilter *filter;
    int status, offset, length;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s filter offset length", argv[0]);

    filter = GetBitStreamFilter(argv[1]);
    ReturnErrorIf2(filter == NULL,
        "%s: no such bitstream filter %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &offset);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &length);
    ReturnErrorIf(status != TCL_OK);

    status = BitStreamFilterAdd(filter, offset, length);
    ReturnErrorIf2(status == DVM_STREAMS_FILTER_FULL,
        "%s: filter table of %s is full.", argv[0], argv[1]);

    return TCL_OK;
}


int
BitStreamFilterResizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStreamFilter *filter;
    int status, size;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s filter newSize", argv[0]);

    filter = GetBitStreamFilter(argv[1]);
    ReturnErrorIf2(filter == NULL,
        "%s: no such bitstream filter %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &size);
    ReturnErrorIf(status != TCL_OK);

    BitStreamFilterResize(filter, size);

    return TCL_OK;
}

int
BitStreamDumpUsingFilterCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStreamFilter *filter;
    int srcOffset, destOffset, length;
    BitStream *srcbs, *destbs;
    int status, copied;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s srcbs srcOffset dstbs destOffset length filter", argv[0]);

    srcbs = GetBitStream(argv[1]);
    ReturnErrorIf2(srcbs == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &srcOffset);
    ReturnErrorIf(status != TCL_OK);

    destbs = GetBitStream(argv[3]);
    ReturnErrorIf2(destbs == NULL,
        "%s: no such bitstream %s", argv[0], argv[3]);

    status = Tcl_GetInt(interp, argv[4], &destOffset);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &length);
    ReturnErrorIf(status != TCL_OK);

    filter = GetBitStreamFilter(argv[6]);
    ReturnErrorIf2(filter == NULL,
        "%s: no such bitstream filter %s", argv[0], argv[6]);

    copied = BitStreamDumpUsingFilter(srcbs, srcOffset, destbs,
        destOffset, length, filter);
    sprintf(interp->result, "%d", copied);
    return TCL_OK;
}

int
BitStreamFilterChannelReadCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStreamFilter *filter;
    Tcl_Channel chan;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s filter channel", argv[0]);

    filter = GetBitStreamFilter(argv[1]);
    ReturnErrorIf2(filter == NULL,
        "%s : no such bitstream filter %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such channel %s", argv[0], argv[2]);

    BitStreamFilterChannelRead(filter, chan);

    return TCL_OK;
}

int
BitStreamFilterChannelWriteCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStreamFilter *filter;
    Tcl_Channel chan;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s filter channel", argv[0]);

    filter = GetBitStreamFilter(argv[1]);
    ReturnErrorIf2(filter == NULL,
        "%s : no such bitstream filter %s", argv[0], argv[1]);

    chan = Tcl_GetChannel(interp, argv[2], NULL);
    ReturnErrorIf2(chan == NULL,
        "%s : no such channel %s", argv[0], argv[2]);

    BitStreamFilterChannelWrite(filter, chan);

    return TCL_OK;
}

int
BitStreamFilterStartScanCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStreamFilter *filter;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s filter", argv[0]);

    filter = GetBitStreamFilter(argv[1]);
    ReturnErrorIf2(filter == NULL,
        "%s : no such bitstream filter %s", argv[0], argv[1]);

    BitStreamFilterStartScan(filter);

    return TCL_OK;
}
