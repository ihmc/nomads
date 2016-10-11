/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * yuvtorgb.c
 *
 * Wei Tsang June 97
 *
 * Defines functions that perform color space conversion.
 *
 *----------------------------------------------------------------------
 */

#include "colorInt.h"

extern unsigned char theCropTable[4096];

/* VR[i] = ((i - 128)*  91881) >> 16) + 2048 */
short VR[256] = {
         1868,      1869,      1871,      1872, 
         1874,      1875,      1876,      1878, 
         1879,      1881,      1882,      1883, 
         1885,      1886,      1888,      1889, 
         1890,      1892,      1893,      1895, 
         1896,      1897,      1899,      1900, 
         1902,      1903,      1904,      1906, 
         1907,      1909,      1910,      1912, 
         1913,      1914,      1916,      1917, 
         1919,      1920,      1921,      1923, 
         1924,      1926,      1927,      1928, 
         1930,      1931,      1933,      1934, 
         1935,      1937,      1938,      1940, 
         1941,      1942,      1944,      1945, 
         1947,      1948,      1949,      1951, 
         1952,      1954,      1955,      1956, 
         1958,      1959,      1961,      1962, 
         1963,      1965,      1966,      1968, 
         1969,      1970,      1972,      1973, 
         1975,      1976,      1977,      1979, 
         1980,      1982,      1983,      1984, 
         1986,      1987,      1989,      1990, 
         1991,      1993,      1994,      1996, 
         1997,      1998,      2000,      2001, 
         2003,      2004,      2005,      2007, 
         2008,      2010,      2011,      2012, 
         2014,      2015,      2017,      2018, 
         2019,      2021,      2022,      2024, 
         2025,      2026,      2028,      2029, 
         2031,      2032,      2033,      2035, 
         2036,      2038,      2039,      2040, 
         2042,      2043,      2045,      2046, 
         2048,      2049,      2050,      2052, 
         2053,      2055,      2056,      2057, 
         2059,      2060,      2062,      2063, 
         2064,      2066,      2067,      2069, 
         2070,      2071,      2073,      2074, 
         2076,      2077,      2078,      2080, 
         2081,      2083,      2084,      2085, 
         2087,      2088,      2090,      2091, 
         2092,      2094,      2095,      2097, 
         2098,      2099,      2101,      2102, 
         2104,      2105,      2106,      2108, 
         2109,      2111,      2112,      2113, 
         2115,      2116,      2118,      2119, 
         2120,      2122,      2123,      2125, 
         2126,      2127,      2129,      2130, 
         2132,      2133,      2134,      2136, 
         2137,      2139,      2140,      2141, 
         2143,      2144,      2146,      2147, 
         2148,      2150,      2151,      2153, 
         2154,      2155,      2157,      2158, 
         2160,      2161,      2162,      2164, 
         2165,      2167,      2168,      2169, 
         2171,      2172,      2174,      2175, 
         2176,      2178,      2179,      2181, 
         2182,      2183,      2185,      2186, 
         2188,      2189,      2191,      2192, 
         2193,      2195,      2196,      2198, 
         2199,      2200,      2202,      2203, 
         2205,      2206,      2207,      2209, 
         2210,      2212,      2213,      2214, 
         2216,      2217,      2219,      2220, 
         2221,      2223,      2224,      2226, 
    };


/* UB[i] = ((i - 128)* 116130) >> 16) + 2048 */
short UB[256] = {
         1821,      1822,      1824,      1826, 
         1828,      1830,      1831,      1833, 
         1835,      1837,      1838,      1840, 
         1842,      1844,      1845,      1847, 
         1849,      1851,      1853,      1854, 
         1856,      1858,      1860,      1861, 
         1863,      1865,      1867,      1869, 
         1870,      1872,      1874,      1876, 
         1877,      1879,      1881,      1883, 
         1884,      1886,      1888,      1890, 
         1892,      1893,      1895,      1897, 
         1899,      1900,      1902,      1904, 
         1906,      1908,      1909,      1911, 
         1913,      1915,      1916,      1918, 
         1920,      1922,      1923,      1925, 
         1927,      1929,      1931,      1932, 
         1934,      1936,      1938,      1939, 
         1941,      1943,      1945,      1946, 
         1948,      1950,      1952,      1954, 
         1955,      1957,      1959,      1961, 
         1962,      1964,      1966,      1968, 
         1970,      1971,      1973,      1975, 
         1977,      1978,      1980,      1982, 
         1984,      1985,      1987,      1989, 
         1991,      1993,      1994,      1996, 
         1998,      2000,      2001,      2003, 
         2005,      2007,      2009,      2010, 
         2012,      2014,      2016,      2017, 
         2019,      2021,      2023,      2024, 
         2026,      2028,      2030,      2032, 
         2033,      2035,      2037,      2039, 
         2040,      2042,      2044,      2046, 
         2048,      2049,      2051,      2053, 
         2055,      2056,      2058,      2060, 
         2062,      2063,      2065,      2067, 
         2069,      2071,      2072,      2074, 
         2076,      2078,      2079,      2081, 
         2083,      2085,      2086,      2088, 
         2090,      2092,      2094,      2095, 
         2097,      2099,      2101,      2102, 
         2104,      2106,      2108,      2110, 
         2111,      2113,      2115,      2117, 
         2118,      2120,      2122,      2124, 
         2125,      2127,      2129,      2131, 
         2133,      2134,      2136,      2138, 
         2140,      2141,      2143,      2145, 
         2147,      2149,      2150,      2152, 
         2154,      2156,      2157,      2159, 
         2161,      2163,      2164,      2166, 
         2168,      2170,      2172,      2173, 
         2175,      2177,      2179,      2180, 
         2182,      2184,      2186,      2187, 
         2189,      2191,      2193,      2195, 
         2196,      2198,      2200,      2202, 
         2203,      2205,      2207,      2209, 
         2211,      2212,      2214,      2216, 
         2218,      2219,      2221,      2223, 
         2225,      2226,      2228,      2230, 
         2232,      2234,      2235,      2237, 
         2239,      2241,      2242,      2244, 
         2246,      2248,      2250,      2251, 
         2253,      2255,      2257,      2258, 
         2260,      2262,      2264,      2265, 
         2267,      2269,      2271,      2273, 
    };


/* UG[i] = ((i - 128)* -22554) */
int UG[256] = {
      2886855,   2864302,   2841748,   2819194, 
      2796641,   2774087,   2751534,   2728980, 
      2706427,   2683873,   2661320,   2638766, 
      2616212,   2593659,   2571105,   2548552, 
      2525998,   2503445,   2480891,   2458338, 
      2435784,   2413230,   2390677,   2368123, 
      2345570,   2323016,   2300463,   2277909, 
      2255355,   2232802,   2210248,   2187695, 
      2165141,   2142588,   2120034,   2097481, 
      2074927,   2052373,   2029820,   2007266, 
      1984713,   1962159,   1939606,   1917052, 
      1894498,   1871945,   1849391,   1826838, 
      1804284,   1781731,   1759177,   1736624, 
      1714070,   1691516,   1668963,   1646409, 
      1623856,   1601302,   1578749,   1556195, 
      1533642,   1511088,   1488534,   1465981, 
      1443427,   1420874,   1398320,   1375767, 
      1353213,   1330660,   1308106,   1285552, 
      1262999,   1240445,   1217892,   1195338, 
      1172785,   1150231,   1127677,   1105124, 
      1082570,   1060017,   1037463,   1014910, 
       992356,    969803,    947249,    924695, 
       902142,    879588,    857035,    834481, 
       811928,    789374,    766821,    744267, 
       721713,    699160,    676606,    654053, 
       631499,    608946,    586392,    563838, 
       541285,    518731,    496178,    473624, 
       451071,    428517,    405964,    383410, 
       360856,    338303,    315749,    293196, 
       270642,    248089,    225535,    202982, 
       180428,    157874,    135321,    112767, 
        90214,     67660,     45107,     22553, 
            0,    -22553,    -45107,    -67660, 
       -90214,   -112767,   -135321,   -157874, 
      -180428,   -202982,   -225535,   -248089, 
      -270642,   -293196,   -315749,   -338303, 
      -360856,   -383410,   -405964,   -428517, 
      -451071,   -473624,   -496178,   -518731, 
      -541285,   -563838,   -586392,   -608946, 
      -631499,   -654053,   -676606,   -699160, 
      -721713,   -744267,   -766821,   -789374, 
      -811928,   -834481,   -857035,   -879588, 
      -902142,   -924695,   -947249,   -969803, 
      -992356,  -1014910,  -1037463,  -1060017, 
     -1082570,  -1105124,  -1127677,  -1150231, 
     -1172785,  -1195338,  -1217892,  -1240445, 
     -1262999,  -1285552,  -1308106,  -1330660, 
     -1353213,  -1375767,  -1398320,  -1420874, 
     -1443427,  -1465981,  -1488534,  -1511088, 
     -1533642,  -1556195,  -1578749,  -1601302, 
     -1623856,  -1646409,  -1668963,  -1691516, 
     -1714070,  -1736624,  -1759177,  -1781731, 
     -1804284,  -1826838,  -1849391,  -1871945, 
     -1894498,  -1917052,  -1939606,  -1962159, 
     -1984713,  -2007266,  -2029820,  -2052373, 
     -2074927,  -2097481,  -2120034,  -2142588, 
     -2165141,  -2187695,  -2210248,  -2232802, 
     -2255355,  -2277909,  -2300463,  -2323016, 
     -2345570,  -2368123,  -2390677,  -2413230, 
     -2435784,  -2458338,  -2480891,  -2503445, 
     -2525998,  -2548552,  -2571105,  -2593659, 
     -2616212,  -2638766,  -2661320,  -2683873, 
     -2706427,  -2728980,  -2751534,  -2774087, 
     -2796641,  -2819194,  -2841748,  -2864302, 
    };


/* VG[i] = ((i - 128)* -46802) */
int VG[256] = {
      5990640,   5943838,   5897036,   5850235, 
      5803433,   5756631,   5709829,   5663027, 
      5616225,   5569423,   5522621,   5475820, 
      5429018,   5382216,   5335414,   5288612, 
      5241810,   5195008,   5148206,   5101405, 
      5054603,   5007801,   4960999,   4914197, 
      4867395,   4820593,   4773791,   4726990, 
      4680188,   4633386,   4586584,   4539782, 
      4492980,   4446178,   4399376,   4352574, 
      4305773,   4258971,   4212169,   4165367, 
      4118565,   4071763,   4024961,   3978159, 
      3931357,   3884556,   3837754,   3790952, 
      3744150,   3697348,   3650546,   3603744, 
      3556942,   3510141,   3463339,   3416537, 
      3369735,   3322933,   3276131,   3229329, 
      3182527,   3135726,   3088924,   3042122, 
      2995320,   2948518,   2901716,   2854914, 
      2808112,   2761310,   2714509,   2667707, 
      2620905,   2574103,   2527301,   2480499, 
      2433697,   2386895,   2340094,   2293292, 
      2246490,   2199688,   2152886,   2106084, 
      2059282,   2012480,   1965678,   1918877, 
      1872075,   1825273,   1778471,   1731669, 
      1684867,   1638065,   1591263,   1544462, 
      1497660,   1450858,   1404056,   1357254, 
      1310452,   1263650,   1216848,   1170047, 
      1123245,   1076443,   1029641,    982839, 
       936037,    889235,    842433,    795631, 
       748830,    702028,    655226,    608424, 
       561622,    514820,    468018,    421216, 
       374415,    327613,    280811,    234009, 
       187207,    140405,     93603,     46801, 
            0,    -46801,    -93603,   -140405, 
      -187207,   -234009,   -280811,   -327613, 
      -374415,   -421216,   -468018,   -514820, 
      -561622,   -608424,   -655226,   -702028, 
      -748830,   -795631,   -842433,   -889235, 
      -936037,   -982839,  -1029641,  -1076443, 
     -1123245,  -1170047,  -1216848,  -1263650, 
     -1310452,  -1357254,  -1404056,  -1450858, 
     -1497660,  -1544462,  -1591263,  -1638065, 
     -1684867,  -1731669,  -1778471,  -1825273, 
     -1872075,  -1918877,  -1965678,  -2012480, 
     -2059282,  -2106084,  -2152886,  -2199688, 
     -2246490,  -2293292,  -2340094,  -2386895, 
     -2433697,  -2480499,  -2527301,  -2574103, 
     -2620905,  -2667707,  -2714509,  -2761310, 
     -2808112,  -2854914,  -2901716,  -2948518, 
     -2995320,  -3042122,  -3088924,  -3135726, 
     -3182527,  -3229329,  -3276131,  -3322933, 
     -3369735,  -3416537,  -3463339,  -3510141, 
     -3556942,  -3603744,  -3650546,  -3697348, 
     -3744150,  -3790952,  -3837754,  -3884556, 
     -3931357,  -3978159,  -4024961,  -4071763, 
     -4118565,  -4165367,  -4212169,  -4258971, 
     -4305773,  -4352574,  -4399376,  -4446178, 
     -4492980,  -4539782,  -4586584,  -4633386, 
     -4680188,  -4726990,  -4773791,  -4820593, 
     -4867395,  -4914197,  -4960999,  -5007801, 
     -5054603,  -5101405,  -5148206,  -5195008, 
     -5241810,  -5288612,  -5335414,  -5382216, 
     -5429018,  -5475820,  -5522621,  -5569423, 
     -5616225,  -5663027,  -5709829,  -5756631, 
     -5803433,  -5850235,  -5897036,  -5943838, 
    };


#define CROP(n) theCropTable[(n)]

void 
YuvToRgb411(yBuf, uBuf, vBuf, rBuf, gBuf, bBuf)
    ByteImage *yBuf;
    ByteImage *uBuf;
    ByteImage *vBuf;
    ByteImage *rBuf;
    ByteImage *gBuf;
    ByteImage *bBuf;
{
    register int  row, col, w, h;
    register int  rSkip, gSkip, bSkip, ySkip, uSkip, vSkip;
    unsigned char *rRow, *gRow, *bRow, *yRow, *uRow, *vRow;
    long int r, g, b;
    long int u, v;
    register int vr, uvg, ub, y;

    rRow = rBuf->firstByte;
    gRow = gBuf->firstByte;
    bRow = bBuf->firstByte;
    yRow = yBuf->firstByte;
    uRow = uBuf->firstByte;
    vRow = vBuf->firstByte;

    rSkip = rBuf->parentWidth - rBuf->width;
    gSkip = gBuf->parentWidth - gBuf->width;
    bSkip = bBuf->parentWidth - bBuf->width;
    ySkip = yBuf->parentWidth - yBuf->width;
    uSkip = uBuf->parentWidth - uBuf->width;
    vSkip = vBuf->parentWidth - vBuf->width;

    h = rBuf->height;
    w = rBuf->width >> 2;

    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col ++) {

            u = *uRow++;
            v = *vRow++;
            vr = VR[v];
            uvg = ((UG[u] + VG[v]) >> 16) + 2048;
            ub = UB[u];
            y = *yRow++;

            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow++ = CROP(r);
            *gRow++ = CROP(g);
            *bRow++ = CROP(b);

            y = *yRow++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow++ = CROP(r);
            *gRow++ = CROP(g);
            *bRow++ = CROP(b);

            y = *yRow++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow++ = CROP(r);
            *gRow++ = CROP(g);
            *bRow++ = CROP(b);

            y = *yRow++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow++ = CROP(r);
            *gRow++ = CROP(g);
            *bRow++ = CROP(b);
        }
    rRow += rSkip;
    gRow += gSkip;
    bRow += bSkip;
    yRow += ySkip;
    uRow += uSkip;
    vRow += vSkip;
    }
}


void 
YuvToRgb422(yBuf, uBuf, vBuf, rBuf, gBuf, bBuf)
    ByteImage *yBuf;
    ByteImage *uBuf;
    ByteImage *vBuf;
    ByteImage *rBuf;
    ByteImage *gBuf;
    ByteImage *bBuf;
{
    register int  row, col, w, h;
    register int  rSkip, gSkip, bSkip, ySkip, uSkip, vSkip;
    unsigned char *rRow, *gRow, *bRow, *yRow, *uRow, *vRow;
    long int r, g, b;
    long int y, u, v;
    register int vr, uvg, ub;

    rRow = rBuf->firstByte;
    gRow = gBuf->firstByte;
    bRow = bBuf->firstByte;
    yRow = yBuf->firstByte;
    uRow = uBuf->firstByte;
    vRow = vBuf->firstByte;

    rSkip = rBuf->parentWidth - rBuf->width;
    gSkip = gBuf->parentWidth - gBuf->width;
    bSkip = bBuf->parentWidth - bBuf->width;
    ySkip = yBuf->parentWidth - yBuf->width;
    uSkip = uBuf->parentWidth - uBuf->width;
    vSkip = vBuf->parentWidth - vBuf->width;

    h = rBuf->height;
    w = rBuf->width >> 1;

    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col ++) {
            u = *uRow++;
            v = *vRow++;
            vr = VR[v];
            uvg = ((UG[u] + VG[v]) >> 16) + 2048;
            ub = UB[u];

            y = *yRow++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow++ = CROP(r);
            *gRow++ = CROP(g);
            *bRow++ = CROP(b);

            y = *yRow++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow++ = CROP(r);
            *gRow++ = CROP(g);
            *bRow++ = CROP(b);
        }
    rRow += rSkip;
    gRow += gSkip;
    bRow += bSkip;
    yRow += ySkip;
    uRow += uSkip;
    vRow += vSkip;
    }
}

#ifdef DEBUG
void 
YuvTo1Rgb420(yBuf, uBuf, vBuf, rBuf)
    ByteImage *yBuf;
    ByteImage *uBuf;
    ByteImage *vBuf;
    ByteImage *rBuf;
{
    int  row, col, w, h;
    int  rSkip, ySkip, uSkip, vSkip;
    register unsigned char *rRow1, *yRow1, *uRow, *vRow;
    register unsigned char *rRow2, *yRow2; 
    register long int r, g, b;
    register long int y, u, v;
    register int vr, uvg, ub;

    rRow1 = rBuf->firstByte;
    yRow1 = yBuf->firstByte;
    uRow = uBuf->firstByte;
    vRow = vBuf->firstByte;

    rSkip = 2*rBuf->parentWidth - rBuf->width;
    ySkip = 2*yBuf->parentWidth - yBuf->width;
    uSkip = uBuf->parentWidth - uBuf->width;
    vSkip = vBuf->parentWidth - vBuf->width;

    yRow2 = yRow1 + yBuf->parentWidth;
    rRow2 = rRow1 + rBuf->parentWidth;

    h = yBuf->height >> 1;
    w = yBuf->width >> 1;

    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col ++) {
            /* 
             * CROP should be defined as table[n+2048] but here
             * we add 2048 to vr, uvg and ub to avoid addition
             * every call to CROP 
             */
            u = *uRow++;
            v = *vRow++;
            vr = VR[v];
            uvg = ((UG[u] + VG[v]) >> 16) + 2048;
            ub = UB[u] ;

            y = *yRow1++;
            
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow1++ = CROP(r);
            *rRow1++ = CROP(g);
            *rRow1++ = CROP(b);

            y = *yRow1++;
            
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow1++ = CROP(r);
            *rRow1++ = CROP(g);
            *rRow1++ = CROP(b);

            y = *yRow2++;
            
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow2++ = CROP(r);
            *rRow2++ = CROP(g);
            *rRow2++ = CROP(b);

            y = *yRow2++;
            
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow2++ = CROP(r);
            *rRow2++ = CROP(g);
            *rRow2++ = CROP(b);
        }
    rRow2 += rSkip;
    yRow2 += ySkip;
    rRow1 += rSkip;
    yRow1 += ySkip;
    uRow += uSkip;
    vRow += vSkip;
    }
}
#endif


void 
YuvToRgb420(yBuf, uBuf, vBuf, rBuf, gBuf, bBuf)
    ByteImage *yBuf;
    ByteImage *uBuf;
    ByteImage *vBuf;
    ByteImage *rBuf;
    ByteImage *gBuf;
    ByteImage *bBuf;
{
    int  row, col, w, h;
    int  rSkip, gSkip, bSkip, ySkip, uSkip, vSkip;
    register unsigned char *rRow1, *gRow1, *bRow1, *yRow1, *uRow, *vRow;
    register unsigned char *rRow2, *gRow2, *bRow2, *yRow2; 
    register long int r, g, b;
    register long int y, u, v;
    register int vr, uvg, ub;

    rRow1 = rBuf->firstByte;
    gRow1 = gBuf->firstByte;
    bRow1 = bBuf->firstByte;
    yRow1 = yBuf->firstByte;
    uRow = uBuf->firstByte;
    vRow = vBuf->firstByte;

    rSkip = 2*rBuf->parentWidth - rBuf->width;
    gSkip = 2*gBuf->parentWidth - gBuf->width;
    bSkip = 2*bBuf->parentWidth - bBuf->width;
    ySkip = 2*yBuf->parentWidth - yBuf->width;
    uSkip = uBuf->parentWidth - uBuf->width;
    vSkip = vBuf->parentWidth - vBuf->width;

    yRow2 = yRow1 + yBuf->parentWidth;
    rRow2 = rRow1 + rBuf->parentWidth;
    gRow2 = gRow1 + gBuf->parentWidth;
    bRow2 = bRow1 + bBuf->parentWidth;

    h = rBuf->height >> 1;
    w = rBuf->width >> 1;

    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col ++) {
            u = *uRow++;
            v = *vRow++;
            vr = VR[v];
            uvg = ((UG[u] + VG[v]) >> 16) + 2048;
            ub = UB[u];

            y = *yRow1++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow1++ = CROP(r);
            *gRow1++ = CROP(g);
            *bRow1++ = CROP(b);

            y = *yRow1++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow1++ = CROP(r);
            *gRow1++ = CROP(g);
            *bRow1++ = CROP(b);

            y = *yRow2++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow2++ = CROP(r);
            *gRow2++ = CROP(g);
            *bRow2++ = CROP(b);

            y = *yRow2++;
            r = vr + y;
            g = uvg + y;
            b = ub + y;

            *rRow2++ = CROP(r);
            *gRow2++ = CROP(g);
            *bRow2++ = CROP(b);
        }
    rRow2 += rSkip;
    gRow2 += gSkip;
    bRow2 += bSkip;
    yRow2 += ySkip;
    rRow1 += rSkip;
    gRow1 += gSkip;
    bRow1 += bSkip;
    yRow1 += ySkip;
    uRow += uSkip;
    vRow += vSkip;
    }
}


void 
YuvToRgb444(yBuf, uBuf, vBuf, rBuf, gBuf, bBuf)
    ByteImage *yBuf;
    ByteImage *uBuf;
    ByteImage *vBuf;
    ByteImage *rBuf;
    ByteImage *gBuf;
    ByteImage *bBuf;
{
    register int  row, w, h;
    register int  rSkip, gSkip, bSkip, ySkip, uSkip, vSkip;
    unsigned char *rRow, *gRow, *bRow, *yRow, *uRow, *vRow;
    long int r, g, b;
    long int u1, v1;
    register int y;

    rRow = rBuf->firstByte;
    gRow = gBuf->firstByte;
    bRow = bBuf->firstByte;
    yRow = yBuf->firstByte;
    uRow = uBuf->firstByte;
    vRow = vBuf->firstByte;

    rSkip = rBuf->parentWidth - rBuf->width;
    gSkip = gBuf->parentWidth - gBuf->width;
    bSkip = bBuf->parentWidth - bBuf->width;
    ySkip = yBuf->parentWidth - yBuf->width;
    uSkip = uBuf->parentWidth - uBuf->width;
    vSkip = vBuf->parentWidth - vBuf->width;

    h = rBuf->height;
    w = rBuf->width;

    for (row = 0; row < h; row++) {
        DO_N_TIMES (w, 
            y = *yRow++;
            u1 = *uRow++;
            v1 = *vRow++;

            r = VR[v1] + y;
            g = ((UG[u1] + VG[v1]) >> 16) + 2048 + y;
            b = UB[u1] + y;

            *rRow++ = CROP(r);
            *gRow++ = CROP(g);
            *bRow++ = CROP(b);
        )
    rRow += rSkip;
    gRow += gSkip;
    bRow += bSkip;
    yRow += ySkip;
    uRow += uSkip;
    vRow += vSkip;
    }
}
