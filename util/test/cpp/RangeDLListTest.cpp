/**************************************************************************************************
* Author:	Thomas Rolinger trolinger@ihmc.us
* Date:		9/12/11
* File:		TestRangeDLList.cpp
* Description:	This is a test class for the RangeDLList util class. The purpose of this class is to
*		test all relevant methods and operations for the RangeDLList<uint32>
**************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "RangeDLList.h"
#include "StrClass.h"

using namespace NOMADSUtil;

void listRanges (RangeDLList<uint32> &ranges, const char *pszExpected)
{
    printf ("\t\t- Print out the contents of the list (Expected: %s)\n", pszExpected);

    char buf[1024];
    uint32 ui32Begin, ui32End;
    if (0 != (ranges.getFirst(ui32Begin, ui32End, true))) {
        printf ("\t\t-Error: getFirst method failed\n\n");
        exit (1);
    }
    int rc = sprintf (buf, "%u-%u ", ui32Begin, ui32End);
    while ( 0 == (ranges.getNext(ui32Begin, ui32End)) ) {
        rc += sprintf ((buf + rc), "%u-%u ", ui32Begin, ui32End);	
    }

    String s (buf);
    s.trim();
    if (s != pszExpected) {
        printf ("ERROR! Actual list: %s", s.c_str());
        exit (2);
    }

    printf ("\t\t\t%s", s.c_str());
}

int test (bool bUseSequentialArithmetic)
{
    uint32 ui32Begin, ui32End;
    printf ("\n\n***** Beginning the RangeDList Test ****\n");

    //Create an instance of the RangeDLList<uint32> class
    UInt32RangeDLList *pRangeList = new UInt32RangeDLList (bUseSequentialArithmetic);

//#################################################################################################
    //Test to see if the list has information (expected: FALSE)
    printf ("\t+ See if the list contains information (expected: FALSE)\n");
    if ( pRangeList->haveInformation() ) {
        printf ("\t\t-The list contains information\n\n");
    }
    else {
        printf ("\t\t-The list does not contain information\n\n");
    }

//#################################################################################################
    //Add ranges to the list, as well as single elements. The list should end up being 1-11 16-16
    printf ("\t+ Adding the ranges 1-4, 5-6, 7-8, 9-10 and elements 11 and 16 to the list...\n");
    if ( !(pRangeList->addTSN (2, 4)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add range 2-4\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the range 2-4 successfully\n");
    }
    if ( !(pRangeList->addTSN (5, 6)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add range 5-6\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the range 5-6 successfully\n");
    }
    if ( !(pRangeList->addTSN (7, 8)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add range 7-8\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the range 7-8 successfully\n");
    }
    if ( !(pRangeList->addTSN (9, 10)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add range 9-10\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the range 9-10 successfully\n");
    }
    if ( !(pRangeList->addTSN (11)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add the element 11\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the element 11 successfully\n");
    }
    if ( !(pRangeList->addTSN (11,12)) ) {
            printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add the range 11,12\n");
            exit(1);
	}
	else {
            printf ("\t\t-Added the rage 11,12 successfully\n");
	}
    if ( !(pRangeList->addTSN (0,2)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add the range 0,2\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the rage 0,2 successfully\n");
    }


    if ( !(pRangeList->addTSN (16)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add element 16\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the element 16 successfully\n\n");
    }

//#################################################################################################
    //Attempt to add some duplicate ranges. Expected: ERROR
    printf ("\t+ Attempting to add duplicate ranges (Expected: Error)\n");
    if ( !(pRangeList->addTSN (1, 4)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: List already contains 1-4\n");
    }
    else {
        printf ("\t\t-Added the range 1-4 successfully\n");
    }
    if ( !(pRangeList->addTSN (5, 8)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: List already containts 5-8\n\n");
    }
    else {
        printf ("\t\t-Added the range 5-8 successfully\n\n");
    }

//#################################################################################################
    //Attempt to add some duplicate elements. Expected: ERROR
    printf ("\t+ Attempting to add duplicate elements (Expected: Error)\n");
    if ( !(pRangeList->addTSN (1)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: List already contains 1\n");
    }
    else {
        printf ("\t\t-Added the element 1 successfully\n");
    }
    if ( !(pRangeList->addTSN (6)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: List already containts 6\n\n");
    }
    else {
        printf ("\t\t-Added the element 6 successfully\n\n");
    }

//#################################################################################################
    //Test to see if the list has information
    printf ("\t+ See if the list contains information (expected: TRUE)\n");
    if ( pRangeList->haveInformation() ) {
        printf ("\t\t-The list contains information\n\n");
    }
    else {
        printf ("\t\t-The list does not contain information\n\n");
    }

//#################################################################################################
    //Print out the contents of the list: Expected 0-12 16-16
    printf ("\t+ Print out the contents of the list (Expected: 0-12 16-16)\n");
    if (0 != ( pRangeList->getFirst(ui32Begin, ui32End, true)) ) {
        printf ("\t\t-Error: getFirst method failed\n\n");
        exit(1);
    }
    printf ("\t\t%d-%d ", ui32Begin, ui32End);
    while ( 0 == (pRangeList->getNext(ui32Begin, ui32End)) ) {
        printf ("%d-%d ", ui32Begin, ui32End);	
    }

//#################################################################################################
    //Test to see if the list contains an element within the range in the list: 3.
    //Expected: TRUE
    printf ("\n\n\t+ See if the list contains the element 3 within the range(s) (expected: TRUE)\n");
    if ( pRangeList->hasTSN(3) ) {
        printf ("\t\t-The list contains 3\n\n");
    }
    else {
        printf ("\t\t-The list does not contain 3\n\n");
    }

//#################################################################################################
    //Test to see if the list contains an element within the range in the list: 12.
    //Expected: FALSE
    printf ("\t+ See if the list contains the element 13 within the range(s) (expected: FALSE)\n");
    if ( pRangeList->hasTSN(13) ) {
        printf ("\t\t-The list contains 13\n\n");
    }
    else {
        printf ("\t\t-The list does not contain 13\n\n");
    }	

//#################################################################################################
    //Test the getFirst method. Expected Range: 0-12
    printf ("\t+ Test the getFirst() method (Expected Range: 0-12)\n");
    if (0 == ( pRangeList->getFirst(ui32Begin, ui32End, true)) ) {
        printf ("\t\t-Begin = %d, End = %d\n\n", ui32Begin, ui32End);
    }
    else {
        printf ("\t\t-Error: getFirst method failed\n\n");
        exit(1);
    }

//#################################################################################################
    //Test the getNext method. Expected Range 16-16
    printf ("\t+ Test the getNext() method (Expected Range: 16-16)\n");
    if (0 == ( pRangeList->getNext(ui32Begin, ui32End)) ) {
        printf ("\t\t-Begin = %d, End = %d\n\n", ui32Begin, ui32End);
    }
    else {
        printf ("\t\t-Error: getNext method failed\n\n");
        exit(1);
    }

//#################################################################################################
    //Add elements 14, 15
    printf ("\t+ Adding the elements 14, and 15 to the list. List should now be 0-12 14-16\n");
    if ( !(pRangeList->addTSN (14)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add element 14\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the element 14 successfully\n");
    }
    if ( !(pRangeList->addTSN (15)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add element 15\n");
        exit(1);
    }
    else {
        printf ("\t\t-Added the element 15 successfully\n\n");
    }
	
//#################################################################################################
    //Print out the contents of the list: Expected 0-12 14-16
    printf ("\t+ Print out the contents of the list (Expected: 0-12 14-16)\n");
    if (0 != ( pRangeList->getFirst(ui32Begin, ui32End, true)) ) {
        printf ("\t\t-Error: getFirst method failed\n\n");
        exit(1);
    }
    printf ("\t\t%d-%d ", ui32Begin, ui32End);
    while ( 0 == (pRangeList->getNext(ui32Begin, ui32End)) ) {
        printf ("%d-%d ", ui32Begin, ui32End);	
    }

//#################################################################################################
    //Test the getFirst method. Expected Range: 0-12
    printf ("\n\n\t+ Test the getFirst() method (Expected Range: 0-12)\n");
    if (0 == ( pRangeList->getFirst(ui32Begin, ui32End, true)) ) {
        printf ("\t\t-Begin = %d, End = %d\n\n", ui32Begin, ui32End);
    }
    else {
        printf ("\t\t-Error: getFirst method failed\n\n");
        exit(1);
    }

//#################################################################################################
    //Test the getNext method. Expected 14-16
    printf ("\t+ Test the getNext() method (Expected Range: 14-16)\n");
    if (0 == ( pRangeList->getNext(ui32Begin, ui32End)) ) {
        printf ("\t\t-Begin = %d, End = %d\n\n", ui32Begin, ui32End);
    }
    else {
        printf ("\t\t-Error: getNext method failed\n\n");
        exit(1);
    }

//#################################################################################################
    //Test the removeTSN method. Remove from the beginnging of the list, end, and middle
    printf ("\t+Test the removeTSN() method\n");
    printf ("\t\t-Remove from the beginning of the list: 1\n");
    pRangeList->removeTSN (1);
    listRanges (*pRangeList,  "0-0 2-12 14-16");
	
//#################################################################################################
    //Remove from the end of the list: 16
    printf ("\n\t\t-Remove from the end of the list: 16\n");
    pRangeList->removeTSN(16);
    listRanges (*pRangeList,  "0-0 2-12 14-15");

//#################################################################################################
    //Remove from the middle of the list: 5
    printf ("\n\t\t-Remove from the middle of the list: 5\n");
    pRangeList->removeTSN(5);
    listRanges (*pRangeList,  "0-0 2-4 6-12 14-15");

//#################################################################################################
    //Add a single range 20-20 and remove 20 from the list. Expected: Whole range is deleted
    printf ("\n\n\tAdding range 20-20 to list\n");
    if ( !(pRangeList->addTSN (20, 20)) ) {
        printf ("\t\t-Error: TestRangeDLList.cpp ::main: Failed to add the range 20-20\n");
        exit(1);
    }
    else {
            printf ("\t\t-Added the range 20-20 successfully\n");
    }
    printf ("\t\t- Print out the contents of the list (Expected: )\n");
    listRanges (*pRangeList,  "0-0 2-4 6-12 14-15 20-20");

    //Remove the range 20-20
    printf ("\n\n\t\t-Remove from the list: 20\n");
    pRangeList->removeTSN(20);
    printf ("\t\t- Print out the contents of the list (Expected: )\n");
    listRanges (*pRangeList,  "0-0 2-4 6-12 14-15");

    printf ("\n\n\t\t-Remove from the list: 0, and add 0-20\n");
    pRangeList->removeTSN(0);
    pRangeList->addTSN (0, 20);
    printf ("\t\t- Print out the contents of the list (Expected: 0-20)\n");
    listRanges (*pRangeList,  "0-20");

    printf ("\n\n\t\t-Remove from the list: 9,10,11,12\n");
    pRangeList->removeTSN (9);
    pRangeList->removeTSN (10);
    pRangeList->removeTSN (11);
    pRangeList->removeTSN (12);
    //Print out the contents of the list: Expected 0-8 13-20
    printf ("\t\t- Print out the contents of the list (Expected: )\n");
    listRanges (*pRangeList,  "0-8 13-20");

    // Add non-overlapping range in the middle of the list
    printf ("\n\n\t\t-Add to the list: 10,11\n");
    pRangeList->addTSN (10,11);
    listRanges (*pRangeList,  "0-8 10-11 13-20");

    // Add non-overlapping range at the end of the list
    printf ("\n\n\t\t-Add to the list: 60,70\n");
    pRangeList->addTSN (60,70);
    listRanges (*pRangeList,  "0-8 10-11 13-20 60-70");

    // Add non-overlapping range in the middle of the list
    printf ("\n\n\t\t-Add to the list: 30,40\n");
    pRangeList->addTSN (30,40);
    listRanges (*pRangeList, "0-8 10-11 13-20 30-40 60-70");

    // Add overlapping range in the middle of the list
    printf ("\n\n\t\t-Add to the list: 40,60\n");
    pRangeList->addTSN (40,60);
    listRanges (*pRangeList, "0-8 10-11 13-20 30-70");

    // Add non-overlapping range at the end of the list
    printf ("\n\n\t\t-Add to the list: 80,90\n");
    pRangeList->addTSN (80,90);
    listRanges (*pRangeList, "0-8 10-11 13-20 30-70 80-90");

    // Add range overlapping multiple ranges in the middle of the list
    printf ("\n\n\t\t-Add to the list: 35,81\n");
    pRangeList->addTSN (35,81);
    listRanges (*pRangeList, "0-8 10-11 13-20 30-90");

    // Removing range from the beginning of the list
    printf ("\n\n\t\t-Remove from the list: 0,8\n");
    pRangeList->removeTSN (0, 8);
    listRanges (*pRangeList, "10-11 13-20 30-90");

    // Add non-overlapping range at the beginning of the list
    printf ("\n\n\t\t-Add element to the list: 0,8\n");
    pRangeList->addTSN (0, 8);
    listRanges (*pRangeList, "0-8 10-11 13-20 30-90");

    // Removing range from the beginning of the list
    printf ("\n\n\t\t-Remove from the list: 0,8\n");
    pRangeList->removeTSN (0, 8);
    listRanges (*pRangeList, "10-11 13-20 30-90");

    // Add overlapping range at the beginning of the list
    printf ("\n\n\t\t-Add element to the list: 0,10\n");
    pRangeList->addTSN (0, 10);
    listRanges (*pRangeList, "0-11 13-20 30-90");

    // Add non-overlapping, but merging, range in the middle of the list
    printf ("\n\n\t\t-Add element to the list: 21-29\n");
    pRangeList->addTSN (21, 29);
    listRanges (*pRangeList, "0-11 13-90");

    if (bUseSequentialArithmetic) {
    }
    else {
        // Add range overlapping the maximum possible value
        printf ("\n\n\t\t-Add element to the list: (0xFFFFFFFF-10)-0xFFFFFFFF\n");
        pRangeList->addTSN ((0xFFFFFFFF-10), 0xFFFFFFFF);
        listRanges (*pRangeList, "0-11 13-90 4294967285-4294967295");

        printf ("\n\n\t\t-Removing element from the list: 0-12\n");
        pRangeList->removeTSN (0, 12);
        listRanges (*pRangeList, "13-90 4294967285-4294967295");

        printf ("\n\n\t\t-Add element to the list: 0-5\n");
        pRangeList->addTSN (0, 5);
        listRanges (*pRangeList, "0-5 13-90 4294967285-4294967295");

        printf ("\n\n\t\t-Add element to the list: 0-5\n");
        pRangeList->addTSN (0, 5);
        listRanges (*pRangeList, "0-5 13-90 4294967285-4294967295");

        printf ("\n\n\t\t-Add element to the list: 0-6\n");
        pRangeList->addTSN (0, 6);
        listRanges (*pRangeList, "0-6 13-90 4294967285-4294967295");

        // Add range overlapping the whole list
        printf ("\n\n\t\t-Add element to the list: 0-4294967295\n");
        pRangeList->addTSN (0, 4294967295);
        listRanges (*pRangeList, "0-4294967295");
    }

    printf ("\n");
    return 0;
}

int main (int argc, char *argv[])
{
    test (true);
    test (false);
}

