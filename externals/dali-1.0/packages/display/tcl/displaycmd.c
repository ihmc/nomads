/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tk.h"
#include "tclDvmDisplay.h"

#if TK_MAJOR_VERSION == 8
#define NEWTK
#else
#undef NEWTK
#endif

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     photo_make_from_rgb r g b photo
 * precond 
 *     sizes must match
 * return 
 *     none
 * side effect :
 *     The photo image is filled with the contents of r,g,b
 *
 *----------------------------------------------------------------------
 */
int
RgbToPhotoCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Tk_PhotoHandle ph;
    Tk_PhotoImageBlock block;
    ByteImage *r, *g, *b;
    int w, h, stride;

    ReturnErrorIf1 (argc != 5,
        "wrong # args: should be %s r g b photo", argv[0]);

    r = GetByteImage (argv[1]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    g = GetByteImage (argv[2]);
    ReturnErrorIf2 (g == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    b = GetByteImage (argv[3]);
    ReturnErrorIf2 (b == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

#ifndef NEWTK
    ph = Tk_FindPhoto (argv[4]);
#else
    ph = Tk_FindPhoto (interp,argv[4]);
#endif
    ReturnErrorIf2 (ph == NULL,
        "%s: no such photo image %s", argv[0], argv[4]);

    w = r->width;
    h = r->height;
    stride = r->parentWidth;
    ReturnErrorIf1((w != g->width)||(w != b->width) ||
                   (h != g->height)||(h != b->height),
        "%s: mismatched dimensions", argv[0]);

    ReturnErrorIf1((stride != g->parentWidth)||(stride != b->parentWidth),
        "%s: mismatched strides", argv[0]);

    block.width = w;
    block.height = h;
    block.pixelPtr = r->firstByte;
    block.pitch = stride;
    block.pixelSize = sizeof(unsigned char);
    block.offset[0] = 0;
    block.offset[1] = (int)(g->firstByte - r->firstByte);
    block.offset[2] = (int)(b->firstByte - r->firstByte);

    Tk_PhotoPutBlock (ph, &block, 0, 0, w, h);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_to_photo byte photo
 * precond 
 *     sizes must match
 * return 
 *     none
 * side effect :
 *     photo is loaded with the contents of <y>
 *
 *----------------------------------------------------------------------
 */
int
ByteToPhotoCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Tk_PhotoHandle ph;
    Tk_PhotoImageBlock block;
    ByteImage *y;
    int w, h, stride;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s byte photo", argv[0]);

    y = GetByteImage (argv[1]);
    ReturnErrorIf2 (y == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

#ifndef NEWTK
    ph = Tk_FindPhoto (argv[2]);
#else
    ph = Tk_FindPhoto (interp,argv[2]);
#endif
    ReturnErrorIf2 (ph == NULL,
        "%s: no such photo image %s", argv[0], argv[2]);

    w = y->width;
    h = y->height;
    stride = y->parentWidth;

    block.width = w;
    block.height = h;
    block.pixelPtr = y->firstByte;
    block.pixelSize = sizeof(unsigned char);
    block.pitch = stride;
    block.offset[0] = block.offset[1] = block.offset[2] = 0;

    Tk_PhotoPutBlock (ph, &block, 0, 0, w, h);
    return TCL_OK;
}
