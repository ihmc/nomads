#ifndef INCL_ISAAC_RAND
#define INCL_ISAAC_RAND

#include <cstdio>
#include "FTypes.h"

/* if (flag!=0), then use the contents of randrsl[] to initialize mm[]. */
#define mix(a,b,c,d,e,f,g,h) {\
    a^=b<<11; d+=a; b+=c;\
    b^=c>>2;  e+=b; c+=d;\
    c^=d<<8;  f+=c; d+=e;\
    d^=e>>16; g+=d; e+=f;\
    e^=f<<10; h+=e; f+=g;\
    f^=g>>4;  a+=f; g+=h;\
    g^=h<<8;  b+=g; h+=a;\
    h^=a>>9;  c+=h; a+=b;\
    }
#define RANDVALS 256


namespace NOMADSUtil
{
    class ISAACRand
    {
    public:
        static uint32 getRnd (void);
        static uint32 getRnd (uint32 seed);

    private:
        static void isaac (void);
        static void reset (void);
        static void randinit (bool flag);
        static void startISAAC (void);
        static void startISAAC (uint32 seed);

        static const unsigned short int randcnt = RANDVALS;
        static unsigned char index;
        static uint32 randrsl[RANDVALS], mm[RANDVALS], aa, bb, cc;
    };

}
#endif  // INCL_ISAAC_RAND
