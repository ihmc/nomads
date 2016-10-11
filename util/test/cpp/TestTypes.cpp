/* 
 * File:   TestTypes.cpp
 * Author: ebenvegnu
 *
 * Created on March 19, 2009, 4:26 PM
 */

#include <stdlib.h>
#include <stdio.h>
#include "FTypes.h"

//using namespace NOMADSUtil;
/*
 * 
 */
int main(int argc, char** argv)
{
    int8 i8Myint8 = 0;
    
    uint8 ui8Myuint8 = 0;
    
    int16 i16Myint16 = 0;
    
    uint16 ui16Myuint16 = 0;
    
    int32 i32Myint32 = 0;
    
    uint32 ui32Myuint32 = 0;
    
    int64 i64Myint64 = 0;
    
    uint64 ui64Myuint64 = 0;
    
    printf ("Size of int8 %d\n", sizeof(i8Myint8));
    printf ("Size of uint8 %d\n", sizeof(ui8Myuint8));
    printf ("Size of int16 %d\n", sizeof(i16Myint16));
    printf ("Size of uint16 %d\n", sizeof(ui16Myuint16));
    printf ("Size of int32 %d\n", sizeof(i32Myint32));
    printf ("Size of uint32 %d\n", sizeof(ui32Myuint32));
    printf ("Size of int64 %d\n", sizeof(i64Myint64));
    printf ("Size of uint64 %d\n", sizeof(ui64Myuint64));
    
    
    return 0;
}


