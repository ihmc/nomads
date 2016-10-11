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
 *----------------------------------------------------------------------
 *
 * map.c
 *
 * Functions that manipulate ImageMaps
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmImap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_new
 *
 * precond 
 *     none
 *
 * return 
 *     the buffer number
 * 
 * side effect :
 *     a new buffer is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */

int
ImageMapNewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    ImageMap *buf;

    ReturnErrorIf1 (argc != 1,
        "wrong #args: should be %s ", argv[0]);

    buf = ImageMapNew();
    PutImageMap(interp,buf);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_init <imagemap> <values>
 *
 * precond 
 *     none
 *
 * return 
 *     none
 * 
 * side effect :
 *     initializes _imagemap_ with _values_
 *
 *----------------------------------------------------------------------
 */

int
ImageMapInitCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];


{
    ImageMap *map;
    int listArgc, listValue,i;
    char **listArgv,**argvPos;
    unsigned char *arrayPos;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s imageMap values", argv[0]);

    map = GetImageMap(argv[1]);
    ReturnErrorIf2 (map == NULL,
        "%s: no such image map %s", argv[0], argv[1]);

    if (Tcl_SplitList(interp, argv[2], &listArgc, &listArgv) != TCL_OK) {
        sprintf (interp->result, "%s: 2nd argument must be of type list",
                argv[0]);
        return TCL_ERROR;
    }

    if (listArgc != 256) {
        if (listArgc == 32768)
            sprintf (interp->result, 
                    "%s:list too long:RGB ImageMaps have not been implemented",
                    argv[0]);
        else
            sprintf (interp->result, 
                    "%s: list should have exactly 256 elements", argv[0]);
        return TCL_ERROR;
    }

    /*
     * parse the TCL list into an array of unsigned chars and write it into
     * the image map
     */
    argvPos = listArgv;
    arrayPos = map->table;
    for (i=1;i<=256;i++) {
        Tcl_GetInt(interp, *argvPos++, &listValue);
        /*
         * check for illegitimate values
         */
        if ((listValue < 0) || (listValue > 255)) {
            sprintf (interp->result, "List values must be between 0 and 255");
            ckfree((char *)map);
            return TCL_ERROR;
        }
        *arrayPos++ = (unsigned char)listValue;
    }

    /*
     * There is no need to call ImageMapInit since this function directly
     * reads into the ImageMap. 
     */
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_free <buf>
 *
 * precond 
 *      _buf_ exists
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the memory allocated for _buf_ is freed.
 *
 *----------------------------------------------------------------------
 */

int
ImageMapFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ImageMap *buf;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s imageMap", argv[0]);

    buf = RemoveImageMap(argv[1]);
    ReturnErrorIf2 (buf == NULL,
        "%s: no such image map %s", argv[0], argv[1]);

    ImageMapFree(buf);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_copy <srcbuf> <destbuf>
 *
 * precond 
 *      _srcbuf_ exists
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *     a duplicate copy of buf is created and put into the hashtable
 *
 *----------------------------------------------------------------------
 */

int
ImageMapCopyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    ImageMap *src, *dest;

    ReturnErrorIf1 (argc != 3,
        "wrong #args, should be %s srcMap destMap", argv[0]);

    src = GetImageMap(argv[1]);
    ReturnErrorIf2 (src == NULL,
        "%s: no such image map %s", argv[0], argv[1]);
    dest = GetImageMap(argv[2]);
    ReturnErrorIf2 (dest == NULL,
        "%s: no such image map %s", argv[0], argv[2]);

    ImageMapCopy(src, dest);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_get_values <buf>
 *
 * precond 
 *      _buf_ exists
 *      
 * return 
 *     returns a list of the values in _buf_
 * 
 * side effect :
 *     none
 *
 * Comment :
 *     there is no parallel c function
 *----------------------------------------------------------------------
 */

int
ImageMapGetValuesCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{

    int i;
    ImageMap *map;
    char str[256];
    unsigned char *value;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s imageMap", argv[0]);

    map = GetImageMap(argv[1]);
    ReturnErrorIf2 (map == NULL,
        "%s: no such image map %s", argv[0], argv[1]);

    value = ImageMapGetValues(map);
    Tcl_ResetResult(interp);
    for (i=1;i<=256;i++) {
        sprintf (str, "%d", value[i]);
        Tcl_AppendResult (interp, str, NULL);
    }
        
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_compose <buf1> <buf2>
 *
 * precond 
 *      _buf1_ and _buf2_ exists
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *     a new map is created by composing buf1 with buf2 and
 *       is entered into the _theBufferTable. The composition is 
 *       equivalent to applying buf1 and then buf2.
 *
 *----------------------------------------------------------------------
 */

int
ImageMapComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ImageMap *buf1,*buf2,*newbuf;

    ReturnErrorIf1 (argc != 4,
        "wrong #args, should be %s map1 map2 dest", argv[0]);

    buf1 = GetImageMap(argv[1]);
    ReturnErrorIf2 (buf1 == NULL,
        "%s: no such image map %s", argv[0], argv[1]);

    buf2 = GetImageMap(argv[2]);
    ReturnErrorIf2 (buf2 == NULL,
        "%s: no such image map %s", argv[0], argv[2]);

    newbuf = GetImageMap(argv[3]);
    ReturnErrorIf2 (newbuf == NULL,
        "%s: no such image map %s", argv[0], argv[3]);

    ImageMapCompose(buf1,buf2, newbuf);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_apply <map> <srcimage> <destimage>
 * precond 
 *      _map_ and _srcimage_ and _destimage_ exists
 * return 
 *     none
 * side effect :
 *     _map_ is applied to the _srcimage_ and the result is stored
 *       in dest image
 *
 *----------------------------------------------------------------------
 */

int
ImageMapApplyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    ImageMap *map;
    ByteImage *srcImage, *destImage;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1 (argc != 4,
        "wrong #args, should be %s <map> <srcimage> <destimage>", argv[0]);

    map = GetImageMap(argv[1]);
    ReturnErrorIf2 (map == NULL,
        "%s: no such image map %s", argv[0], argv[1]);

    srcImage = GetByteImage(argv[2]);
    ReturnErrorIf2 (srcImage == NULL,
        "%s: no such image %s", argv[0], argv[2]);

    destImage = GetByteImage(argv[3]);
    ReturnErrorIf2 (destImage == NULL,
        "%s: no such image %s", argv[0], argv[3]);

    ImageMapApply(map,srcImage,destImage);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_init_histo_equal <image>
 *
 * precond 
 *      _image_ exists
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *        _image_ is analyzed to create a histogram equalizing image map       
 *
 *----------------------------------------------------------------------
 */

int
ImageMapInitHistoEqualCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    ByteImage *image;
    ImageMap *map;

    ReturnErrorIf1 (argc != 3,
        "wrong #args, should be %s byteImage map" , argv[0]);

    image = GetByteImage(argv[1]);
    ReturnErrorIf2 (image == NULL,
        "%s: no such image %s", argv[0], argv[1]);
    map = GetImageMap(argv[2]);
    ReturnErrorIf2 (map == NULL,
        "%s: no such image map %s", argv[0], argv[2]);

    ImageMapInitHistoEqual(image, map);
    return TCL_OK;
}

    
/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     imagemap_init_identity 
 *
 * precond 
 *      none
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *        creates a identity map
 *
 *----------------------------------------------------------------------
 */


int
ImageMapInitIdentityCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    ImageMap *map;

    ReturnErrorIf1 (argc != 2,
        "wrong # args : should be %s imageMap", argv[0]);

    map = GetImageMap(argv[1]);
    ReturnErrorIf2 (map == NULL,
        "%s: no such image map %s", argv[0], argv[1]);

    ImageMapInitIdentity(map);
    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     map_init_inverse
 *
 * precond 
 *      none
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *        creates an inverse identity map
 *
 *----------------------------------------------------------------------
 */


int
ImageMapInitInverseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    ImageMap *map;

    ReturnErrorIf1 (argc != 2,
        "wrong #args : should be %s imageMap", argv[0]);

    map = GetImageMap(argv[1]);
    ReturnErrorIf2 (map == NULL,
        "%s: no such image map %s", argv[0], argv[1]);

    ImageMapInitInverse(map);

    return TCL_OK;
}
