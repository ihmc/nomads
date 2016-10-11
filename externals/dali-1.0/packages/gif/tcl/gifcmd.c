/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/* Documentation updated by Jiesang 10/11/98 */

#include "tclDvmGif.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_seq_hdr_new
 * precond 
 *     none
 * return 
 *     the buffer number
 * side effect :
 *     a new gif sequence header is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */
int
GifSeqHdrNewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    gifHdr = GifSeqHdrNew();
    PutGifSeqHdr(interp, gifHdr);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_seq_hdr_free <hdr>
 * precond 
 *      _hdr_ exists
 * return 
 *     none
 * side effect :
 *     the memory allocated for _hdr_ is freed.
 *
 *----------------------------------------------------------------------
 */

int
GifSeqHdrFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s gifSeqHdr", argv[0]);

    hdr = RemoveGifSeqHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such seq hdr %s", argv[0], argv[1]);

    GifSeqHdrFree (hdr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_seq_hdr_parse <bp> <gifHdr>
 * precond 
 *      _bp_ and _buf_ exist
 * return 
 *     none
 * side effect :
 *     Assuming that the corresponding bitstreams position is at the 
 *     beginning of a GIF file, bitparser bp will parse its bitstream and 
 *     initialize the sequence header structure _gifHdr_. After parsing, the 
 *     bitparser's current bit position is right after the end of the sequence 
 *     header. 
 *
 *----------------------------------------------------------------------
 */

int
GifSeqHdrParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser* bp;
    GifSeqHdr* gifHdr;
    int ret;

    /*
     * Retrive the file and buffer from the hashtables
     */

    ReturnErrorIf1 (argc != 3,
        "wrong # of args: should be %s bitParser gifHdr", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2  (bp == NULL,
        "%s: no such bit parser %s", argv[0], argv[1]);

    gifHdr = GetGifSeqHdr (argv[2]);
    ReturnErrorIf2  (gifHdr == NULL,
        "%s: no such GIF img header %s", argv[0], argv[2]);

    if ((ret = GifSeqHdrParse(bp, gifHdr)) != DVM_GIF_OK)
    {
        switch(ret) {
        case DVM_GIF_BAD_HEADER:
            sprintf(interp->result, "%s: not a valid GIF file header."
                    ,argv[0]);
            return TCL_ERROR;
        case DVM_GIF_BAD_VERSION:
            sprintf(interp->result, "%s: only versions 87a & 89a are supported."
                    ,argv[0]);
            return TCL_ERROR;
        }
    }

    return TCL_OK;
 }

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_seq_hdr_encode <seqHdr> <bp>
 * precond 
 *      _bp_ and _seqHdr_ exist
 * return 
 *     none
 * side effect :
 *     Encode the content of sequence header seqHdr with bitparser bp. 
 *
 *----------------------------------------------------------------------
 */

int
GifSeqHdrEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser* bp;
    GifSeqHdr* seqHdr;

    ReturnErrorIf1 (argc != 3,
        "wrong # of args: should be %s seqHdr bitParser",
        argv[0]);

    seqHdr = GetGifSeqHdr (argv[1]);
    ReturnErrorIf2 (seqHdr == NULL,
        "%s: no such GIF seq header %s", argv[0], argv[1]);

    bp = GetBitParser (argv[2]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    GifSeqHdrEncode(seqHdr, bp);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_seq_loop_encode <bp>
 * precond 
 *      _bp_ exists
 * return 
 *     none
 * side effect :
 *     The bitparser bp, encodes the Netscape application header that indicated
 *     that the sequence should be played in a loop
 *
 *----------------------------------------------------------------------
 */

int
GifSeqLoopEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;

    ReturnErrorIf1 (argc != 2,
        "wrong # of args: should be %s bitParser", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    GifSeqLoopEncode(bp);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_seq_trailer_encode <bp>
 * precond 
 *      _bp_ exists
 * return 
 *     none
 * side effect :
 *     Encode the GIF sequence trailer to indicate the end of a GIF file
 *
 *----------------------------------------------------------------------
 */

int
GifSeqTrailerEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;

    ReturnErrorIf1 (argc != 2,
        "wrong # of args: should be %s bitParser", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    GifSeqTrailerEncode(bp);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_seq_ct_parse <bp> <redmap> <greenmap> <bluemap>
 * precond 
 *      _bp_ and the three maps exist
 * return 
 *     none
 * side effect :
 *     Assuming the bitparser bp's position is at the beginning of a 
 *     color table, parse the color table, and initialize the ImageMaps 
 *     redmap, bluemap, and greenmap.  The bitparser's new position will 
 *     be immediately following the color table.  
 *
 *----------------------------------------------------------------------
 */

int
GifCtParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser* bp;
    int status, size;
    ImageMap *red, *green, *blue;

    /*
     * Retrive the file and buffer from the hashtables
     */

    ReturnErrorIf1 (argc != 6,
            "wrong # of args: should be %s bitParser size rMap gMap bMap",
            argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    status = Tcl_GetInt (interp, argv[2], &size);
    ReturnErrorIf (status != TCL_OK);

    red = GetImageMap (argv[3]);
    ReturnErrorIf2 (red == NULL,
        "%s: no such image map %s", argv[0], argv[3]);

    green = GetImageMap (argv[4]);
    ReturnErrorIf2 (red == NULL,
        "%s: no such image map %s", argv[0], argv[4]);

    blue = GetImageMap (argv[5]);
    ReturnErrorIf2 (blue == NULL,
        "%s: no such image map %s", argv[0], argv[5]);

    GifCtParse(bp, size, red, green, blue);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_ct_encode <seqHdr> <redmap> <greenmap> <bluemap> <bp>
 * precond 
 *      _bp_, _seqHdr_, and the three maps exist
 * return 
 *     none
 * side effect :
 *     Encode the color table with values from the ImageMaps redmap, 
 *     bluemap, and greenmap using bitparser bp.  
 *
 *----------------------------------------------------------------------
 */

int
GifCtEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser* bp;
    int size, status;
    ImageMap *red, *green, *blue;

    ReturnErrorIf1 (argc != 6,
        "wrong # of args: should be %s ctSize rMap gMap bMap bitParser",
        argv[0]);

    status = Tcl_GetInt (interp, argv[1], &size);
    ReturnErrorIf (status != TCL_OK);

    red = GetImageMap (argv[2]);
    ReturnErrorIf2 (red == NULL,
        "%s: no such Image Map %s", argv[0], argv[2]);

    green = GetImageMap (argv[3]);
    ReturnErrorIf2 (green == NULL,
        "%s: no such Image Map %s", argv[0], argv[3]);

    blue = GetImageMap (argv[4]);
    ReturnErrorIf2 (red == NULL,
        "%s: no such Image Map %s", argv[0], argv[4]);

    bp = GetBitParser (argv[5]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[5]);

    GifCtEncode(size, red, green, blue, bp);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_hdr_new
 * precond 
 *     none
 * return 
 *     the buffer number
 * side effect :
 *     a new gif sequence header is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */

int
GifImgHdrNewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;

    gifHdr = GifImgHdrNew ();

    PutGifImgHdr(interp, gifHdr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_hdr_free <hdr>
 * precond 
 *      _hdr_ exists
 * return 
 *     none
 * side effect :
 *     the memory allocated for _hdr_ is freed.
 *
 *----------------------------------------------------------------------
 */

int
GifImgHdrFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = RemoveGifImgHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such img hdr %s", argv[0], argv[1]);

    GifImgHdrFree (hdr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_hdr_parse <bp> <gifHdr>
 * precond 
 *      _bp_ and _gifHdr_ exist
 * return 
 *     none
 * side effect :
 *     Assuming that the corresponding bitstreams position is at the 
 *     beginning of an image header, bitparser bp will parse its bitstream and 
 *     initialize the image header structure _gifHdr_. After parsing, the 
 *     bitparser's current bit position is right after the end of the sequence 
 *     header. 
 *
 *----------------------------------------------------------------------
 */

int
GifImgHdrParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser* bp;
    GifImgHdr* gifHdr;
    int ret;

    /*
     * Retrive the file and buffer from the hashtables
     */

    ReturnErrorIf1 (argc != 3,
        "wrong # of args: should be %s bitParser gifHdr", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2  (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    gifHdr = GetGifImgHdr (argv[2]);
    ReturnErrorIf2 (gifHdr == NULL,
        "%s: no such GIF img header %s", argv[0], argv[2]);

    if ((ret = GifImgHdrParse(bp, gifHdr)) != DVM_GIF_OK) {
        if (ret == DVM_GIF_IMG_SEPARATOR_ERROR) {
            sprintf(interp->result, "%s: invalid GIF image header", argv[0]);
            return TCL_ERROR;
        }
        else if (ret == DVM_GIF_EOF_ERROR) {
            sprintf(interp->result, "%s: GIF trailer found", argv[0]);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_hdr_encode <hdr> <bp>
 * precond 
 *      _bp_ and _imgHdr_ exist
 * return 
 *     none
 * side effect :
 *     Encode the content of img header hdr with bitparser bp. 
 *
 *----------------------------------------------------------------------
 */
int
GifImgHdrEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* hdr;
    BitParser* bp;

    ReturnErrorIf1 (argc != 3,
        "wrong # of args: should be %s imgHdr bitParser", argv[0]);

    hdr = GetGifImgHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such GIF img header %s", argv[0], argv[1]);

    bp = GetBitParser (argv[2]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    GifImgHdrEncode(hdr, bp);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_parse <bp> <seqHdr> <imgHdr> <mapBuf>
 * precond 
 *      _bp_, _seqHdr_, _imgHdr_, and _mapBuf_ exist
 * return 
 *     none
 * side effect :
 *     Assuming that the corresponding bitstream's position is at the 
 *     beginning of image data, bitparser bp will parse its bitstream and 
 *     initialize the ByteImage buf with ColorTable indices corresponding 
 *     to the ImageMaps containing the ColorTable data. After parsing, the 
 *     bitparser's current bit position is immediately following the end of 
 *     the image data. 
 *
 *----------------------------------------------------------------------
 */

int
GifImgInterlacedParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser* bp;
    GifSeqHdr* seqHdr;
    GifImgHdr* imgHdr;
    ByteImage *mapBuf;

    /*
     * Retrive the file and buffer from the hashtables
     */

    ReturnErrorIf1 (argc != 5,
        "wrong # of args: should be %s bitParser seqHdr imgHdr byteImage"
        , argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    seqHdr = GetGifSeqHdr (argv[2]);
    ReturnErrorIf2 (seqHdr == NULL,
        "%s: no such GIF seq header %s", argv[0], argv[2]);

    imgHdr = GetGifImgHdr (argv[3]);
    ReturnErrorIf2 (imgHdr == NULL,
        "%s: no such GIF img header %s", argv[0], argv[3]);

    mapBuf = GetByteImage(argv[4]);
    ReturnErrorIf2 (mapBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    if (GifImgInterlacedParse(bp, seqHdr, imgHdr, mapBuf) != DVM_GIF_OK)
    {
        sprintf(interp->result, "%s: Invalid image data", argv[0]);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_parse <bp> <seqHdr> <imgHdr> <mapBuf>
 * precond 
 *      _bp_, _seqHdr_, _imgHdr_, and _mapBuf_ exist
 * return 
 *     none
 * side effect :
 *     Assuming that the corresponding bitstream's position is at the 
 *     beginning of image data, bitparser bp will parse its bitstream and 
 *     initialize the ByteImage buf with ColorTable indices corresponding 
 *     to the ImageMaps containing the ColorTable data. After parsing, the 
 *     bitparser's current bit position is immediately following the end of 
 *     the image data. 
 *
 *----------------------------------------------------------------------
 */

int
GifImgNonInterlacedParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser* bp;
    GifSeqHdr* seqHdr;
    GifImgHdr* imgHdr;
    ByteImage *mapBuf;

    /*
     * Retrive the file and buffer from the hashtables
     */

    ReturnErrorIf1 (argc != 5,
        "wrong # of args: should be %s bitParser seqHdr imgHdr byteImage"
        , argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    seqHdr = GetGifSeqHdr (argv[2]);
    ReturnErrorIf2 (seqHdr == NULL,
        "%s: no such GIF seq header %s", argv[0], argv[2]);

    imgHdr = GetGifImgHdr (argv[3]);
    ReturnErrorIf2 (imgHdr == NULL,
        "%s: no such GIF img header %s", argv[0], argv[3]);

    mapBuf = GetByteImage(argv[4]);
    ReturnErrorIf2 (mapBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    if (GifImgNonInterlacedParse(bp, seqHdr, imgHdr, mapBuf) != DVM_GIF_OK)
    {
        sprintf(interp->result, "%s: Invalid image data", argv[0]);
        return TCL_ERROR;
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_encode <bp> <srcBuf> <seqHdr> <imgHdr> 
 * precond 
 *      _bp_, _srcBuf_, _seqHdr_, and _imgHdr_ exist
 * return 
 *     none
 * side effect :
 *     Encode the Color Table indices in ByteImage srcBuf using bitparser bp 
 *
 *----------------------------------------------------------------------
 */

int
GifImgEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser* bp;
    ByteImage *srcBuf;
    GifSeqHdr *seqHdr;
    GifImgHdr *imgHdr;

    ReturnErrorIf1 (argc != 5,
        "wrong # of args: should be %s seqHdr imgHdr byte bitParser",
        argv[0]);

    seqHdr = GetGifSeqHdr (argv[1]);
    ReturnErrorIf2 (seqHdr == NULL,
        "%s: no such GIF seq header %s", argv[0], argv[1]);

    imgHdr = GetGifImgHdr (argv[2]);
    ReturnErrorIf2 (imgHdr == NULL,
        "%s: no such GIF img header %s", argv[0], argv[2]);

    srcBuf = GetByteImage(argv[3]);
    ReturnErrorIf2 (srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    bp = GetBitParser (argv[4]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[4]);

    GifImgEncode(seqHdr, imgHdr, srcBuf, bp);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_skip <bp>
 * precond 
 *      _bp_ exists
 * return 
 *     none
 * side effect :
 *     Assuming the bitparser bp's position is at the beginning of an image 
 *     header, skip the image.  Afterwards, the bitparser's current bit position 
 *     is at the start of the next image header, or the Gif sequence trailer. 
 *
 *----------------------------------------------------------------------
 */
int
GifImgSkipCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int retval;

    ReturnErrorIf1 (argc != 2,
        "wrong # of args: should be %s bitParser", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    retval = GifImgSkip(bp);
    if (retval != DVM_GIF_OK)
    {
        switch (retval) {
            case DVM_GIF_IMG_READ_ERROR:
                sprintf(interp->result, "%s: Invalid image data", argv[0]);
                return TCL_ERROR;
            case DVM_GIF_EOF_ERROR:
                sprintf(interp->result, "%s: GIF end of file trailer found", 
                        argv[0]);
                return TCL_OK;
            case DVM_GIF_IMG_SEPARATOR_ERROR:
                sprintf(interp->result, "%s: Invalid image seperator", argv[0]);
                return TCL_ERROR;
        }
        sprintf(interp->result, "%s: Unkown error", argv[0]);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     gif_img_find <bp>
 * precond 
 *      _bp_ exists
 * return 
 *     none
 * side effect :
 *     Using bitparser bp, search for the next graphic control extension or
 *     image separator
 *
 *----------------------------------------------------------------------
 */
int
GifImgFindCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int retval;

    ReturnErrorIf1 (argc != 2,
        "wrong # of args: should be %s bitParser", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    retval = GifImgFind(bp);
    sprintf(interp->result, "%d", retval);
    return TCL_OK;
}
