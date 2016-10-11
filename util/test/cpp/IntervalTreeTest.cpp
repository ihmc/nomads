#include "IntervalTree.h"
#include "DArray2.h"
#include <time.h>

#define RangeTree IntervalTree<unsigned int>
#define Intervals DArray2<RangeTree::Interval>
#define forEach(collection) for (unsigned int i = 0; i < collection.size(); i++)

using namespace NOMADSUtil;

namespace IntervalTreeTest
{
    unsigned int getRandomInt (void)
    {
        // Generate random number between 0 and 999
        return (rand () % 1000);
    }

    void swap (unsigned int &uiBegin, unsigned int &uiEnd)
    {
        uiBegin += uiEnd;
        uiEnd = uiBegin - uiEnd;
        uiBegin -= uiEnd;
    }

    void getRandomRange (unsigned int &uiBegin, unsigned int &uiEnd)
    {
        uiBegin = getRandomInt();
        uiEnd = getRandomInt();
        if (uiBegin > uiEnd) {
            swap (uiBegin, uiEnd);
        }
    }

    bool checkTree (RangeTree &tree)
    {
        unsigned int uiBegin, uiEnd, uiPrev;
        RangeTree::Iterator iter = tree.getAllElements (TreeUtil::IN_ORDER);
        if (!iter.getNext (uiBegin, uiEnd)) {
            return -1;
        }
        for (uiPrev = uiBegin; iter.getNext (uiBegin, uiEnd); uiPrev = uiBegin) {
            if (uiPrev > uiBegin) {
                printf ("In order traversal found out-of-order intervals: [%u, x], [%u %u]\n",
                       uiPrev, uiBegin, uiEnd);
                return false;
            }
        }
        return true;
    }

    int testBalanced (Intervals &intervals, bool bMergeUponInsertion)
    {
        DArray<double> priorities;
        forEach (intervals) {
            priorities[i] = rand();
        }

        // Fill Tree
        unsigned int uiErr = 0;
        RangeTree tree (bMergeUponInsertion, true);
        forEach (intervals) {
            if (tree.insert (intervals[i].begin, intervals[i].end, priorities[i]) < 0) {
                uiErr = i;
                break;
            }
        }
        printf ("Created tree with %u elements\n", tree.getCount());

        if (!checkTree (tree)) {
            RangeTree dbgTree (bMergeUponInsertion, true);
            forEach (intervals) {
                if (uiErr == i) {
                    printf ("stop");
                }
                dbgTree.insert (intervals[i].begin, intervals[i].end, priorities[i]);
            }
            return -2;
        }
        
        return 0;
    }

    int testBalanced (Intervals &intervals)
    {
        printf ("\nBALANCED\n");

        printf ("Without merging\n");
        assert (testBalanced (intervals, false) == 0);

        printf ("With merging\n");
        assert (testBalanced (intervals, true) == 0);

        return 0;
    }

    int testUnbalanced (Intervals &intervals, bool bMergeUponInsertion)
    {
        /*
        static const unsigned short BEGIN = 0;
        static const unsigned short END = 1;
        static const unsigned int N = 5;
        unsigned int intervals[N][2] = { { 6, 8 }, { 1, 9 }, { 2, 4 }, { 7, 10 }, { 4, 7 } };*/

        // Fill Tree
        RangeTree tree (bMergeUponInsertion, false);
        forEach (intervals) {
            tree.insert (intervals[i].begin, intervals[i].end);
        }
        unsigned int uiBegin, uiEnd, uiPrev;
        if (!checkTree (tree)) {
            return -2;
        }

        printf ("Level-order list:\n");
        tree.print (stdout);

        printf ("\nCompact list:\n");
        PtrLList<RangeTree::Node> mergedIntervals;
        tree.getCompactList (mergedIntervals);
        for (RangeTree::Node *pCurr = mergedIntervals.getFirst(); pCurr != NULL; pCurr = mergedIntervals.getNext()) {
            printf ("[%u %u] ", pCurr->interval.begin, pCurr->interval.end);
        }
        printf ("\n");
        return 0;
    }

    int testUnbalanced (Intervals &intervals)
    {
        printf ("\nUNBALANCED\n");

        printf ("Without merging\n");;
        assert (testUnbalanced (intervals, false) == 0);

        printf ("With merging\n");
        assert (testUnbalanced (intervals, true) == 0);

        return 0;
    }
}

using namespace IntervalTreeTest;

int main (int argc, char **ppszArgv)
{
    // Generate random intervals
    srand (time (NULL));
    Intervals intervals;
    for (unsigned int i = 0; i < 1000; i++) {
        unsigned int uiBegin, uiEnd;
        getRandomRange (uiBegin, uiEnd);
        intervals[i].begin = uiBegin;
        intervals[i].end = uiEnd;
    }

    assert (testUnbalanced (intervals) == 0);
    assert (testBalanced (intervals) == 0);

    return 0;
}

