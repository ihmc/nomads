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
 * commands.c
 *
 * Wei Tsang Mar 97
 *
 * contains general functions for initializing the program. (adding
 * commands and creating hash table)
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmBasic.h"

/*
 *----------------------------------------------------------------------
 * CreateCommands
 * 
 *     Add all commands into Tcl interpreter
 *----------------------------------------------------------------------
 */

void
CreateCommands(interp, cmd, cmdsize)
    Tcl_Interp *interp;
    Commands cmd[];
    int cmdsize;
{
    int i, num_of_cmds;

    num_of_cmds = cmdsize / sizeof(Commands);
    for (i = 0; i < num_of_cmds; i++) {
        Tcl_CreateCommand(interp, cmd[i].name, cmd[i].proc, cmd[i].cd,
            cmd[i].delProc);
    }
}
