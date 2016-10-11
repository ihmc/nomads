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
 * kernel.c
 *
 * Defines allocation and initialization of
 * convolution kernels.
 *
 *----------------------------------------------
 */

#include "tclDvmKernel.h"


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_new <width> <height>
 *
 * precond
 *     None
 *
 * return
 *     The kernel number
 * 
 * side effect
 *     A new kernel is created and is added into _theKernelTable_
 *     Its width and height are set,
 *     and the values and offset are inited to 0, the divfactor to 1.
 *
 *----------------------------------------------------------------------
 */

int
KernelNewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;
    int width, height;

    /*
     * Check & Parse args
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s height width", argv[0]);

    if (Tcl_GetInt(interp, argv[1], &width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &height) != TCL_OK) {
        return TCL_ERROR;
    }

    kern = KernelNew(width,height);
    PutKernel(interp, kern);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_free <kern>
 *
 * precond
 *     Kernel _kern_ exists
 *
 * return
 *     None
 * 
 * side effect
 *     The memory allocated for _kern_ is freed.
 *
 *----------------------------------------------------------------------
 */

int
KernelFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;

    /*
     * Check args, retrieve kernel from table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s kern", argv[0]);

    kern = RemoveKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such convolution kernel %s", argv[0], argv[1]);

    /*
     * Free memory associated with this kernel
     */
    KernelFree(kern);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_set <kern> [-values <vals>]
 *                       [[-divfactor <divfactor>] [-offset <offset>] | -defaults]
 *
 * precond
 *     Kernel _kern_ exists
 *     and _vals_ is a list of lists matching the size of _kern_
 *
 * return
 *     None
 * 
 * side effect
 *     Given fields of kernel _kern_ are set
 *
 *----------------------------------------------------------------------
 */

int
KernelSetValuesCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;
    char **kernelArgv, **rowArgv;
    int height, row, width, col;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s kernel valueList", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such convolution kernel %s", argv[0], argv[1]);

    if (Tcl_SplitList(interp, argv[2], &height, &kernelArgv) != TCL_OK) {
        return TCL_ERROR;
    }

    ReturnErrorIf2 (height != kern->height,
        "%s: input height does not match %s height", argv[0], argv[1]);

    /*
     * For each row, extract individual values from the row 
     * (which is a list).  Check length of rows.
     */

    for (row=0; row<height; row++) {

        if (Tcl_SplitList(interp, kernelArgv[row], &width, &rowArgv) != TCL_OK) 
            return TCL_ERROR;

        ReturnErrorIf2 (width != kern->width,
                "%s: row %d of input has wrong width", argv[0], row);

        for (col=0; col<width; col++) {
            if (Tcl_GetInt(interp, rowArgv[col], &kern->vals[row][col]) != TCL_OK)
                return TCL_ERROR;
        }
    }
    FREE(rowArgv);
    FREE(kernelArgv);

    return TCL_OK;
}


int
KernelSetDivFactorCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;
    int status;
    
    ReturnErrorIf1 (argc != 3,
        "wrong # args : %s kern divfactor", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such convolution kernel %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &kern->divfactor);
    ReturnErrorIf (status != TCL_OK);

    return TCL_OK;
}


int
KernelSetOffsetCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;
    int status;
    
    ReturnErrorIf1 (argc != 3,
        "wrong # args : %s kern offset", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such convolution kernel %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &kern->offset);
    ReturnErrorIf (status != TCL_OK);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_get_width <kern>
 *
 * precond
 *     Kernel _kern_ exists
 *
 * return
 *     Width of the kernel
 * 
 * side effect
 *     None
 *
 *----------------------------------------------------------------------
 */

int
KernelGetWidthCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;

    /*
     * Check args, retrieve kernel from table.
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s kern", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", KernelGetWidth(kern));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_get_height <kern>
 *
 * precond
 *     Kernel _kern_ exists
 *
 * return
 *     Height of the kernel
 * 
 * side effect
 *     None
 *
 *----------------------------------------------------------------------
 */

int
KernelGetHeightCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;

    /*
     * Check args, retrieve kernel from table.
     */

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s kern", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", KernelGetHeight(kern));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_get_values <kern>
 *
 * precond
 *     Kernel _kern_ exists
 *
 * return
 *     Values of the kernel, as a list of lists (rows).
 * 
 * side effect
 *     None
 *
 *----------------------------------------------------------------------
 */

int
KernelGetValuesCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;
    char **valrow;
    char **outvals;
    int row, col;
    int size = 20;

    /*
     * Check args, retrieve kernel from table.
     */

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s kern", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[1]);

    /*
     * Allocate temp local strings
     */

    outvals = NEWARRAY (char *, kern->height);
    valrow = NEWARRAY (char *, kern->width);
    col = 0;
    DO_N_TIMES(kern->width,
        valrow[col++] = NEWARRAY (char, size);
    );

    /*
     * Retrieve values from kernel and merge them into list strings
     */

    for (row=0; row< kern->height; row++) {
        col = 0;
        DO_N_TIMES(kern->width,
            sprintf (valrow[col], "%d", kern->vals[row][col]);
            col++;
        );
        outvals[row] = Tcl_Merge(kern->width, valrow);
    }

    interp->result = Tcl_Merge(kern->height, outvals);
    interp->freeProc = (Tcl_FreeProc *) free;

    /*
     * Free all the temp local memory
     */

    row = 0;
    DO_N_TIMES(kern->height,
        FREE(outvals[row++]);
    );
    FREE(outvals);

    col = 0;
    DO_N_TIMES(kern->width,
        FREE(valrow[col++]);
    );
    FREE(valrow);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_get_divfactor <kern>
 *
 * precond
 *     Kernel _kern_ exists
 *
 * return
 *     Division factor of the kernel
 * 
 * side effect
 *     None
 *
 *----------------------------------------------------------------------
 */

int
KernelGetDivFactorCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;

    /*
     * Check args, retrieve kernel from table.
     */

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s kern", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", KernelGetDivFactor(kern));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage
 *     kernel_get_offset <kern>
 *
 * precond
 *     Kernel _kern_ exists
 *
 * return
 *     Offset of the kernel
 * 
 * side effect
 *     None
 *
 *----------------------------------------------------------------------
 */

int
KernelGetOffsetCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Kernel *kern;

    /*
     * Check args, retrieve kernel from table.
     */

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s kern", argv[0]);

    kern = GetKernel(argv[1]);
    ReturnErrorIf2 (kern == NULL,
        "%s: no such color convolution kernel %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", KernelGetOffset(kern));
    return TCL_OK;
}
