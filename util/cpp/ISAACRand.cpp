/*
------------------------------------------------------------------------------
readable.c: My random number generator, ISAAC.
(c) Bob Jenkins, March 1996, Public Domain
You may use this code in any way you wish, and it is free.  No warrantee.
* May 2008 -- made it not depend on standard.h
------------------------------------------------------------------------------
*/

#include "ISAACRand.h"

namespace NOMADSUtil
{
    void ISAACRand::isaac(void)
    {
        register uint32 i,x,y;

        cc = cc + 1;    /* cc just gets incremented once per 256 results */
        bb = bb + cc;   /* then combined with bb */

        for (i=0; i<randcnt; ++i) {
            x = mm[i];
            switch (i%4) {
            case 0: aa = aa^(aa<<13); break;
            case 1: aa = aa^(aa>>6); break;
            case 2: aa = aa^(aa<<2); break;
            case 3: aa = aa^(aa>>16); break;
            }
            aa              = mm[(i+128)%randcnt] + aa;
            mm[i]      = y  = mm[(x>>2)%randcnt] + aa + bb;
            randrsl[i] = bb = mm[(y>>10)%randcnt] + x;

            /* Note that bits 2..9 are chosen from x but 10..17 are chosen
            from y.  The only important thing here is that 2..9 and 10..17
            don't overlap.  2..9 and 10..17 were then chosen for speed in
            the optimized version (rand.c) */
            /* See http://burtleburtle.net/bob/rand/isaac.html
            for further explanations and analysis. */
        }
    }

    void ISAACRand::reset(void)
    {
        aa = 0;
        bb = 0;
        cc = 0;
        index = 0;
    }

    void ISAACRand::randinit(bool flag)
    {
        int i;
        uint32 a,b,c,d,e,f,g,h;
        aa=bb=cc=0;
        a=b=c=d=e=f=g=h=0x9e3779b9;  /* the golden ratio */

        for (i=0; i<4; ++i) {          /* scramble it */
            mix(a,b,c,d,e,f,g,h);
        }

        for (i=0; i<randcnt; i+=8) {  /* fill in mm[] with messy stuff */
            if (flag) {                 /* use all the information in the seed */
                a+=randrsl[i];
                b+=randrsl[i+1];
                c+=randrsl[i+2];
                d+=randrsl[i+3];
                e+=randrsl[i+4];
                f+=randrsl[i+5];
                g+=randrsl[i+6];
                h+=randrsl[i+7];
            }
            mix(a,b,c,d,e,f,g,h);
            mm[i]=a;
            mm[i+1]=b;
            mm[i+2]=c;
            mm[i+3]=d;
            mm[i+4]=e;
            mm[i+5]=f;
            mm[i+6]=g;
            mm[i+7]=h;
        }

        if (flag) {        /* do a second pass to make all of the seed affect all of mm */
            for (i=0; i<randcnt; i+=8) {
                a+=mm[i];
                b+=mm[i+1];
                c+=mm[i+2];
                d+=mm[i+3];
                e+=mm[i+4];
                f+=mm[i+5];
                g+=mm[i+6];
                h+=mm[i+7];
                mix(a,b,c,d,e,f,g,h);
                mm[i]=a;
                mm[i+1]=b;
                mm[i+2]=c;
                mm[i+3]=d;
                mm[i+4]=e;
                mm[i+5]=f;
                mm[i+6]=g;
                mm[i+7]=h;
            }
        }

        isaac();            /* fill in the first set of results */
    }

    void ISAACRand::startISAAC(void)
    {
        reset();
        for (int i=0; i<randcnt; ++i) {
            mm[i] = randrsl[i] = 0;
        }
        randinit(true);
        isaac();
    }

    void ISAACRand::startISAAC (uint32 seed)
    {
        reset();
        for (int i=0; i<randcnt; ++i) {
            mm[i] = 0;
        }
        for (register int i = 0; i<randcnt; ++i) {
            randrsl[i] = (seed >> (i%4)) + i*(i<<(i%15));
        }
        randinit(true);
    }


    uint32 ISAACRand::getRnd()
    {
        if (index == 0) {
            startISAAC();
        }
        return randrsl[index++];
    }

    uint32 ISAACRand::getRnd(uint32 seed)
    {
        if (index == 0) {
            startISAAC (seed);
        }
        return randrsl[index++];
    }

    unsigned char ISAACRand::index = 0;
    uint32 ISAACRand::mm[] = {};
    uint32 ISAACRand::randrsl[] = {};
    uint32 ISAACRand::aa=0, ISAACRand::bb=0, ISAACRand::cc=0;

}
