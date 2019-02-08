/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef DVM_COLOR_H
#define DVM_COLOR_H

#include "dvmbasic.h"
#include "dvmimap.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Union Color
 *
 * An r, g, b color can be either packed (as a 4 byte integer)
 * or unpacked (as three unsigned color).  Packed color is useful
 * for comparison and assignment.  Unpacked is used when we need
 * to access individual r, g, b color.  The tag field can be used
 * in different way up to the programmer.  For example, in a hash
 * table, tag == 1 if r, g, b (pack != 0) are valid and tag == 0x00
 * (pack == 0) if r, g, b are empty.  (Note that this works in both
 * little-endian and big-endian machine.)
 *
 */
    typedef struct UnpackColor {
        unsigned char tag;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    } UnpackColor;

    typedef union Color {
        UnpackColor unpack;
        int pack;
    } Color;

/*
 * Type ColorHashEntry
 *
 * An entry in the ColorHashTable. Consists a the key color, an a
 * value.  The value can be used as counter, index etc.
 */
    typedef struct ColorHashEntry {
        Color color;            /* 0 if empty, otherwise it's a pack of r, g, b */
        int value;              /* value for this hash table (can be counter, index etc) */
    } ColorHashEntry;

    typedef struct ColorHashTable {
        int size;
        int numOfEntry;
        ColorHashEntry *table;
    } ColorHashTable;

    typedef struct VpNode {
        unsigned char index;
        unsigned char left;
        unsigned char right;
        float ll, lu, rl, ru, mu;
    } VpNode, *VpTree;

#define ColorHashTableGetSize(x)  (x)->size
#define ColorHashTableGetNumOfEntry(x)  (x)->numOfEntry

/*
 * Error codes
 */
#define DVM_COLOR_HASH_TABLE_OK   0
#define DVM_COLOR_HASH_TABLE_FULL -1
#define DVM_COLOR_NOT_FOUND -2


    ColorHashTable *ColorHashTableNew(int);
    void ColorHashTableClear(ColorHashTable *);
    void ColorHashTableFree(ColorHashTable *);
    int ColorHashTableAdd(ColorHashTable *, unsigned char, unsigned char, unsigned char, int);
    int ColorHashTableAddAt(ColorHashTable *, int, unsigned char, unsigned char, unsigned char, int);
    int ColorHashTableFind(ColorHashTable *, unsigned char, unsigned char, unsigned char, int *, int *);
    void ColorHashTableSet(ColorHashTable *, int index, int value);
    void ColorHashTablePackSelf(ColorHashTable *);

    void RgbToY(ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void RgbToYuv444(ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void RgbToYuv422(ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void RgbToYuv411(ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void RgbToYuv420(ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void YuvToRgb444(ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void YuvToRgb422(ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void YuvToRgb411(ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void YuvToRgb420(ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *, ByteImage *);
    void YuvTo1Rgb420(ByteImage *, ByteImage *, ByteImage *, ByteImage *);

    void RgbTo256(ByteImage *, ByteImage *, ByteImage *, ColorHashTable *,
        ImageMap *, ImageMap *, ImageMap *);

    void RgbQuantWithHashTable(ByteImage *, ByteImage *, ByteImage *,
        ColorHashTable *, ImageMap *, ImageMap *, ImageMap *, ByteImage *);

    void RgbQuantWithVpTree(ByteImage *, ByteImage *, ByteImage *,
        VpNode *, ColorHashTable *, ImageMap *, ImageMap *, ImageMap *, ByteImage *);

    void VpTreeInit(ImageMap *, ImageMap *, ImageMap *, VpNode *);
    unsigned char VpTreeFind(VpTree, ImageMap *, ImageMap *, ImageMap *,
        unsigned char, unsigned char, unsigned char);
    void VpTreeFree(VpNode *);
    VpNode *VpTreeNew();

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
