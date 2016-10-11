/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/*------------------------------------------------------------------------
 * 
 * vptree.c
 *
 * This file implements a Vp (Vintage Point) Tree structure for fast
 * closest neighbour search.
 *
 * This data structure is describe in details from a paper by Yianilos
 * entitle "Data structures and algorithms for nearest neighbor search 
 * in general metric spaces", SODA 93.
 *
 *--------------------------------------------------------------------------
 */

#include "dvmcolor.h"

extern unsigned short theSquareTable[];
#define sqr(x) theSquareTable[x + 255]


static void
FindMedian(a, n)
    int *a;
    int n;
{
    int v,t;
    int i,j,r,l;

    //l=1;r=n;
    l=0;r=n-1;

    while(r>l) {
        v=a[r]; i=l-1; j=r;
        for (;;) {
            while (a[++i] < v);
            while (a[--j] > v && j>l); 
            if (i >= j) break;
            t=a[i]; a[i] = a[j]; a[j]=t;
        }
        t=a[i]; a[i] = a[r]; a[r]=t;
        if (a[i]>=n/2) r=i-1;
        if (a[i]<=n/2) l=i+1;
    }
}


static void
SelectVp (list, n, first, rmap, gmap, bmap, median, index)
    unsigned char *list;
    int n;
    int first;
    ImageMap *rmap;
    ImageMap *gmap;
    ImageMap *bmap;
    float *median;
    int *index;
{
    /*
     * Randomly sample 10 points from the list if n >= 10, 
     * else just sample all
     */

    /*
     * rgb is a 10 elements array of (index, r, g, b)
     */
    unsigned char rgb[256][4]; 
    int distance[255], mu, bestmu;
    int diff, ex, ex2, variance, best;
    unsigned char random, bestIndex;
    int i, j, k, size;

    bestmu = 0;
    bestIndex = 0;

    /*size = (n >= 10) ? 10 : n;*/
    size = n;

    /*
     * Randomly sample 10 colors from the 256 colors
     */
    srand(256);
    for (i = 0; i < size; i++) {
        /*random = rand() % n;*/
        random = i;

        /*
         * Make sure this is not selected before
        clear = 0;
        while (!clear) {
            foundEqual = 0;
            for (j = 0; j < i; j++) {
                if (random == rgb[i][0]) {
                    foundEqual = 1;
                    break;
                }
            }
            if (foundEqual) {
                clear = 0;
                random = rand() % n;
            } else {
                clear = 1;
            }
        }
         */
            
        rgb[i][0] = random;
        rgb[i][1] = rmap->table[list[first + random]];
        rgb[i][2] = gmap->table[list[first + random]];
        rgb[i][3] = bmap->table[list[first + random]];
    }

    best = (1 << (8*sizeof(int) - 1));  // negative max int. 
    /*
     * Now find the median distance of those points
     */
    for (i = 0; i < size; i++) {
        k = 0;
        for (j = 0; j < size; j++) {
            if (i != j) {
                distance[k] = sqr(rgb[i][1] - rgb[j][1]) +
                            sqr(rgb[i][2] - rgb[j][2]) +
                            sqr(rgb[i][3] - rgb[j][3]);
                // distance[k] = (distance[k])/(1.0 + distance[k]);
                k++;
            }
        }
        FindMedian(distance, size - 1);
        mu = distance[(size - 1)/2];

        /* find the variance */
        ex = 0;
        ex2 = 0;
        for (j = 0; j < size - 1; j++) {
            diff = distance[j] - mu;
            ex += diff;
            ex2 += diff*diff;
        } 
        variance = ex2 - ex*ex;
        if (variance > best) {
            best = variance;
            bestmu = mu;
            bestIndex = list[first + rgb[i][0]];
        }
    }

    *median = (float)sqrt(bestmu);
    *index  = bestIndex;
}



VpNode *
VpTreeNew()
{
    VpNode *i;

    i = NEWARRAY(VpNode, 256);

    return i;
}


/*-------------------------------------------------------------------
 *
 * CreateSubTree
 *
 * Recursive version of VpTreeInit
 *
 * list[first] .. list[first + n] are the stuff that we are interested in.
 * Create a new Vptree with tree[*root] as root.
 *
 *-------------------------------------------------------------------
 */


static void
CreateSubTree(root, list, n, first, rmap, gmap, bmap, tree)
    int *root;
    unsigned char *list;
    int n;
    int first;    
    ImageMap *rmap;
    ImageMap *gmap;
    ImageMap *bmap;
    VpTree tree;
{
    int index;
    unsigned char left[256];
    unsigned char right[256];
    float mu, distance;
    int l, r, j, thisNode;

#if 0
    int hi, lo;
#endif

    /*fprintf(stderr, "%d\t%d\t", *root, n);*/
    /*
     * if there is only one color left, just create the thing
     * and return
     */
    if (n == 1) {
        tree[*root].index = list[0];
        tree[*root].mu    = 0.0;
        tree[*root].ll    = 5.0;
        tree[*root].lu    = -5.0;
        tree[*root].rl    = 5.0;
        tree[*root].ru    = -5.0;
        tree[*root].left  = -1;
        tree[*root].right  = -1;
        /*fprintf(stderr, "\n");*/
        return;
    }

    SelectVp(list, n, first, rmap, gmap, bmap, &mu, &index);
    /*fprintf(stderr, "%f\n", mu);*/
    tree[*root].index = index;
    tree[*root].mu = mu;
    tree[*root].ll = (1<<30);
    tree[*root].lu = -(1<<30);
    tree[*root].rl = (1<<30);
    tree[*root].ru = -(1<<30);

    /*
     * now partition list[first] .. list[first+n] into two.
     * those < mu on the left and >= mu on the right
     */
    l = 0;
    r = 0;

    for (j = first; j < first + n; j++) {
        if (list[j] != index) {
            distance = (float)(
                sqrt(sqr(rmap->table[list[j]] - rmap->table[index]) +
                sqr(gmap->table[list[j]] - gmap->table[index]) +
                sqr(bmap->table[list[j]] - bmap->table[index])));
            // distance = distance/(1.0 + distance);
            if (distance < mu) {
                if (tree[*root].ll > distance) {
                    tree[*root].ll = distance;
                }
                if (tree[*root].lu < distance) {
                    tree[*root].lu = distance;
                }
                left[l] = list[j];
                l++;
            } else {
                if (tree[*root].rl > distance) {
                    tree[*root].rl = distance;
                }
                if (tree[*root].ru < distance) {
                    tree[*root].ru = distance;
                }
                right[r] = list[j];
                r++;
            }
        }
    }

    thisNode = *root;
    if (l != 0) {
        (*root) ++;
        tree[thisNode].left = *root;
        CreateSubTree(root, left, l, 0, rmap, gmap, bmap, tree);
    }
    if (r != 0) {
        (*root) ++;
        tree[thisNode].right = *root;
        CreateSubTree(root, right, r, 0, rmap, gmap, bmap, tree);
    }
}


/*-------------------------------------------------------------------
 *
 * VpTreeInit
 *
 * Initialized the tree with colors from the ImageMap rmap,gmap,bmap
 *
 *-------------------------------------------------------------------
 */

void
VpTreeInit(rmap, gmap, bmap, root)
    ImageMap *rmap;
    ImageMap *gmap;
    ImageMap *bmap;
    VpNode *root;
{

    unsigned char list[256];
    int i;

    for (i = 0; i < 256; i++) {
        list[i] = i;
    }

    i = 0;
    CreateSubTree(&i, list, 256, 0, rmap, gmap, bmap, root);
}


/*-------------------------------------------------------------------
 *
 * RecurSearch
 *
 * Recursive version of VpFind.
 * Traverse the tree, keeping track of the index of the closest color
 * so far.  Prune of branches that contains only colors with larger
 * distance.
 *
 *-------------------------------------------------------------------
 */

static void
RecurSearch(tree, rmap, gmap, bmap, r, g, b, n, min, minIndex)
    VpNode *tree;
    ImageMap *rmap;
    ImageMap *gmap;
    ImageMap *bmap;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    int n;
    float *min;
    int *minIndex;
{
    int x;
    int middle;

    if (n == -1)
        return;

    x = (int)(sqrt(sqr(r - rmap->table[tree[n].index]) +
        sqr(g - gmap->table[tree[n].index]) +
        sqr(b - bmap->table[tree[n].index])));

    if (x < *min) {
        *min = (float)x;
        *minIndex = tree[n].index;
    }
    middle = (int)((tree[n].lu + tree[n].rl)/2); 
    if (x < middle) {
        if ((x > tree[n].ll - *min) && (x < tree[n].lu + *min)) {
            RecurSearch(tree, rmap, gmap, bmap, r, g, b, 
                    tree[n].left, min, minIndex);
        }
        if ((x > tree[n].rl - *min) && (x < tree[n].ru + *min)) {
            RecurSearch(tree, rmap, gmap, bmap, r, g, b, 
                    tree[n].right, min, minIndex);
        }
    } else {
        if ((x > tree[n].rl - *min) && (x < tree[n].ru + *min)) {
            RecurSearch(tree, rmap, gmap, bmap, r, g, b, 
                    tree[n].right, min, minIndex);
        }
        if ((x > tree[n].ll - *min) && (x < tree[n].lu + *min)) {
            RecurSearch(tree, rmap, gmap, bmap, r, g, b, 
                    tree[n].left, min, minIndex);
        }
    }
}

/*-------------------------------------------------------------------
 *
 * VpTreeFind
 *
 * Look for color (r,g,b) in the VpTree tree.  Return the index to 
 * the color in color maps rmap, gmap and rmap that is the closest to 
 * (r,g,b).
 *
 *-------------------------------------------------------------------
 */

unsigned char 
VpTreeFind(tree, rmap, gmap, bmap, r, g, b)
    VpTree tree;
    ImageMap *rmap;
    ImageMap *gmap;
    ImageMap *bmap;
    unsigned char r;
    unsigned char g;
    unsigned char b;
{
    float min = (float)(1 << 16);
    int minIndex = 0;

    RecurSearch(tree, rmap, gmap, bmap, r, g, b, 0, &min, &minIndex);
    return minIndex;
}

void
VpTreeFree(tree)
    VpTree tree;
{
    FREE(tree);
}
