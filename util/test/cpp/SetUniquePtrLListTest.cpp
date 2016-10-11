#include "SetUniquePtrLList.h"

#include <stdio.h>

using namespace NOMADSUtil;

struct Integer {

    Integer (char chKey, int val);

    bool operator> (const Integer &other) const;
    bool operator< (const Integer &other) const;
    bool operator== (const Integer &other) const;

    char chKey;
    int val;
};

Integer::Integer (char chKey, int val)
{
    this->chKey = chKey;
    this->val = val;
}

bool Integer::operator>(const Integer &other) const
{
    return (val > other.val);
}

bool Integer::operator<(const Integer &other) const
{
    return (val < other.val);
}

bool Integer::operator==(const Integer &other) const
{
    return (chKey == other.chKey);
}

void display  (SetUniquePtrLList<Integer> &l)
{
    Integer *pCurr = l.getFirst();
    while (pCurr != NULL) {
        printf ("%d (%c)\t", pCurr->val, pCurr->chKey);
        pCurr = l.getNext();
    }
    printf ("\n");
}

int main(int argc, const char *argv[])
{
    SetUniquePtrLList<Integer> l;

    l.append (new Integer ('a', 2));
    l.append (new Integer ('b', 3));
    l.append (new Integer ('c', 1));
    l.append (new Integer ('b', 3));
    l.prepend (new Integer ('d', 5), true);
    l.prepend (new Integer ('d', 5), true);
    display (l);
    printf ("Changing the 2 element to 3 - should NOT change\n");
    l.prepend (new Integer ('a', 3), true);
    display (l);

    printf ("=============================================\n");

    SetUniquePtrLList<Integer> l2 (true, true);

    l2.append (new Integer ('a', 2));
    display (l2);
    l2.append (new Integer ('b', 3));
    display (l2);
    l2.append (new Integer ('c', 1));
    display (l2);
    l2.append (new Integer ('b', 3));
    display (l2);
    l2.prepend (new Integer ('d', 5), true);
    display (l2);
    l2.prepend (new Integer ('d', 5), true);
    display (l2);
    printf ("Changing the 2 element to 3 - should change\n");
    l2.prepend (new Integer ('a', 3), true);
    display (l2);

    printf ("=============================================\n");

    SetUniquePtrLList<Integer> l3 (false, false);

    l3.insertUnique (new Integer ('a', 4));
    display (l3);
    l3.insertUnique (new Integer ('b', 3));
    display (l3);
    l3.insertUnique (new Integer ('c', 2));
    display (l3);
    l3.insertUnique (new Integer ('a', 4));
    display (l3);
    l3.insertUnique (new Integer ('b', 3));
    display (l3);
    l3.insertUnique (new Integer ('c', 2));
    display (l3);

    display (l3);
    printf ("Changing the 4 element to 1 - should NOT change\n");
    l3.insertUnique (new Integer ('a', 1));
    display (l3);

    printf ("=============================================\n");

    SetUniquePtrLList<Integer> l4 (false, true);

    l4.insertUnique (new Integer ('a', 4));
    display (l4);
    l4.insertUnique (new Integer ('b', 3));
    display (l4);
    l4.insertUnique (new Integer ('c', 2));
    l4.insertUnique (new Integer ('a', 4));
    l4.insertUnique (new Integer ('b', 3));
    l4.insertUnique (new Integer ('c', 2));

    display (l4);
    printf ("Changing the 4 element to 1 - should change\n");
    l4.insertUnique (new Integer ('a', 1));
    display (l4);
    
    l4.insertUnique (new Integer ('a', 0));
    display (l4);

    l4.insertUnique (new Integer ('b', -1));
    display (l4);

    return 0;
}
