#include "IntervalTree.h"
#include "RangeDLList.h"

#include "DArray2.h"
#include "MovingAverage.h"

#include <time.h>

#define IntervalType uint32
#define RangeTree IntervalTree<IntervalType>
#define Intervals DArray2<RangeTree::Interval>

using namespace NOMADSUtil;

namespace IntervalTreeTest
{
    class IntervalGenerator
    {
        public:
            IntervalGenerator (void) {};
            virtual ~IntervalGenerator (void) {};

            virtual bool getNext (IntervalType &begin, IntervalType &end) = 0;
            virtual const char * getName (void) = 0;
    };

    class TrainIntervalGenerator : public IntervalGenerator
    {
        public:
            TrainIntervalGenerator (unsigned int uiNElements, IntervalType pktSize, double dLossProb)
                : _uiNElements (uiNElements), _uiCounter (0), _curr (0), _pktSize (pktSize), _dLossProb (dLossProb) {};
            ~TrainIntervalGenerator (void) {};

            bool getNext (IntervalType &begin, IntervalType &end)
            {
                while (_uiCounter < _uiNElements) {
                    begin = _curr;
                    end = begin + _pktSize;
                    _curr += _pktSize;
                    _uiCounter++;
                    int iRnd = rand() % 100;
                    if (iRnd < _dLossProb) {
                        // drop
                    }
                    else {
                        return true;
                    }
                }
                return false;
            }

            const char * getName (void) { return "TrainIntervalGenerator";  }
            void reset () { _uiCounter = 0;  }

        private:
            const unsigned int _uiNElements;
            unsigned int _uiCounter;
            IntervalType _curr;
            IntervalType _end;
            const IntervalType _pktSize;
            const double _dLossProb;
    };

    class MultiTrainIntervalGenerator : public IntervalGenerator
    {
        public:
            MultiTrainIntervalGenerator (unsigned int uiNTrains, unsigned int uiNElements, IntervalType pktSize, double dLossProb)
                : _uiNTrains (uiNTrains), _uiCounter (0), _gen (uiNElements, pktSize, dLossProb) {};

            bool getNext (IntervalType &begin, IntervalType &end)
            {
                for (; _uiCounter < _uiNTrains; _uiCounter++) {
                    if (_gen.getNext (begin, end)) {
                        return true;
                    }
                    _gen.reset();
                }
                return false;
            }

            const char * getName (void) { return "MultiTrainIntervalGenerator"; }

        private:
            const unsigned int _uiNTrains;
            unsigned int _uiCounter;
            TrainIntervalGenerator _gen;
    };

    class RandomIntervalGenerator : public IntervalGenerator
    {
        public:
            RandomIntervalGenerator (unsigned int uiNElements, IntervalType pktSize)
                : _uiNElements (uiNElements), _uiCounter (0), _pktSize (pktSize) {};
            ~RandomIntervalGenerator (void) {}

            bool getNext (IntervalType &begin, IntervalType &end)
            {
                if (_uiCounter < _uiNElements) {
                    // Generate random number between (uiNElements * pktSize)
                    static const uint64 ui64Range = (_uiNElements * _pktSize) - _pktSize;
                    begin = (rand() % ui64Range);
                    end = begin + _pktSize;
                    _uiCounter++;
                    return true;
                }
                return false;
            }

            const char * getName (void) { return "RandomIntervalGenerator"; }

       private:
            const unsigned int _uiNElements;
            unsigned int _uiCounter;
            const IntervalType _pktSize;
    };

    struct Adder {
        Adder (void){}
        virtual ~Adder (void) {}
        virtual bool operator()(IntervalType begin, IntervalType end) = 0;
        virtual void reset() = 0;
    };

    struct IntervalTreeAdder : public Adder {
        explicit IntervalTreeAdder (bool bBalance) : tree (true, bBalance) {}
        bool operator()(IntervalType begin, IntervalType end)
        {
            return (tree.insert (begin, end) == 0);
        }
        void reset() { tree.reset(); }
        private:
            RangeTree tree;
    };

    struct IntervalListAdder : public Adder {
        IntervalListAdder (void) : list (false) {}
        bool operator()(IntervalType begin, IntervalType end)
        {
            return (list.addTSN (begin, end) >= 0);
        }
        void reset () { list.reset(); }
        private:
            UInt32RangeDLList list;
    };

    struct Results
    {
        explicit Results (unsigned int nIntervals) : uiNIntervals (nIntervals) {};
        Results (Results &res) : uiNIntervals (res.uiNIntervals),
            ba2t (res.ba2t), ua2t (res.ua2t), a2l (res.a2l){};
        ~Results (void) {};
        const unsigned int uiNIntervals;
        double ba2t, ua2t, a2l;
    };

    double test (Intervals &intervals, unsigned short usNRuns, Adder &add)
    {
        MovingAverage<int64> avg (usNRuns);
        for (unsigned short r = 0; r < usNRuns; r++) {
            int64 i64Start = getTimeInMilliseconds();
            for (unsigned int i = 0; i < intervals.size (); i++) {
                assert (add (intervals[i].begin, intervals[i].end));
            }
            int64 i64Elapsed = getTimeInMilliseconds() - i64Start;
            add.reset();
            if (r > 0) {
                // Skip the first test to normalize for caching
                avg.add (i64Elapsed);
            }
        }
        return avg.getAverage();
    }

    Results test (IntervalGenerator &gen, unsigned short usNRuns)
    {
        // Generate intervals
        Intervals intervals;
        unsigned int uiBegin, uiEnd;
        for (unsigned int i = 0; gen.getNext (uiBegin, uiEnd); i++) {
            intervals[i].begin = uiBegin;
            intervals[i].end = uiEnd;
        }

        // Create configurations
        IntervalTreeAdder ba2t (true);  // balanced add to tree
        IntervalTreeAdder ua2t (false); // unbalanced add to tree
        IntervalListAdder a2l;          // add to list

        // Run tests
        Results res (intervals.size());
        res.ba2t = test (intervals, usNRuns, ba2t);
        res.ua2t = test (intervals, usNRuns, ua2t);
        res.a2l = test (intervals, usNRuns, a2l);

        return res;
    }

    void printResults (IntervalGenerator &gen, unsigned short N_RUNS)
    {
        Results res (test (gen, N_RUNS));
        printf ("[  Balanced Interval Tree  ] %s: inserting %u intervals took %f milliseconds.\n", gen.getName(), res.uiNIntervals, res.ba2t);
        printf ("[ Unbalanced Interval Tree ] %s: inserting %u intervals took %f milliseconds.\n", gen.getName(), res.uiNIntervals, res.ua2t);
        printf ("[      Interval List       ] %s: inserting %u intervals took %f milliseconds.\n", gen.getName(), res.uiNIntervals, res.a2l);
        printf ("\n");
    }
}

using namespace IntervalTreeTest;

int main (int argc, char **ppszArgv)
{
    srand (time (NULL));

    unsigned int N_ELEMENTS = 100000;
    unsigned int SIZE = 1000;
    unsigned int N_RUNS = 10;

    printf ("Input: set of in-order contiguous ranges.\n");
    printf ("Expectation: similar performance, and balancing should not matter, since the intervals should collapse to a single one.\n");
    TrainIntervalGenerator gen0 (N_ELEMENTS, SIZE, -1.0);  // no drop
    printResults (gen0, N_RUNS);

    //-----------------------------------------------------

    printf ("Input: set of in-order non contiguous ranges.\n");
    printf ("Expectation: similar performance, and balancing should matter because the intervals are added in-order, so if there is no balancing"
            "the resulting tree should look like a linked list of right branches.\nThe DArray list should even do better becasuse it has a reference to the"
            "tail of the list, and exploits the fact that the train is (mostly) in-order.\n");
    TrainIntervalGenerator gen1 (N_ELEMENTS, SIZE, 15.0);
    printResults (gen1, N_RUNS);

    //-----------------------------------------------------

    printf ("Input: set of in-order non contiguous ranges.\n");
    printf ("Expectation: similar performance, and balancing should matter because the intervals are added in-order, so if there is no balancing"
        "the resulting tree should look like a linked list of right branches.\nThe DArray list should even do better becasuse it has a reference to the"
        "tail of the list, and exploits the fact that the train is (mostly) in-order.\n");
    MultiTrainIntervalGenerator gen7 (10, N_ELEMENTS, SIZE, 15.0);
    printResults (gen7, N_RUNS);

    //-----------------------------------------------------

    printf ("Input: set of in-order non contiguous ranges.\n");
    printf ("Expectation: similar performance, and balancing should matter because the intervals are added in-order, so if there is no balancing"
        "the resulting tree should look like a linked list of right branches.\nThe DArray list should even do better becasuse it has a reference to the"
        "tail of the list, and exploits the fact that the train is (mostly) in-order.\n");
    TrainIntervalGenerator gen2 (N_ELEMENTS, SIZE, 85.0);
    printResults (gen2, N_RUNS);

    //-----------------------------------------------------

    printf ("Input: set of in-order non contiguous ranges.\n");
    printf ("Expectation: similar performance, and balancing should matter because the intervals are added in-order, so if there is no balancing"
        "the resulting tree should look like a linked list of right branches.\nThe DArray list should even do better becasuse it has a reference to the"
        "tail of the list, and exploits the fact that the train is (mostly) in-order.\n");
    TrainIntervalGenerator gen3 (N_ELEMENTS, SIZE, 95.0);
    printResults (gen3, N_RUNS);

    //-----------------------------------------------------

    printf ("Input: set of in-order non contiguous ranges.\n");
    printf ("Expectation: similar performance, and balancing should matter because the intervals are added in-order, so if there is no balancing"
        "the resulting tree should look like a linked list of right branches.\nThe DArray list should even do better becasuse it has a reference to the"
        "tail of the list, and exploits the fact that the train is (mostly) in-order.\n");
    MultiTrainIntervalGenerator gen4 (10, N_ELEMENTS, SIZE, 95.0);
    printResults (gen4, N_RUNS);

    //-----------------------------------------------------

    printf ("Input: set of random-order potentially overlapping ranges.\n");
    printf ("Expectation: the performance of the balanced tree should be much better, because the average number of elements accesses is O(log(n)) worst case Vs O(n) in the list case.\n");
    RandomIntervalGenerator gen5 (N_ELEMENTS, SIZE);
    printResults (gen5, N_RUNS);

    //-----------------------------------------------------

    printf ("Input: set of random-order potentially overlapping ranges.\n");
    printf ("Expectation: the performance of the balanced tree should be much better, because the average number of elements accesses is O(log(n)) worst case Vs O(n) in the list case.\n");
    RandomIntervalGenerator gen6 (N_ELEMENTS * 2, SIZE);
    printResults (gen6, N_RUNS);

    return 0;
}

