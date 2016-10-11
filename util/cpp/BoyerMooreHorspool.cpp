#include "BoyerMooreHorspool.h"

#include "FileReader.h"

#include <stdlib.h>

using namespace NOMADSUtil;

BoyerMooreHorspool::BoyerMooreHorspool (void)
    : _ui16NeedleLen (0),
      _pHaystack (NULL),
      _pui8Needle (NULL)
{
}

BoyerMooreHorspool::~BoyerMooreHorspool (void)
{
}

int BoyerMooreHorspool::init (FileReader *pHaystack, long lHaystackLen, uint8 *pui8Needle, uint16 ui16NeedleLen)
{
    if (pHaystack == NULL || lHaystackLen == 0 || pui8Needle == NULL || ui16NeedleLen == 0) {
        return -1;
    }
    for (unsigned int i = 0; i < 256; i++) {
        _table[i] = ui16NeedleLen;
    }
    for (unsigned int i = 0; i < (ui16NeedleLen - 1); i++) {
        _table[pui8Needle[i]] = ui16NeedleLen - 1 - i;
    }
    _pHaystack = pHaystack;
    _lHaystackLen = lHaystackLen;
    _pui8Needle = pui8Needle;
    _ui16NeedleLen = ui16NeedleLen;
    return 0;
}

int BoyerMooreHorspool::init (FileReader *pHaystack, long lHaystackLen, uint32 ui32Needle)
{
    unsigned char a[4];

    a[0] = (ui32Needle >> 24) & 0xFF;
    a[1] = (ui32Needle >> 16) & 0xFF;
    a[2] = (ui32Needle >> 8) & 0xFF;
    a[3] = ui32Needle & 0xFF;
    return init (pHaystack, lHaystackLen, a, sizeof (uint32));
}

namespace NOMADSUtil
{
    int pickFromHaystack (FileReader &haystack, long lPos, uint8 &ui8)
    {
        if (haystack.seek (lPos) < 0) {
            return -1;
        }
        if (haystack.read8 (&ui8) < 0) {
            return -2;
        }
        return 0;
    }
}

long BoyerMooreHorspool::findFirstNeedle (long lPos) const
{
    if (_pHaystack == NULL || _lHaystackLen == 0 || _pui8Needle == NULL || _ui16NeedleLen == 0) {
        return -1;
    }
    long lSkip = lPos;
    for (uint8 ui8; (_lHaystackLen - lSkip) >= _ui16NeedleLen;) {
        int i = _ui16NeedleLen - 1;
        int rc;
        while (((rc = pickFromHaystack (*_pHaystack, lSkip + i, ui8)) == 0) && (ui8  == _pui8Needle[i])) {
            if (i == 0) {
                return lSkip;
            }
            i -= 1;
        }
        if (rc < 0) {
            return -2;
        }
        if (pickFromHaystack (*_pHaystack, lSkip + _ui16NeedleLen - 1, ui8) < 0) {
            return -3;
        }
        lSkip += +_table[ui8];
    }
    return -4;
}

