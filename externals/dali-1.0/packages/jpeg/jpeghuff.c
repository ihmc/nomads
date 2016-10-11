/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "jpegInt.h"

/*
 * Prototypes for local procedures defined in this file:
 */
static void CreateDCEncHufftab (unsigned char *bits,
        unsigned char *vals,
        unsigned short *hufcoPtr,
        unsigned short *hufsiPtr);

static void CreateACEncHufftab  (unsigned char *bits,
        unsigned char *vals,
        unsigned short *ehufco,
        unsigned short *ehufsi,
        unsigned char *acnbits);

JpegHuffTable *
JpegHuffTableNew(numOfComponents)
    int numOfComponents;
{
    JpegHuffTable *table;

    table = NEW(JpegHuffTable);
    table->dcCode = NEWARRAY(unsigned short, numOfComponents*4096);
    table->dcSymbol = NEWARRAY(unsigned short, numOfComponents*4096);
    table->acCode = NEWARRAY(unsigned short, numOfComponents*256);
    table->acSymbol = NEWARRAY(unsigned short, numOfComponents*256);
    table->acNumOfBits = NEWARRAY(unsigned char, numOfComponents*1024);
    table->numOfComponents = numOfComponents;

    return table;
}

void
JpegHuffTableInit(jpegHdr, scanHdr, huffTable)
    JpegHdr *jpegHdr;
    JpegScanHdr *scanHdr; 
    JpegHuffTable *huffTable;
{
    int i, id;

    for (i = 0; i < huffTable->numOfComponents; i++) {
        id = scanHdr->dcid[i];
        CreateDCEncHufftab(jpegHdr->ht[id].dcBits, 
            jpegHdr->ht[id].dcVals, 
            JpegHuffTableGetDcCodeTable(huffTable, i), 
            JpegHuffTableGetDcSymbolTable(huffTable, i)); 
        id = scanHdr->acid[i];
        CreateACEncHufftab(jpegHdr->ht[id].acBits, 
            jpegHdr->ht[id].acVals,
            JpegHuffTableGetAcCodeTable(huffTable, i), 
            JpegHuffTableGetAcSymbolTable(huffTable, i), 
            JpegHuffTableGetAcNumOfBitsTable(huffTable, i)); 
    }
}


void
JpegHuffTableFree(table)
    JpegHuffTable *table;
{
    free(table->dcCode);
    free(table->dcSymbol);
    free(table->acCode);
    free(table->acSymbol);
    free(table->acNumOfBits);
}

/*
 *--------------------------------------------------------------
 *
 * CreateDCEncHufftab
 *
 *    Convert a list of bits and values for a Huffman
 *    table in to the two tables: ehufco and ehusi which
 *    cab be used for encoding the DC coefficients
 *
 * Results:
 *    The two pointers are filled with the dynamically
 *    allocated tables
 *
 * Side effects:
 *    Memory is allocated for the tables
 *
 *--------------------------------------------------------------
 */
static void
CreateDCEncHufftab(bits, vals, xhufco, xhufsi)
    unsigned char *bits;
    unsigned char *vals;
    unsigned short *xhufco;
    unsigned short *xhufsi;
{
    int symbols, sizes[256], codes[256], code, i, j;
    unsigned short ehufco[256];
    unsigned char ehufsi[256];
    unsigned short *cp, *sp;
    int diff, step, limit;

    /* 
     * Count the total number of symbols
     * and generate the codes
     */
    symbols = 0;
    code = 0;
    for (i=0; i<16; i++) {
        for (j=0; j<(int)(bits[i]); j++) {
            codes[symbols] = code++;
            sizes[symbols] = i + 1;
            symbols++;
        }
        code <<= 1;
    }

    memset(xhufco, 0, 4096 * sizeof(short) );
    memset(xhufsi, 0, 4096 * sizeof(short) );

    /*
     * Generate intermediate encoding tables
     * These are code and size indexed by symbol value
    memset(ehufsi, 0, sizeof(ehufsi));
    memset(ehufco, 0, sizeof(ehufco));
     */

    for (i = 0; i < symbols; i++) {
        ehufco[vals[i]] = codes[i];
        ehufsi[vals[i]] = sizes[i];
    }
    
    /*
     * Generate the values in the final table
     */

    cp = xhufco + 2047;
    sp = xhufsi + 2047;

    /* The zero coefficient */
    *sp++ = ehufsi[0];
    *cp++ = 0;

    /* The positive DC coefficients */
    limit = 2;
    diff = 1;
    for (step = 1; step <12; step++) {
        while (diff < limit) {
            *sp++ = ehufsi[step] + step;
            *cp++ = (diff & bitmask[step]) | (ehufco[step] << step);
            diff++;
        }
        limit *= 2;
    }
    /* And the negative DC coefficients */
    cp = xhufco + 2046;
    sp = xhufsi + 2046;
    limit = -2;
    diff = -1;
    for (step = 1; step <12; step++) {
        while (diff > limit) {
            *sp-- = ehufsi[step] + step;
            *cp-- = ((diff-1) & bitmask[step]) | (ehufco[step] << step);
            diff--;
        }
        limit *= 2;
    }
        
}

/*
 *--------------------------------------------------------------
 *
 * CreateACEncHufftab
 *
 *    Convert a list of bits and values for a Huffman
 *    table in to the two tables: ehufco and ehusi which
 *    cab be used for encoding AC coefficients
 *
 * Results:
 *    The two pointers are filled with the dynamically
 *    allocated tables
 *
 * Side effects:
 *    Memory is allocated for the tables
 *
 *--------------------------------------------------------------
 */
static void
CreateACEncHufftab(bits, vals, ehufco, ehufsi, acnbits)
    unsigned char *bits;
    unsigned char *vals;
    unsigned short *ehufco;
    unsigned short *ehufsi;
    unsigned char *acnbits;
{
    int symbols, sizes[256], codes[256], code, i, j;
    int ac, step, limit;

    /* 
     * Count the total number of symbols
     * and generate the codes
     */
    symbols = 0;
    code = 0;
    for (i=0; i<16; i++) {
        for (j=0; j<(int)(bits[i]); j++) {
            codes[symbols] = code++;
            sizes[symbols] = i + 1;
            symbols++;
        }
        code <<= 1;
    }

    /*
     * Generate hufco encoding tables
     * These are code and size indexed by symbol value
    memset(ehufsi, 0, sizeof(ehufsi));
    memset(ehufco, 0, sizeof(ehufco));
     */

    for (i = 0; i < symbols; i++) {
        ehufco[vals[i]] = codes[i];
        ehufsi[vals[i]] = sizes[i];
    }
    
    /* Fill in the nbits table */
    *acnbits++ = 0;
    ac = 1;
    limit = 2;
    for (step = 1; step < 11; step++) {
        while (ac < limit) {
            *acnbits++ = step;
            ac++;
        }
        limit *= 2;
    }
}


