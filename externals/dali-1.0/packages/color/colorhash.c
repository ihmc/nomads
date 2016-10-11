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
 * colorhash.c
 *
 * This is an implementation of a hash table with fixed size, using open
 * addressing method.  Only insertion and retrival is implemented, since
 * I don't see any use of deletion.  The size of each hash table is power
 * of two (so that we can use double hashing), this also makes computing
 * % pretty fast.
 */

#include "colorInt.h"

extern unsigned int table33023[];
extern unsigned int table30013[];
extern unsigned int table27011[];

ColorHashTable *
ColorHashTableNew(size)
    int size;
{
    ColorHashTable *table;

    table = NEW(ColorHashTable);
    table->size  = 1 << size;
    table->table = NEWARRAY(ColorHashEntry, (table->size));
    return table;
}

void
ColorHashTableFree(table)
    ColorHashTable *table;
{
    FREE(table);
}

int
ColorHashTableAdd(table, r, g, b, value)
    ColorHashTable *table;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    int value;
{
    int hash;
    ColorHashEntry *t;
    Color color;
    int m, n;
    int i;

    t = table->table;
    color.unpack.tag = 1; /* indicates element is non-empty */
    color.unpack.r = r;
    color.unpack.g = g;
    color.unpack.b = b;
    m = (table33023[color.unpack.r] + 
         table30013[color.unpack.g] + 
         table27011[color.unpack.b]);
    hash = m & (table->size - 1);
    n = ((m % (table->size - 1)) << 1) + 1;
    for (i = 0; i < table->size; i++) {
        hash = (hash + n) & (table->size - 1);
        if (t[hash].color.pack == 0) { 
            /* it's empty. occupy it. */
            t[hash].color = color; 
            t[hash].value = value;
            table->numOfEntry++;
            return DVM_COLOR_HASH_TABLE_OK;
        } else if (t[hash].color.pack == color.pack) {
            t[hash].value = value;
            return DVM_COLOR_HASH_TABLE_OK;
        }
    }
    return DVM_COLOR_HASH_TABLE_FULL;
}


int
ColorHashTableFind(table, r, g, b, index, value)
    ColorHashTable *table;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    int *index;
    int *value;
{
    int mods, m, n, times, hash;
    ColorHashEntry *t;
    Color color;

    /*count = 0;*/
    t = table->table;
    times = table->size;
    mods = table->size - 1;
    color.unpack.tag = 1;
    color.unpack.r = r;
    color.unpack.g = g;
    color.unpack.b = b;
    // hash = color.pack & mods;
    m = (table33023[color.unpack.r] + 
         table30013[color.unpack.g] + 
         table27011[color.unpack.b]);
    hash = m & (table->size - 1);
    n = ((m % (table->size - 1)) << 1) + 1;
    DO_N_TIMES(times,
        hash = (hash + n) & mods;
        /*count++;*/
        if (t[hash].color.pack == 0) { 
            /*sum += count; calls++; printf("avarage probe : %f times\n",
             *sum/(float)calls);*/
            *index = hash;
            return DVM_COLOR_NOT_FOUND;
        } else if (t[hash].color.pack == color.pack) {
            /*sum += count; calls++; printf("avarage probe : %f times\n",
             *sum/(float)calls);*/
            *value = t[hash].value;
            *index = hash;
            return DVM_COLOR_HASH_TABLE_OK;
        }
    );
    /*sum += count; calls++; printf("avarage probe : %f times\n",
     *sum/(float)calls);*/
    return DVM_COLOR_HASH_TABLE_FULL;
}

void
ColorHashTableSet(table, index, value)
    ColorHashTable *table;
    int index;
    int value;
{
    table->table[index].value = value;
}


int
ColorHashTableAddAt(table, index,  r, g, b, value)
    ColorHashTable *table;
    int index;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    int value;
{
    ColorHashEntry *t;
    Color color;

    t = table->table;
    color.unpack.tag = 1;
    color.unpack.r = r;
    color.unpack.g = g;
    color.unpack.b = b;
    t[index].color = color; 
    t[index].value = value;
    table->numOfEntry++;
    return DVM_COLOR_HASH_TABLE_OK;
}


void
ColorHashTablePackSelf(src)
    ColorHashTable *src;
{
    ColorHashEntry *currSrc, *currDest;
    int times;

    currSrc = src->table;
    currDest = src->table;

    times = 0;
    while (currSrc->color.pack != 0 && times < src->size) {
        currSrc++;
        times++;
    }

    currDest = currSrc;
    while (times < src->size) {
        while (currSrc->color.pack == 0 && times < src->size) {
            currSrc++;
            times++;
        }
        *currDest = *currSrc;
        currDest++;
        currSrc++;
        times++;
    }
}


void
ColorHashTableClear(table)
    ColorHashTable *table;
{
    int size = table->size;
    ColorHashEntry *curr;

    curr = table->table;
    DO_N_TIMES(size,
        curr->color.pack = 0; /* set tag to 0 to indicate empty */
        curr->value = 0;
        curr++;
        );
    table->numOfEntry = 0;
}


void 
ColorHashTablePrint(table)
    ColorHashTable *table;
{
    int size = table->size;
    ColorHashEntry *curr;

    curr = table->table;
    DO_N_TIMES(size,
        if (curr->color.pack != 0) {
            printf("%d %d\n", curr->color.pack, curr->value);
        }
        curr++;
        );
}
