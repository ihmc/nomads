/* 
 * File:   TestCompass.cpp
 * Author: ebenvegnu
 *
 * Created on July 27, 2009, 4:26 PM
 */

#include "CommHelper2.h"
#include "Logger.h"
#include "Serial.h"
#include "SerialReader.h"
#include "SerialWriter.h"

using namespace NOMADSUtil;
using namespace NOMADSUtil;


int main(int argc, char** argv)
{
    printf ("TestCompass...\n");
    
    if (argc != 2) {
        printf ("usage: %s <com port>\n", argv[0]);
        return -1;
    }
    printf ("using <%s>\n", argv[1]);
    int rc;
    Serial serial;
    if (0 != (rc = serial.init (argv[1], 9600))) {
        printf ("initalization of serial port failed; rc = %d\n", rc);
        return -2;
    }
    SerialReader sr (&serial);
    SerialWriter sw (&serial);
    CommHelper2 ch;
    if (0 != (rc = ch.init (&sr, &sw))) {
        printf ("failed to initialize CommHelper; rc = %d\n", rc);
        return -3;
    }
    try {
        while (true) {
            const char **apszTokens = ch.receiveParsed();
            int iHeading = atoi (apszTokens[0]);
            printf ("received heading = %d\n", iHeading);
        }
    }
    catch (Exception e) {
        printf ("Exception occurred - %s\n", e.getMsg());
    }
    
    return 0;
}
