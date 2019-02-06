/*
uint128.h
An unsigned 128 bit integer type for C++

Copyright (c) 2013 - 2017 Jason Lee @ calccrypto at gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

With much help from Auston Sterling

Thanks to Stefan Deigmüller for finding
a bug in operator*.

Thanks to François Dessenne for convincing me
to do a general rewrite of this class.
*/

#ifndef __UINT128__
#define __UINT128__

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>

// Visual Studio
#ifdef _MSC_VER
    #include <string>
#endif

#include "FTypes.h"

namespace NOMADSUtil
{

class uint128
{
    private:
        uint64 UPPER, LOWER;

    public:
        // Constructors
        explicit uint128();
        explicit uint128(const int8 & rhs) :
            UPPER(0), LOWER(rhs) {}
        explicit uint128(const uint8 & rhs) :
            UPPER(0), LOWER(rhs) {}
        explicit uint128(const int16 & rhs) :
            UPPER(0), LOWER(rhs) {}
        explicit uint128(const uint16 & rhs) :
            UPPER(0), LOWER(rhs) {}
        explicit uint128(const int32 & rhs) :
            UPPER(0), LOWER(rhs) {}
        explicit uint128(const uint32 & rhs) :
            UPPER(0), LOWER(rhs) {}
        explicit uint128(const int64 & rhs) :
            UPPER(0), LOWER(rhs) {}
        explicit uint128(const uint64 & rhs) :
            UPPER(0), LOWER(rhs) {}
        uint128(const uint128 & rhs);
        uint128(uint128 && rhs);
        explicit uint128(uint64 upper_rhs, uint64 lower_rhs) :
            UPPER(upper_rhs), LOWER(lower_rhs) { }
        explicit uint128(uint32 upper_rhs, uint32 lower_rhs) :
            UPPER(upper_rhs), LOWER(lower_rhs) { }
        explicit uint128(uint16 upper_rhs, uint16 lower_rhs) :
            UPPER(upper_rhs), LOWER(lower_rhs) { }
        explicit uint128(uint8 upper_rhs, uint8 lower_rhs) :
            UPPER(upper_rhs), LOWER(lower_rhs) { }

        /*
        template <typename T> uint128(const T & rhs)
            : UPPER(0), LOWER(rhs)
        {
            static_assert(std::is_integral <T>::value, "Input argument type must be an integer.");
        }

        template <typename S, typename T> uint128(const S & upper_rhs, const T & lower_rhs)
            : UPPER(upper_rhs), LOWER(lower_rhs)
        {
            static_assert(std::is_integral <S>::value &&
                          std::is_integral <T>::value
                          , "Input argument types must be integers.");
        }
        */
        //  RHS input args only

        // Assignment Operator
        uint128 & operator=(const uint128 & rhs);
        uint128 & operator=(uint128 && rhs);

        template <typename T> uint128 & operator=(const T & rhs){
            static_assert(std::is_integral <T>::value, "Input argument type must be an integer.");
            UPPER = 0;
            LOWER = rhs;
            return *this;
        }

        // Typecast Operators
        operator bool() const;
        operator uint8() const;
        operator uint16() const;
        operator uint32() const;
        operator uint64() const;

        // Bitwise Operators
        uint128 operator&(const uint128 & rhs) const;

        template <typename T> uint128 operator&(const T & rhs) const{
            return uint128(0, LOWER & static_cast<uint64> (rhs));
        }

        uint128 & operator&=(const uint128 & rhs);

        template <typename T> uint128 & operator&=(const T & rhs){
            UPPER = 0;
            LOWER &= rhs;
            return *this;
        }

        uint128 operator|(const uint128 & rhs) const;

        template <typename T> uint128 operator|(const T & rhs) const{
            return uint128(UPPER, LOWER | static_cast<uint64> (rhs));
        }

        uint128 & operator|=(const uint128 & rhs);

        template <typename T> uint128 & operator|=(const T & rhs){
            LOWER |= static_cast<uint64> (rhs);
            return *this;
        }

        uint128 operator^(const uint128 & rhs) const;

        template <typename T> uint128 operator^(const T & rhs) const{
            return uint128(UPPER, LOWER ^ static_cast<uint64> (rhs));
        }

        uint128 & operator^=(const uint128 & rhs);

        template <typename T> uint128 & operator^=(const T & rhs){
            LOWER ^= static_cast<uint64> (rhs);
            return *this;
        }

        uint128 operator~() const;

        // Bit Shift Operators
        uint128 operator<<(const uint128 & rhs) const;

        template <typename T> uint128 operator<<(const T & rhs) const{
            return *this << uint128(rhs);
        }

        uint128 & operator<<=(const uint128 & rhs);

        template <typename T> uint128 & operator<<=(const T & rhs){
            *this = *this << uint128(rhs);
            return *this;
        }

        uint128 operator>>(const uint128 & rhs) const;

        template <typename T> uint128 operator>>(const T & rhs) const{
            return *this >> uint128(rhs);
        }

        uint128 & operator>>=(const uint128 & rhs);

        template <typename T> uint128 & operator>>=(const T & rhs){
            *this = *this >> uint128(rhs);
            return *this;
        }

        // Logical Operators
        bool operator!() const;
        bool operator&&(const uint128 & rhs) const;
        bool operator||(const uint128 & rhs) const;

        template <typename T> bool operator&&(const T & rhs){
            return static_cast <bool> (*this && rhs);
        }

        template <typename T> bool operator||(const T & rhs){
            return static_cast <bool> (*this || rhs);
        }

        // Comparison Operators
        bool operator==(const uint128 & rhs) const;

        template <typename T> bool operator==(const T & rhs) const{
            return (!UPPER && (LOWER == static_cast<uint64> (rhs)));
        }

        bool operator!=(const uint128 & rhs) const;

        template <typename T> bool operator!=(const T & rhs) const{
            return (UPPER | (LOWER != static_cast<uint64> (rhs)));
        }

        bool operator>(const uint128 & rhs) const;

        template <typename T> bool operator>(const T & rhs) const{
            return (UPPER || (LOWER > static_cast<uint64> (rhs)));
        }

        bool operator<(const uint128 & rhs) const;

        template <typename T> bool operator<(const T & rhs) const{
            return (!UPPER)?(LOWER < static_cast<uint64> (rhs)):false;
        }

        bool operator>=(const uint128 & rhs) const;

        template <typename T> bool operator>=(const T & rhs) const{
            return ((*this > rhs) | (*this == rhs));
        }

        bool operator<=(const uint128 & rhs) const;

        template <typename T> bool operator<=(const T & rhs) const{
            return ((*this < rhs) | (*this == rhs));
        }

        // Arithmetic Operators
        uint128 operator+(const uint128 & rhs) const;

        template <typename T> uint128 operator+(const T & rhs) const{
            return uint128(UPPER + ((LOWER + static_cast<uint64> (rhs)) < LOWER), LOWER + static_cast<uint64> (rhs));
        }

        uint128 & operator+=(const uint128 & rhs);

        template <typename T> uint128 & operator+=(const T & rhs){
            UPPER = UPPER + ((LOWER + rhs) < LOWER);
            LOWER = LOWER + rhs;
            return *this;
        }

        uint128 operator-(const uint128 & rhs) const;

        template <typename T> uint128 operator-(const T & rhs) const{
            return uint128(static_cast<uint64> (UPPER - ((LOWER - rhs) > LOWER)), static_cast<uint64> (LOWER - rhs));
        }

        uint128 & operator-=(const uint128 & rhs);

        template <typename T> uint128 & operator-=(const T & rhs){
            *this = *this - rhs;
            return *this;
        }

        uint128 operator*(const uint128 & rhs) const;

        template <typename T> uint128 operator*(const T & rhs) const{
            return *this * uint128(rhs);
        }

        uint128 & operator*=(const uint128 & rhs);

        template <typename T> uint128 & operator*=(const T & rhs){
            *this = *this * uint128(rhs);
            return *this;
        }

    private:
        std::pair <uint128, uint128> divmod(const uint128 & lhs, const uint128 & rhs) const;

    public:
        uint128 operator/(const uint128 & rhs) const;

        template <typename T> uint128 operator/(const T & rhs) const{
            return *this / uint128(rhs);
        }

        uint128 & operator/=(const uint128 & rhs);

        template <typename T> uint128 & operator/=(const T & rhs){
            *this = *this / uint128(rhs);
            return *this;
        }

        uint128 operator%(const uint128 & rhs) const;

        template <typename T> uint128 operator%(const T & rhs) const{
            return *this % uint128(rhs);
        }

        uint128 & operator%=(const uint128 & rhs);

        template <typename T> uint128 & operator%=(const T & rhs){
            *this = *this % uint128(rhs);
            return *this;
        }

        // Increment Operator
        uint128 & operator++();
        uint128 operator++(int);

        // Decrement Operator
        uint128 & operator--();
        uint128 operator--(int);

        // Nothing done since promotion doesn't work here
        uint128 operator+() const;

        // two's complement
        uint128 operator-() const;

        // Get private values
        const uint64 & upper() const;
        const uint64 & lower() const;

        // Get bitsize of value
        uint8 bits() const;

        // Get string representation of value
        std::string str(uint8 base = 10, const unsigned int & len = 0) const;
};


// useful values
extern const uint128 uint128_0;
extern const uint128 uint128_1;

// lhs type T as first arguemnt
// If the output is not a bool, casts to type T

// Bitwise Operators
template <typename T> uint128 operator&(const T & lhs, const uint128 & rhs){
    return rhs & lhs;
}

template <typename T> T & operator&=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (rhs & lhs);
}

template <typename T> uint128 operator|(const T & lhs, const uint128 & rhs){
    return rhs | lhs;
}

template <typename T> T & operator|=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (rhs | lhs);
}

template <typename T> uint128 operator^(const T & lhs, const uint128 & rhs){
    return rhs ^ lhs;
}

template <typename T> T & operator^=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (rhs ^ lhs);
}

// Bitshift operators
uint128 operator<<(const bool   & lhs, const uint128 & rhs);
uint128 operator<<(const uint8  & lhs, const uint128 & rhs);
uint128 operator<<(const uint16 & lhs, const uint128 & rhs);
uint128 operator<<(const uint32 & lhs, const uint128 & rhs);
uint128 operator<<(const uint64 & lhs, const uint128 & rhs);
uint128 operator<<(const int8   & lhs, const uint128 & rhs);
uint128 operator<<(const int16  & lhs, const uint128 & rhs);
uint128 operator<<(const int32  & lhs, const uint128 & rhs);
uint128 operator<<(const int64  & lhs, const uint128 & rhs);

template <typename T> T & operator<<=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (uint128(lhs) << rhs);
}

uint128 operator>>(const bool   & lhs, const uint128 & rhs);
uint128 operator>>(const uint8  & lhs, const uint128 & rhs);
uint128 operator>>(const uint16 & lhs, const uint128 & rhs);
uint128 operator>>(const uint32 & lhs, const uint128 & rhs);
uint128 operator>>(const uint64 & lhs, const uint128 & rhs);
uint128 operator>>(const int8   & lhs, const uint128 & rhs);
uint128 operator>>(const int16  & lhs, const uint128 & rhs);
uint128 operator>>(const int32  & lhs, const uint128 & rhs);
uint128 operator>>(const int64  & lhs, const uint128 & rhs);

template <typename T> T & operator>>=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (uint128(lhs) >> rhs);
}

// Comparison Operators
template <typename T> bool operator==(const T & lhs, const uint128 & rhs){
    return (!rhs.upper() && (static_cast<uint64> (lhs) == rhs.lower()));
}

template <typename T> bool operator!=(const T & lhs, const uint128 & rhs) {
    return (rhs.upper() | (static_cast<uint64> (lhs) != rhs.lower()));
}

template <typename T> bool operator>(const T & lhs, const uint128 & rhs){
    return (!rhs.upper()) && (static_cast<uint64> (lhs) > rhs.lower());
}

template <typename T> bool operator<(const T & lhs, const uint128 & rhs){
    if (rhs.upper()){
        return true;
    }
    return (static_cast<uint64> (lhs) < rhs.lower());
}

template <typename T> bool operator>=(const T & lhs, const uint128 & rhs){
    if (rhs.upper()){
        return false;
    }
    return (static_cast<uint64> (lhs) >= rhs.lower());
}

template <typename T> bool operator<=(const T & lhs, const uint128 & rhs){
    if (rhs.upper()){
        return true;
    }
    return (static_cast<uint64> (lhs) <= rhs.lower());
}

// Arithmetic Operators
template <typename T> uint128 operator+(const T & lhs, const uint128 & rhs){
    return rhs + lhs;
}

template <typename T> T & operator+=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (rhs + lhs);
}

template <typename T> uint128 operator-(const T & lhs, const uint128 & rhs){
    return -(rhs - lhs);
}

template <typename T> T & operator-=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (-(rhs - lhs));
}

template <typename T> uint128 operator*(const T & lhs, const uint128 & rhs){
    return rhs * lhs;
}

template <typename T> T & operator*=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (rhs * lhs);
}

template <typename T> uint128 operator/(const T & lhs, const uint128 & rhs){
    return uint128(lhs) / rhs;
}

template <typename T> T & operator/=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (uint128(lhs) / rhs);
}

template <typename T> uint128 operator%(const T & lhs, const uint128 & rhs){
    return uint128(lhs) % rhs;
}

template <typename T> T & operator%=(T & lhs, const uint128 & rhs){
    return lhs = static_cast <T> (uint128(lhs) % rhs);
}

// IO Operator
std::ostream & operator<<(std::ostream & stream, const uint128 & rhs);

}
#endif
