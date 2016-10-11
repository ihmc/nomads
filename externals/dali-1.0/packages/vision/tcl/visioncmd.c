/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmVision.h"


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_smooth <srcbuf> <destbuf> <passes>
 *
 * precond 
 *     Buffers _srcbuf_ and _destbuf_ exist and have the same dimensions, passes >= 1
 *
 * return 
 *     nothing
 *
 * side effect :
 *     destbuf contains the smooth version of srcbuf
 *
 *----------------------------------------------------------------------
 */


int
ByteSmoothCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int numPasses;
    int ret;

    /*
    * Check args, retrieve buffer from hash table, parse rest of args
    */

    if (argc != 4) {
    sprintf (interp->result,
        "wrong # args: should be %s srcbuf destbuf passes", argv[0]);
        return TCL_ERROR;
    }


    srcBuf  = GetByteImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    destBuf  = GetByteImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &numPasses) != TCL_OK) {
        return TCL_ERROR;
    }

    if ((ret = ByteSmooth(srcBuf, destBuf, numPasses)) != DVM_VISION_OK) {
        switch (ret) {
        case DVM_BAD_NUM_PASSES:
            sprintf (interp->result, "%s: number of passes but be a positive integer.", argv[0]);
            return TCL_ERROR;
        case DVM_DIFFERENT_SIZES:
            sprintf(interp->result, "%s: both byte images must have the same dimensions.", argv[0]);
            return TCL_ERROR;
        case DVM_SCRATCH_SIZE_TOO_SMALL:
            sprintf(interp->result, "%s: constant SCRATCHSIZE in dvmvision.h is too small.", argv[0]);
            return TCL_ERROR;
        }
    }


    return TCL_OK;

}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_edge_detect_sobel srcbuf dest1buf dest2buf percent threshold1 threshold2 
 *
 * precond 
 *     All three buffers have the same dimensions, percent is an integer between 0 and 100
 *
 * return 
 *     nothing
 *
 * side effect :
 *     dest1buf contains the convolution of the kernel (-1 0 1) and dest2buf contains the convolution
 *     of the kernel (-1
 *                     0
 *                     1), threshold1 is the threshold of dest1buf, and threshold2 is the threshold of dest2buf
 *
 *----------------------------------------------------------------------
 */

int
ByteEdgeDetectSobelCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *srcBuf, *dest1Buf, *dest2Buf;
    int percent;
    int thresh1, thresh2;
    int ret;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    if (argc != 5) {
    sprintf (interp->result,
        "wrong # args: should be %s srcbuf dest1buf dest2buf percent", argv[0]);
    return TCL_ERROR;
    } 

    srcBuf  = GetByteImage(argv[1]);
    if (srcBuf == NULL) {
    sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
    return TCL_ERROR;
    }

    dest1Buf  = GetByteImage(argv[2]);
    if (dest1Buf == NULL) {
    sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[2]);
    return TCL_ERROR;
    }

    dest2Buf  = GetByteImage(argv[3]);
    if (dest2Buf == NULL) {
    sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[3]);
    return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[4], &percent) != TCL_OK) {
        return TCL_ERROR;
    }  

    if ((ret = ByteEdgeDetectSobel(srcBuf, dest1Buf, dest2Buf, percent, &thresh1, &thresh2)) != DVM_VISION_OK) {
        switch (ret) {
        case DVM_DIFFERENT_SIZES:
            sprintf (interp->result, "%s: both byte images must have the same dimensions.", argv[0]);
            return TCL_ERROR;
        case DVM_BAD_PERCENT:
            sprintf (interp->result, "%s: percent must be between 0 and 100.", argv[0]);
            return TCL_ERROR;
        }
    }

    sprintf(interp->result, "%d %d", thresh1, thresh2);

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_edge_detect_canny srcbuf destbuf t_low t_high
 *
 * precond 
 *     Buffers have the same dimensions
 *
 * return 
 *     nothing
 *
 * side effect :
 *     destbuf contains results of the Canny edge detector, edges have a value
 *     of 255 and non-edges have a value of 0
 *
 *----------------------------------------------------------------------
 */

int
ByteEdgeDetectCannyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int t_low, t_high;
    int ret;

    /*
     * Check args, retrieve buffer from hash table.
     */

    if (argc != 5) {
        sprintf (interp->result,
                "wrong # args: should be %s src dest t_low t_high", argv[0]);
        return TCL_ERROR;
    }
    srcBuf  = GetByteImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s", argv[0], argv[1]);
        return TCL_ERROR;
    }
    destBuf = GetByteImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result,
                "%s: no such byte image %s", argv[0], argv[2]);
        return TCL_ERROR;
    }

    /*
     * Get the parameters
     */

    if ((Tcl_GetInt(interp, argv[3], &t_low) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[4], &t_high) != TCL_OK)) {
        return TCL_ERROR;
    }

    /*
     * Do the canny 
     */
    if ((ret = ByteEdgeDetectCanny(srcBuf, destBuf, t_low, t_high)) != DVM_VISION_OK) {
        switch (ret) {
        case DVM_ALLOC_ERROR:
            sprintf("%s: Memory allocation error.", argv[0]);
            return TCL_ERROR;
        case DVM_DIFFERENT_SIZES:
            sprintf("%s: both images must have the same dimensions.", argv[0]);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_make_from_threshold_8 srcbuf destbuf threshold lowVal
 *
 * precond 
 *     _srcbuf_ and _destbuf_ have the same dimensions
 *
 * return 
 *     nothing
 *
 * side effect :
 *     _destbuf_ will contain a value of '1' for each pixel greater than or equal to _threshold_ and '0' otherwise
 *
 *----------------------------------------------------------------------
 */

int
BitMakeFromThreshold8Cmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *srcBuf;
    BitImage *destBuf;
    int thresh, lowVal;
    int ret;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    if (argc != 5) {
    sprintf (interp->result,
             "wrong # args: should be %s srcbuf destbuf threshold lowVal", argv[0]);
    return TCL_ERROR;
    } 

    srcBuf  = GetByteImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    destBuf = GetBitImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &thresh) != TCL_OK) {
      return TCL_ERROR;
    } 

    if (Tcl_GetInt(interp, argv[4], &lowVal) != TCL_OK) {
      return TCL_ERROR;
    } 

    
    if ((ret = BitMakeFromThreshold8(srcBuf, destBuf, thresh, lowVal)) != DVM_VISION_OK)
    {
        switch(ret) {
        case DVM_BAD_THRESHOLD:
            sprintf(interp->result, "%s: threshold must fall in the range 0 to 256.", argv[0]);
            return TCL_ERROR;
        case DVM_NOT_BYTE_ALLIGNED:
            sprintf(interp->result, "%s: the destination buffer must be byte alligned, and the src buffer's width must be divisible by 8",
                argv[0]);
            return TCL_ERROR;
        case DVM_BAD_LOW_VAL:
            sprintf(interp->result, "%s: lowVal must equal 0 or 1.", argv[0]);
            return TCL_ERROR;
        }
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_compute_threshold srcbuf percent 
 *
 * precond 
 *     None
 *
 * return 
 *     threshold such that _percent_ of the pixels fall below that value
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int 
ByteComputeThresholdCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *srcBuf;
    int percent;
    int thresh;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    if (argc != 3) {
    sprintf (interp->result,
        "wrong # args: should be %s srcbuf percent", argv[0]);
    return TCL_ERROR;
    } 

    srcBuf  = GetByteImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &percent) != TCL_OK) {
      return TCL_ERROR;
    } 

    if ((thresh = ByteComputeThreshold(srcBuf, percent)) == DVM_BAD_PERCENT) {
        sprintf(interp->result, "%s: threshold must fall in the range 0 to 100.", argv[0]);
        return TCL_ERROR;
    }

    sprintf(interp->result, "%d", thresh);
    return 1;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_compare_blocks <srcbuf> <destbuf> <blocksize>
 *
 * precond 
 *     Buffers _srcbuf_ and _destbuf_ exist and have the same dimensions, 
 *     block size must be 8 or 16
 *
 * return 
 *     average absolute difference of "on" bits per block
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
BitCompareBlocksCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *srcBuf;
    BitImage *destBuf;
    int size;
    int retval;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    if (argc != 4) {
    sprintf (interp->result,
        "wrong # args: should be %s srcbuf destbuf block_size", argv[0]);
    return TCL_ERROR;
    } 

    srcBuf  = GetBitImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    destBuf = GetBitImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &size) != TCL_OK) {
      return TCL_ERROR;
    } 

    if ((retval = BitCompareBlocks(srcBuf, destBuf, size)) == DVM_BAD_BLOCK_SIZE) {
        sprintf(interp->result, "%s: block size must be 8 or 16.", argv[0]);
        return TCL_ERROR;
    }

    sprintf(interp->result, "%d", retval);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_dilate_8 <srcbuf> <destbuf>
 *
 * precond 
 *     Buffers _srcbuf_ and _destbuf_ exist and have the same dimensions, 
 *
 * return 
 *     nothing
 *
 * side effect :
 *     destBuf contains the dilated "1" bits of srcBuf
 *
 *----------------------------------------------------------------------
 */

int
BitDilate8Cmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *srcBuf, *destBuf;
    int retval;

   /*
    * Check args, retrieve buffer from hash table, parse rest of args
    */

    if (argc != 3) {
    sprintf(interp->result,
        "wrong # args: should be %s srcBuffer destBuffer", argv[0]);
    return TCL_ERROR;
    }

    srcBuf = GetBitImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    destBuf = GetBitImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }

    if ((retval = BitDilate8(srcBuf, destBuf)) != DVM_VISION_OK) {
        switch (retval) {
        case DVM_DIFFERENT_SIZES:
            sprintf (interp->result, "%s: buffers must have the same dimensions.", argv[0]);
            return TCL_ERROR;
        case DVM_NOT_BYTE_ALLIGNED:
            sprintf (interp->result, "%s: buffers must be byte alligned.", argv[0]);
            return TCL_ERROR;
        case DVM_BAD_HEIGHT:
            sprintf (interp->result, "%s: buffers must have height of at least 2.", argv[0]);
            return TCL_ERROR;
        case DVM_SCRATCH_SIZE_TOO_SMALL:
            sprintf(interp->result, "%s: constant SCRATCHSIZE in dvmvision.h is too small.", argv[0]);
            return TCL_ERROR;

        }
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_count_overlap <buf1> <buf2>
 *
 * precond 
 *     Buffers bit buffers must exist and be of the same dimensions
 *
 * return 
 *     percentage of '1' bits in _reference_ that are also '1' in _input_
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int
BitCountOverlapCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *refBuf, *inBuf;
    int percent;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    if (argc != 3) {
       sprintf(interp->result,
        "wrong # args: should be %s referenceBuffer inputBuffer", argv[0]);
       return TCL_ERROR;
    }

    refBuf = GetBitImage(argv[1]);
    if (refBuf == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    inBuf = GetBitImage(argv[2]);
    if (inBuf == NULL) {
         sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[2]);
         return TCL_ERROR;
    }

    if (BitCountOverlap(refBuf, inBuf, &percent) == DVM_USE_ALLIGN)
    {
        sprintf(interp->result, "%s: use %s_8", argv[0], argv[0]);
        return TCL_ERROR;
    }

    sprintf(interp->result, "%d", percent);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_count_overlap_8 <buf1> <buf2>
 *
 * precond 
 *     Buffers bit buffers must exist, be of the same dimensions, and be byte
 *     alligned
 *
 * return 
 *     percentage of '1' bits in _reference_ that are also '1' in _input_
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int
BitCountOverlap8Cmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *refBuf, *inBuf;
    int percent;
    int ret;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    if (argc != 3) {
       sprintf(interp->result,
        "wrong # args: should be %s referenceBuffer inputBuffer", argv[0]);
       return TCL_ERROR;
    }

    refBuf = GetBitImage(argv[1]);
    if (refBuf == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    inBuf = GetBitImage(argv[2]);
    if (inBuf == NULL) {
         sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[2]);
         return TCL_ERROR;
    }

    if ((ret = BitCountOverlap8(refBuf, inBuf, &percent)) != DVM_VISION_OK) {
        switch(ret) {
        case DVM_DIFFERENT_SIZES:
            sprintf(interp->result, "%s: both buffers must be the same size.", argv[0]);
            return TCL_ERROR;
        case DVM_NOT_BYTE_ALLIGNED:
            sprintf(interp->result, "%s: use bit_count_overlap.", argv[0]);
            return TCL_ERROR;
        }
    }

    sprintf(interp->result, "%d", percent);

    return TCL_OK;
}




int
ByteSmoothGaussianCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    double sigma;
    int ret;

    /*
    * Check args, retrieve buffer from hash table, parse rest of args
    */

    if (argc != 4) {
    sprintf (interp->result,
        "wrong # args: should be %s srcbuf destbuf sigma", argv[0]);
        return TCL_ERROR;
    }


    srcBuf  = GetByteImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    destBuf  = GetByteImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }


    if (Tcl_GetDouble(interp, argv[3], &sigma) != TCL_OK) {
        return TCL_ERROR;
    }

    if ((ret = ByteSmoothGaussian(srcBuf, destBuf, (float)sigma)) != DVM_VISION_OK) {
        switch (ret) {
        case DVM_DIFFERENT_SIZES:
            sprintf(interp->result, "%s: both byte images must have the same dimensions.", argv[0]);
            return TCL_ERROR;
        case DVM_ALLOC_ERROR:
            sprintf(interp->result, "%s: alloc error.", argv[0]);
            return TCL_ERROR;
        }
    }


    return TCL_OK;

}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_adaptive_threshold_8 srcbuf destbuf blockw blockh lowVal
 *
 * precond 
 *     _srcbuf_ and _destbuf_ have the same dimensions
 *
 * return 
 *     nothing
 *
 * side effect :
 *     _destbuf_ will contain a value of '1' for each pixel greater than or equal to _threshold_ and '0' otherwise
 *
 *----------------------------------------------------------------------
 */

int
BitAdaptiveThreshold8Cmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *srcBuf;
    BitImage *destBuf;
    int lowVal;
    int ret;
    int allWhite;
    int maxVariance;
    int blockw, blockh;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    if (argc != 8) {
    sprintf (interp->result,
             "wrong # args: should be %s srcbuf destbuf block_width block_height maxVariance allWhite lowVal", argv[0]);
    return TCL_ERROR;
    } 

    srcBuf  = GetByteImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    destBuf = GetBitImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &blockw) != TCL_OK) {
      return TCL_ERROR;
    } 

    if (Tcl_GetInt(interp, argv[4], &blockh) != TCL_OK) {
      return TCL_ERROR;
    } 

    if (Tcl_GetInt(interp, argv[5], &maxVariance) != TCL_OK) {
      return TCL_ERROR;
    } 

    if (Tcl_GetInt(interp, argv[6], &allWhite) != TCL_OK) {
      return TCL_ERROR;
    } 

    if (Tcl_GetInt(interp, argv[7], &lowVal) != TCL_OK) {
      return TCL_ERROR;
    } 
    
    if ((ret = BitAdaptiveThreshold8(srcBuf, destBuf, blockw, blockh, maxVariance, allWhite, lowVal)) != DVM_VISION_OK)
    {
        switch(ret) {
        case DVM_BAD_THRESHOLD:
            sprintf(interp->result, "%s: threshold must fall in the range 0 to 256.", argv[0]);
            return TCL_ERROR;
        case DVM_NOT_BYTE_ALLIGNED:
            sprintf(interp->result, "%s: the destination buffer must be byte alligned, and the src buffer's width must be divisible by 8",
                argv[0]);
            return TCL_ERROR;
        case DVM_BAD_LOW_VAL:
            sprintf(interp->result, "%s: lowVal must equal 0 or 1.", argv[0]);
            return TCL_ERROR;
        }
    }

    return TCL_OK;
}




/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_make_from_threshold_8 srcbuf destbuf threshold lowVal
 *
 * precond 
 *     _srcbuf_ and _destbuf_ have the same dimensions
 *
 * return 
 *     nothing
 *
 * side effect :
 *     _destbuf_ will contain a value of '1' for each pixel greater than or equal to _threshold_ and '0' otherwise
 *
 *----------------------------------------------------------------------
 */

int
ByteMakeFromThreshold8Cmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int thresh, lowVal;
    int ret;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    if (argc != 5) {
    sprintf (interp->result,
             "wrong # args: should be %s srcbuf destbuf threshold lowVal", argv[0]);
    return TCL_ERROR;
    } 

    srcBuf  = GetByteImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    destBuf = GetByteImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &thresh) != TCL_OK) {
      return TCL_ERROR;
    } 

    if (Tcl_GetInt(interp, argv[4], &lowVal) != TCL_OK) {
      return TCL_ERROR;
    } 

    
    if ((ret = ByteMakeFromThreshold8(srcBuf, destBuf, thresh, lowVal)) != DVM_VISION_OK)
    {
        switch(ret) {
        case DVM_BAD_THRESHOLD:
            sprintf(interp->result, "%s: threshold must fall in the range 0 to 256.", argv[0]);
            return TCL_ERROR;
        case DVM_BAD_LOW_VAL:
            sprintf(interp->result, "%s: lowVal must equal 0 or 1.", argv[0]);
            return TCL_ERROR;
        }
    }

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_erode_8 <srcbuf> <destbuf>
 *
 * precond 
 *     Buffers _srcbuf_ and _destbuf_ exist and have the same dimensions, 
 *
 * return 
 *     nothing
 *
 * side effect :
 *     destBuf contains the dilated "1" bits of srcBuf
 *
 *----------------------------------------------------------------------
 */

int
BitErode8Cmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *srcBuf, *destBuf;
    int retval;

   /*
    * Check args, retrieve buffer from hash table, parse rest of args
    */

    if (argc != 3) {
    sprintf(interp->result,
        "wrong # args: should be %s srcBuffer destBuffer", argv[0]);
    return TCL_ERROR;
    }

    srcBuf = GetBitImage(argv[1]);
    if (srcBuf == NULL) {
        sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
        return TCL_ERROR;
    }

    destBuf = GetBitImage(argv[2]);
    if (destBuf == NULL) {
        sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }

    if ((retval = BitErode8(srcBuf, destBuf)) != DVM_VISION_OK) {
        switch (retval) {
        case DVM_DIFFERENT_SIZES:
            sprintf (interp->result, "%s: buffers must have the same dimensions.", argv[0]);
            return TCL_ERROR;
        case DVM_NOT_BYTE_ALLIGNED:
            sprintf (interp->result, "%s: buffers must be byte alligned.", argv[0]);
            return TCL_ERROR;
        case DVM_BAD_HEIGHT:
            sprintf (interp->result, "%s: buffers must have height of at least 2.", argv[0]);
            return TCL_ERROR;
        case DVM_SCRATCH_SIZE_TOO_SMALL:
            sprintf(interp->result, "%s: constant SCRATCHSIZE in dvmvision.h is too small.", argv[0]);
            return TCL_ERROR;

        }
    }

    return TCL_OK;
}

