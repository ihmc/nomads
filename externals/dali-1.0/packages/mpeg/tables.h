/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */

extern int default_intra_quantizer_table[];

extern int absTable[];
extern int squaresTable[];

#define HUFF_MAXRUN     32
#define HUFF_MAXLEVEL   41

extern int huff_maxlevel[];
extern unsigned long *huff_table[];
extern int *huff_bits[];
