#include <stdio.h>

#include "NLFLib.h"
#include "TimeBoundedStringHashset.h"

int main (int argc, char *argv[])
{
    NOMADSUtil::TimeBoundedStringHashset tbsh (10000UL, 4);
    tbsh.put ("a");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("b");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("c");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("d");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("e");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("f");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("g");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("h");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("i");
    tbsh.printStructure();
    printf ("\n");
    tbsh.put ("j");
    tbsh.printStructure();
    printf ("\n");
    for (int i = 'A'; i <= 'Z'; i++) {
        NOMADSUtil::sleepForMilliseconds (1000);
        char str[2];
        str[0] = i;
        str[1] = '\0';
        tbsh.put (str);
        tbsh.printStructure();
        printf ("\n");
    }
    for (int i = 0; i < 20; i++) {
        NOMADSUtil::sleepForMilliseconds (1000);
        printf ("Still contains: ");
        for (int j = 'A'; j <= 'Z'; j++) {
            char str[2];
            str[0] = j;
            str[1] = '\0';
            if (tbsh.containsKey (str)) {
                printf ("%s ", str);
            }
        }
        printf ("\n\n");
    }
    return 0;
}
