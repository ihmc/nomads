#ifndef TUPLES_HASH_FUNCTION_H
#define TUPLES_HASH_FUNCTION_H
/*
*
* TuplesHashFunction.h
*
* This file is part of the IHMC NetProxy Library / Component
* Copyright(c) 2010 - 2018 IHMC.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 3 (GPLv3)as published by the Free Software Foundation.
*
* U.S.Government agencies and organizations may redistribute
* and/or modify this program under terms equivalent to
* "Government Purpose Rights" as defined by DFARS
* 252.227 - 7014(a)(12) (February 2014).
*
* Alternative licenses that allow for use within commercial products may be
* available.Contact Niranjan Suri at IHMC(nsuri@ihmc.us) for details.
*
* This file provides an implementation for hashing tuples, so that they can
* be used as keys in maps. The code is from the Boost library:
* Reciprocal of the golden ratio helps spread entropy and handles duplicates;
* See Mike Seymour in magic-numbers-in-boosthash-combine:
* https://stackoverflow.com/questions/4948780
*/

#include <tuple>
#include <utility>

#include "FTypes.h"



namespace std
{
    namespace
    {
        // Code from boost
        template <class T> inline void hash_combine (std::size_t & seed, const T & v)
        {
            seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        // Recursive template code derived from Matthieu M.
        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
            static void apply (size_t & seed, const Tuple & tuple)
            {
                HashValueImpl<Tuple, Index - 1>::apply (seed, tuple);
                hash_combine (seed, get<Index> (tuple));
            }
        };


        template <class Tuple>
        struct HashValueImpl<Tuple, 0>
        {
            static void apply (size_t& seed, const Tuple & tuple)
            {
                hash_combine (seed, get<0>(tuple));
            }
        };
    }


    template <typename ... TT>
    struct hash<std::tuple<TT...>>
    {
        size_t operator() (const std::tuple<TT...> & tt) const
        {
            size_t seed = 0;
            HashValueImpl<std::tuple<TT...>>::apply (seed, tt);

            return seed;
        }
    };


    template<> struct hash<std::pair<uint32, uint8>>
    {
        std::size_t operator() (const std::pair<uint32, uint8> & s) const noexcept
        {
            std::size_t seed{std::hash<uint32>{} (s.first)};
            hash_combine (seed, s.second);

            return seed;
        }
    };

    template<> struct hash<std::pair<uint32, uint32>>
    {
        std::size_t operator() (const std::pair<uint32, uint32> & s) const noexcept
        {
            std::size_t seed{std::hash<uint32>{} (s.first)};
            hash_combine (seed, s.second);

            return seed;
        }
    };
}

#endif  // TUPLES_HASH_FUNCTION_H