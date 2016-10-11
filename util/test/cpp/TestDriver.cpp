/*
 * TestDriver.cpp
 *
 * Generic Test Driver for multiple test cases
 *
 */

#include <stdio.h>
#include <string.h>

#include "Base64Transcoders.h"
#include "FileWriter.h"
#include "InetAddr.h"
#include "RangeDLList.h"

#include "media/BMPImage.h"

using namespace NOMADSUtil;

int testBMPImage (int argc, char *argv[]);
int testIsIPv4Addr (int argc, char *argv[]);
int testRangeDLList (int argc, char *argv[]);
int testBase64Decode (int argc, char *argv[]);

int main (int argc, char *argv[])
{
    if (argc < 2) {
        fprintf (stderr, "usage: %s <Test> [<arg1> <arg2> ...]\n", argv[0]);
        return -1;
    }
    if (0 == stricmp (argv[1], "BMPImage")) {
        return testBMPImage (argc-2, &argv[2]);
    }
    else if (0 == stricmp (argv[1], "isIPv4Addr")) {
        return testIsIPv4Addr (argc-2, &argv[2]);
    }
    else if (0 == stricmp (argv[1], "RangeDLList")) {
        return testRangeDLList (argc-2, &argv[2]);
    }
    else if (0 == stricmp (argv[1], "base64decode")) {
        return testBase64Decode (argc-2, &argv[2]);
    }
    else {
        fprintf (stderr, "unknown test <%s>\n", argv[1]);
        return -2;
    }
}

int testBMPImage (int argc, char *argv[])
{
    // First test - create a blank BMP
    BMPImage bi;
    bi.initNewImage (1024, 768, 24);
    FileWriter fw ("Test1.bmp", "wb");
    bi.writeHeaderAndImage (&fw);
    fw.close();

    // Second test - create a blank BMP with a diagonal line through it
    BMPImage bi2;
    bi2.initNewImage (1000, 1000, 24);
    FileWriter fw2 ("Test2.bmp", "wb");
    for (uint16 ui16 = 0; ui16 < 1000; ui16++) {
        bi2.setPixel (ui16, 999 - ui16, 255, 255, 255);
    }
    bi2.writeHeaderAndImage (&fw2);
    fw2.close();
    return 0;
}

int testIsIPv4Addr (int argc, char *argv[])
{
    if (argc < 1) {
        fprintf (stderr, "must pass at least one argument for isIPv4Test\n");
        return -1;
    }
    for (int i = 0; i < argc; i++) {
        if (InetAddr::isIPv4Addr (argv[i])) {
            printf ("%s is a valid IPv4 address\n", argv[i]);
        }
        else {
            printf ("%s is NOT a valid IPv4 address\n", argv[i]);
        }
    }
    return 0;
}

int testRangeDLList (int argc, char *argv[])
{
    UInt32RangeDLList dll;
    FILE *fileInputLog = NULL;
    if (NULL == (fileInputLog = fopen ("RangeDLList.TestInput.Log", "r"))) {
        printf ("did not find log file\n");
        return -1;
    }
    
    char szCmd[80];
    uint32 ui32TSN;
    while (fscanf (fileInputLog, "%s %lu", szCmd, &ui32TSN) == 2) {
        printf ("read %s %lu\n", szCmd, ui32TSN);
        if (0 == stricmp (szCmd, "adding")) {
            dll.addTSN (ui32TSN);
        }
        else {
            if (ui32TSN == 348) {
                printf ("here\n");
            }
            dll.removeTSN (ui32TSN);
        }
        if (!dll.validate()) {
            printf ("List is invalid\n");
        }
    }
    //for (uint32 ui32 = 0; ui32 < 10000; ui32++) {
    //    uint32 ui32Value = (uint32) (rand() % 20000);
    //    printf ("adding %lu\n", ui32Value);
    //    dll.addTSN (ui32Value);
    //    ui32Value = (uint32) (rand() % 20000);
    //    printf ("removing %lu\n", ui32Value);
    //    dll.removeTSN (ui32Value);
    //}
    return 0;
}

int testBase64Decode (int argc, char *argv[])
{
    char *buf = (char*) malloc (102400);
    int iCount = 0;
    int iByte;
    while (EOF != (iByte = getchar())) {
        if ((iByte == '\r') || (iByte == '\n')) {
            continue;
        }
        buf[iCount++] = (char) iByte;
    }
    buf[iCount] = '\0';
    printf ("Decoding:\n%s\n", buf);

    unsigned int uiDecodedLength;
    void *pResult = Base64Transcoders::decode (buf, &uiDecodedLength);
    if ((pResult != NULL) && (uiDecodedLength > 0)) {
        FILE *fileOutput = fopen ("output.jpg", "wb");
        fwrite (pResult, uiDecodedLength, 1, fileOutput);
        fclose (fileOutput);
    }
    return 0;
}
