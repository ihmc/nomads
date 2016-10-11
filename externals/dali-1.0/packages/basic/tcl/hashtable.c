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

#define BUF_PREFIX_LEN  9
#define GET_BUF_NUM(s) atoi(s + BUF_PREFIX_LEN)
#define PUT_BUF_NUM(str,prefix,buf) sprintf(str, "%s%d", prefix, buf)

static Tcl_HashTable thePtrTable;       /* maps ids to pointers */
static Tcl_HashTable theKeyTable;       /* maps pointers to Tcl names */
static int currId;
static int inited = 0;

/*
 *----------------------------------------------------------------------
 * InitHashTable
 * 
 *     Create and initialize the hash tables
 *----------------------------------------------------------------------
 */
void
InitHashTable(interp)
    Tcl_Interp *interp;
{
    if (!inited) {
        Tcl_InitHashTable(&thePtrTable, TCL_ONE_WORD_KEYS);
        Tcl_InitHashTable(&theKeyTable, TCL_ONE_WORD_KEYS);
        currId = 0;
        inited = 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetBuf  --
 *
 * Lookup the buffer with the name passed in in the hash table.
 *
 * return 
 *     Pointer to the buffer.
 * 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
void *
GetBuf(str)
    char *str;
{
    Tcl_HashEntry *hashEntry;
    int buffer;

    buffer = GET_BUF_NUM(str);
    hashEntry = Tcl_FindHashEntry(&thePtrTable, (char *) buffer);
    if (hashEntry == NULL) {
        return NULL;
    }
    return (void *) Tcl_GetHashValue(hashEntry);
}

/*
 *----------------------------------------------------------------------
 *
 * FindBuf  --
 *
 * Lookup the name associated with the buffer passed in in the hash table.
 *
 * return 
 *     A read-only pointer to the string with the name
 * 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
char *
FindBuf(ptr)
    void *ptr;
{
    Tcl_HashEntry *hashEntry;

    hashEntry = Tcl_FindHashEntry(&theKeyTable, (char *) ptr);
    if (hashEntry == NULL) {
        return NULL;
    }
    return (char *) Tcl_GetHashValue(hashEntry);
}


/*
 *----------------------------------------------------------------------
 *
 * RemoveBuf  --
 *
 * Removes the buffer with the name passed in from the hash table.
 *
 * return 
 *     Pointer to the buffer.
 * 
 * side effect :
 *     The buffer is removed from thePtrTable
 *
 *----------------------------------------------------------------------
 */

void *
RemoveBuf(tclStr)
    char *tclStr;
{
    Tcl_HashEntry *hashEntry;
    void *ptr;
    char *str;
    int id;

    id = GET_BUF_NUM(tclStr);
    hashEntry = Tcl_FindHashEntry(&thePtrTable, (char *) id);
    if (hashEntry == NULL) {
        return NULL;
    }
    ptr = (void *) Tcl_GetHashValue(hashEntry);
    Tcl_DeleteHashEntry(hashEntry);

    hashEntry = Tcl_FindHashEntry(&theKeyTable, (char *) ptr);
    if (hashEntry == NULL) {
        return NULL;
    }
    str = (void *) Tcl_GetHashValue(hashEntry);
    Tcl_DeleteHashEntry(hashEntry);

    return ptr;
}

/*
 *----------------------------------------------------------------------
 *
 * PutBuf  --
 *
 * precond 
 *     none
 *
 * return 
 *     the id number, and its Tcl name (in interp->result)
 * 
 * side effect :
 *     a new struct and name are added to thePtrTable and theKeyTable
 *
 *----------------------------------------------------------------------
 */
int
PutBuf(interp, prefix, ptr)
    Tcl_Interp *interp;
    char *prefix;
    void *ptr;
{
    Tcl_HashEntry *hashEntry;
    int new;
    int id;
    char *str;

    /*
     * Store the mapping from id to pointer
     */
    hashEntry = Tcl_CreateHashEntry(&thePtrTable, (char *) currId, &new);
    if (!new) {
        return -1;
    }
    Tcl_SetHashValue(hashEntry, (ClientData) ptr);
    id = currId++;

    /*
     * Compute the name in Tcl, and make a copy of it in str
     */
    sprintf(interp->result, "%s%d", prefix, id);
    str = MALLOC(strlen(interp->result) + 1);
    strcpy(str, interp->result);

    /*
     * Store the mapping from pointer ptr to Tcl name.
     */
    hashEntry = Tcl_CreateHashEntry(&theKeyTable, (char *) ptr, &new);
    Tcl_SetHashValue(hashEntry, (ClientData) str);

    return id;
}

/*
 * The XXXByNum FUnctions.
 * These are the same as the above, except that we pass in the id only,
 * instead of the whole string.  These are originally used in Mash, but
 * it is a handy function to have.
 */
void *
RemoveBufByNum(id)
    int id;
{
    Tcl_HashEntry *hashEntry;
    void *ptr;
    char *str;

    hashEntry = Tcl_FindHashEntry(&thePtrTable, (char *) id);
    if (hashEntry == NULL) {
        return NULL;
    }
    ptr = (void *) Tcl_GetHashValue(hashEntry);
    Tcl_DeleteHashEntry(hashEntry);

    hashEntry = Tcl_FindHashEntry(&theKeyTable, (char *) ptr);
    if (hashEntry == NULL) {
        return NULL;
    }
    str = (void *) Tcl_GetHashValue(hashEntry);
    Tcl_DeleteHashEntry(hashEntry);

    return ptr;
}


void *
GetBufByNum(id)
    int id;
{
    Tcl_HashEntry *hashEntry;

    hashEntry = Tcl_FindHashEntry(&thePtrTable, (char *) id);
    if (hashEntry == NULL) {
        return NULL;
    }
    return (void *) Tcl_GetHashValue(hashEntry);
}
