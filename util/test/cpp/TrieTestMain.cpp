#include <iostream>
#include <bitset>
#include <vector>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <sstream>
#include <iterator>
#include <random>
#include <thread>

#include "Trie.h"
#include "InetAddr.h"
#include "NLFLib.h"


std::vector<int> get_bits (uint32 x)
{
    std::string chars(std::bitset<sizeof(uint32) * CHAR_BIT>(x).to_string (char(0), char(1)));

    return std::vector<int>(chars.begin(), chars.end());
}

void do_complete_string_lookup_test (const unsigned int ui32RandPrefix, const unsigned int ui32RandIP, Trie<char, std::string> & trieTest) {
    InetAddr iaIP(ui32RandPrefix);
    auto bitRepresentation = get_bits (htonl (ui32RandPrefix));
    std::stringstream strBitRepr;
    std::copy(bitRepresentation.begin(), bitRepresentation.end(), std::ostream_iterator<int>(strBitRepr, ""));

    std::string str;
    trieTest.addVal (strBitRepr.str().c_str(), strBitRepr.str().length(), InetAddr(ui32RandIP).getIPAsString());
    for (int i = 0; i <= strBitRepr.str().length(); ++i) {
        bool bMatch = trieTest.searchVal (strBitRepr.str().c_str(), i, str);
        if (bMatch && (i != strBitRepr.str().length())) {
            std::cerr << "Complete string lookup test FAILED! i=" << i << " input=\"" <<
                strBitRepr.str() << "\" output=\"" << str << "\"" << std::endl;
            exit (EXIT_FAILURE);
        }
        else if (bMatch && (str != InetAddr(ui32RandIP).getIPAsString())) {
            std::cerr << "Complete string lookup test FAILED! Output mismatch! i=" << i << " input=\"" <<
                strBitRepr.str() << "\" output=\"" << str << "\" inserted value=\"" <<
                InetAddr(ui32RandIP).getIPAsString() << "\"" << std::endl;
            exit (EXIT_FAILURE);
        }
    }
}

void do_longest_prefix_search_test (const unsigned int runs) {
    std::random_device rd;                                                  // Random number from hardware
    std::mt19937 rndGen(rd());                                              // Seed the generator
    std::uniform_int_distribution<unsigned short> unifIntDistrBitRng(0, 31);              // Random within range [0-31]
    std::uniform_int_distribution<unsigned int> unifIntDistrIPRng(1, ULONG_MAX);        // Random within range [0.0.0.1-255.255.255.255]

    for (int run = 0; run < runs; ++run) {
        Trie<char, std::string> trieTest;
        uint32 ui32RandPrefix = unifIntDistrIPRng (rndGen), ui32RandIP = unifIntDistrIPRng (rndGen);
        int bitmaskBits = unifIntDistrBitRng (rndGen);
        InetAddr iaIP(ui32RandPrefix);
        auto bitRepresentation = get_bits (htonl (ui32RandPrefix));
        std::stringstream strBitRepr;
        std::copy (bitRepresentation.begin(), bitRepresentation.end(), std::ostream_iterator<int>(strBitRepr, ""));

        std::string str, strRef;
        bool bMatched = false;
        trieTest.addVal (strBitRepr.str().c_str(), bitmaskBits, InetAddr(ui32RandIP).getIPAsString());
        for (int i = 0; i <= strBitRepr.str().length(); ++i) {
            bool bMatch = trieTest.longestPrefixSearch (strBitRepr.str().c_str(), i, str);
            if (bMatch && (i < bitmaskBits)) {
                std::cerr << "Longest prefix match test FAILED! i=" << i << " bitmask bits= " << bitmaskBits <<
                    " input=\"" << strBitRepr.str() << "\" output=\"" << str << "\"" << std::endl;
                exit (EXIT_FAILURE);
            }
            else if (bMatch && (str != InetAddr(ui32RandIP).getIPAsString())) {
                std::cerr << "Longest prefix match test FAILED! Output mismatch! i=" << i << " bitmask bits= " <<
                    bitmaskBits << " input=\"" << strBitRepr.str() << "\" output=\"" << str <<
                    "\" inserted value=\"" << InetAddr(ui32RandIP).getIPAsString() << "\"" << std::endl;
                exit (EXIT_FAILURE);
            }
            else if (bMatch && bMatched && (str != strRef)) {
                std::cerr << "Longest prefix match test FAILED! Output mismatch with previous iteration! i=" << i <<
                    " bitmask bits= " << bitmaskBits << " input=\"" << strBitRepr.str() << "\" output=\"" << str <<
                    "\" previous iteration output =\"" << strRef << "\"" << std::endl;
                exit (EXIT_FAILURE);
            }
            else if (bMatch) {
                // Correct match
                strRef = str;
                bMatched = true;
            }
        }
    }
}

int main (int argc, char *argv[])
{
    static constexpr unsigned int COMPLETE_LOOKUP_RUNS = 10000;
    static constexpr unsigned int LONGEST_PREFIX_MATCH_RUNS = 10000;

    std::random_device rd;                                                  // Random number from hardware
    std::mt19937 rndGen(rd());                                              // Seed the generator
    std::uniform_int_distribution<unsigned int> unifIntDistrIPRng(1UL, ULONG_MAX);        // Random within range [0.0.0.1-255.255.255.255]
    std::cout << "Running " << COMPLETE_LOOKUP_RUNS <<
        " iterations of the complete string lookup test with random input..." << std::endl;
    Trie<char, std::string> trieTest;
    for (int run = 0; run < COMPLETE_LOOKUP_RUNS; run++) {
        uint32 ui32RandPrefix = unifIntDistrIPRng (rndGen), ui32RandIP = unifIntDistrIPRng (rndGen);
        do_complete_string_lookup_test (ui32RandPrefix, ui32RandIP, trieTest);
    }
    std::cout << "Complete string lookup test PASSED " << COMPLETE_LOOKUP_RUNS << " iterations!" << std::endl;

    const int numthreads = std::thread::hardware_concurrency() > 0 ?
        std::thread::hardware_concurrency() * 2 : 1;
    std::vector<std::thread> threads;
    std::cout << "Running " << (LONGEST_PREFIX_MATCH_RUNS / numthreads) * numthreads << " iterations in " <<
        numthreads << " threads of the longest prefix match test with random input..." << std::endl;
    for (int t = 0; t < numthreads; ++t) {
        threads.emplace_back (do_longest_prefix_search_test, LONGEST_PREFIX_MATCH_RUNS / numthreads);
    }
    for(auto & t : threads) {
        t.join();
    }
    std::cout << "Longest prefix match test PASSED " << (LONGEST_PREFIX_MATCH_RUNS / numthreads) * numthreads <<
        " iterations!" << std::endl;

    return 0;
}