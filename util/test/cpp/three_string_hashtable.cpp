#include "three_string_hashtable.h"

#include "ThreeStringHashtable.h"
#include "StrClass.h"


int main (int argc, char **argv)
{
    ThreeStringHashtable<MyClass> h;

    MyClass *pmc = new MyClass();
    pmc->i=3;


    h.get("one", "two", "asdda");
    h.put("one", "two", "three", pmc);

    return 0;
}
