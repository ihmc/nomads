#include "MSPAlgorithm.h"
#include "StringHashthing.h"
#include "StringHashgraph.h"
#include "StringHashtable.h"
#include "Graph.h"
#include "Thing.h"

#include <string.h>

using namespace NOMADSUtil;

int main (int argc, char *argv[])
{
    StringHashgraph * pGraph = new StringHashgraph ();
    if (pGraph->isDirect()) {
        printf ("It's Direct\n");
    }
    else {
        printf ("It'n NOT direct\n");
    }

    Thing * pT1 = new StringHashthing (pGraph, "T1");
    pGraph->put ("T1", pT1);

    Thing * pT2 = new StringHashthing (pGraph, "T2");
    pGraph->put ("T2", pT2);

    Thing * pT3 = new StringHashthing (pGraph, "T3");
    pGraph->put ("T3", pT3);

    Thing * pT4 = new StringHashthing (pGraph, "T4");
    pGraph->put ("T4", pT4);

    Thing * pT5 = new StringHashthing (pGraph, "T5");
    pGraph->put ("T5", pT5);

    Thing * pT6 = new StringHashthing (pGraph, "T6");
    pGraph->put ("T6", pT6);

    pT1->put (pT2->getId(), pT2);       // 1 <-> 2
    pT1->put (pT3->getId(), pT3);       // 1 <-> 3
    pT2->put (pT3->getId(), pT3);       // 2 <-> 3
    pT3->put (pT4->getId(), pT4);       // 3 <-> 4
    pT4->put (pT5->getId(), pT5);       // 4 <-> 5
    pT4->put (pT6->getId(), pT6);       // 4 <-> 6
    pT5->put (pT1->getId(), pT1);       // 5 <-> 1
    pT6->put (pT1->getId(), pT1);       // 6 <-> 1

    int j = 0;
    Thing *pThing;
    const char * pszId;
    for (StringHashtable<Thing>::Iterator i = pGraph->thingIterator (); !i.end(); i.nextElement()) {
        pszId = i.getKey();
        pThing = i.getValue();
        if (strcmp (pszId, pThing->getId()) == 0) {
            j++;
            printf ("Graph: element %d: <%s>\n", j, pszId, pThing->getId());
            for (StringHashtable<Thing>::Iterator i2 = pThing->iterator (); !i2.end(); i2.nextElement()) {
                const char * pszNeighbor = i2.getKey ();
                printf ("\tArc from %s to %s\n", pszId, pszNeighbor);
            }
        }
        else {
            printf ("ERROR: <%s> <%s>\n", pszId, pThing->getId());
        }
    }
    StringHashgraph mstGraph;
    if (pGraph->isDirect()) {
        printf ("MST is Direct\n");
    }
    else {
        printf ("MST is NOT direct\n");
    }
    MSPAlgorithm::MSPPrimAlgorithm (pGraph, mstGraph, pT1);
    j = 0;
    for (StringHashtable<Thing>::Iterator i = mstGraph.thingIterator (); !i.end(); i.nextElement()) {
        pszId = i.getKey();
        pThing = i.getValue();
        if (strcmp (pszId, pThing->getId()) == 0) {
            j++;
            printf ("MSTGraph: element %d: <%s>\n", j, pszId, pThing->getId());
            for (StringHashtable<Thing>::Iterator i2 = pThing->iterator (); !i2.end(); i2.nextElement()) {
                const char * pszNeighbor = i2.getKey ();
                printf ("\tArc from %s to %s\n", pszId, pszNeighbor);
            }
        }
        else {
            printf ("ERROR: <%s> <%s>\n", pszId, pThing->getId());
        }
    }      
}

