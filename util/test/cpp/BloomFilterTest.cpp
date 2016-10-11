/*
**************************************************************************
*                                                                        *
*                           Open Bloom Filter                            *
*                                                                        *
* Description: Basic Bloom Filter Usage                                  *
* Author: Arash Partow - 2000                                            *
* URL: http://www.partow.net                                             *
* URL: http://www.partow.net/programming/hashfunctions/index.html        *
*                                                                        *
* Copyright notice:                                                      *
* Free use of the Bloom Filter Library is permitted under the guidelines *
* and in accordance with the most current version of the Common Public   *
* License.                                                               *
* http://www.opensource.org/licenses/cpl1.0.php                          *
*                                                                        *
**************************************************************************
*/

#include "BloomFilter.h"

#include <assert.h>
#include <iostream>
#include <string>

using namespace NOMADSUtil;

template<class T>
void query (BloomFilter &filter, const T &element, const bool bExpected)
{
    const bool bContains = filter.contains (element);
    const char *pszMsg = bContains ?
                         "BF contains: " : "BF does not contain: ";
    std::cout << pszMsg << element << std::endl;
    assert (bExpected == bContains);
}

/*
 * Description: This example demonstrates basic usage of the Bloom filter.
 * Initially some values are inserted then they are subsequently
 * queried, noting any false positives or errors.
 */
int simpleTest (void)
{
    BloomParameters parameters;
    parameters.projected_element_count = 1000;   // How many elements roughly do we expect to insert?
    parameters.false_positive_probability = 0.0001; // Maximum tolerable false positive probability? (0,1)
                                                    // 1 in 10000
    parameters.random_seed = 0xA5A5A5A5;  // Simple randomizer (optional)
    if (!parameters) {
        std::cout << "Error - Invalid set of bloom filter parameters!" << std::endl;
        return 1;
    }
    parameters.compute_optimal_parameters();

    BloomFilter filter (parameters);

    // Insert strings and numbers
    std::string str_list[] = { "AbC", "iJk", "XYZ" };
    for (std::size_t i = 0; i < (sizeof (str_list) / sizeof (std::string)); ++i) {
        filter.insert (str_list[i]);
    }
    for (std::size_t i = 0; i < 100; ++i) {
        filter.insert (i);
    }

    // Query existing elements
    for (std::size_t i = 0; i < (sizeof (str_list) / sizeof (std::string)); ++i) {
        query<std::string>(filter, str_list[i], true);
    }
    for (std::size_t i = 0; i < 100; ++i) {
        query<std::size_t> (filter, i, true);
    }

    // Query non existing elements
    std::string invalid_str_list[] = { "AbCX", "iJkX", "XYZX" };
    for (std::size_t i = 0; i < (sizeof (invalid_str_list) / sizeof (std::string)); ++i) {
        query<std::string> (filter, invalid_str_list[i], false);
    }
    for (int i = -1; i > -100; --i) {
        query<int> (filter, i, false);
    }

    return 0;
}

int main (int argc, char **ppszArgv)
{
    simpleTest();

    return 0;
}


