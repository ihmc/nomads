#include "uint128.h"

namespace NOMADSUtil
{
    const uint128 uint128_0(0U);
    const uint128 uint128_1(1U);

    uint128::uint128()
        : UPPER(0), LOWER(0)
    {}

    uint128::uint128(const uint128 & rhs)
        : UPPER(rhs.UPPER), LOWER(rhs.LOWER)
    {}

    uint128::uint128(uint128 && rhs)
        : UPPER(std::move(rhs.UPPER)), LOWER(std::move(rhs.LOWER))
    {
        if (this != &rhs){
            rhs.UPPER = 0;
            rhs.LOWER = 0;
        }
    }

    uint128 & uint128::operator=(const uint128 & rhs){
        UPPER = rhs.UPPER;
        LOWER = rhs.LOWER;
        return *this;
    }

    uint128 & uint128::operator=(uint128 && rhs){
        if (this != &rhs){
            UPPER = std::move(rhs.UPPER);
            LOWER = std::move(rhs.LOWER);
            rhs.UPPER = 0;
            rhs.LOWER = 0;
        }
        return *this;
    }

    uint128::operator bool() const{
        return (bool) (UPPER | LOWER);
    }

    uint128::operator uint8() const{
        return static_cast<uint8> (LOWER);
    }

    uint128::operator uint16() const{
        return static_cast<uint16> (LOWER);
    }

    uint128::operator uint32() const{
        return static_cast<uint32> (LOWER);
    }

    uint128::operator uint64() const{
        return static_cast<uint64> (LOWER);
    }

    uint128 uint128::operator&(const uint128 & rhs) const{
        return uint128{UPPER & rhs.UPPER, LOWER & rhs.LOWER};
    }

    uint128 & uint128::operator&=(const uint128 & rhs){
        UPPER &= rhs.UPPER;
        LOWER &= rhs.LOWER;
        return *this;
    }

    uint128 uint128::operator|(const uint128 & rhs) const{
        return uint128(UPPER | rhs.UPPER, LOWER | rhs.LOWER);
    }

    uint128 & uint128::operator|=(const uint128 & rhs){
        UPPER |= rhs.UPPER;
        LOWER |= rhs.LOWER;
        return *this;
    }

    uint128 uint128::operator^(const uint128 & rhs) const{
        return uint128(UPPER ^ rhs.UPPER, LOWER ^ rhs.LOWER);
    }

    uint128 & uint128::operator^=(const uint128 & rhs){
        UPPER ^= rhs.UPPER;
        LOWER ^= rhs.LOWER;
        return *this;
    }

    uint128 uint128::operator~() const{
        return uint128(~UPPER, ~LOWER);
    }

    uint128 uint128::operator<<(const uint128 & rhs) const{
        const uint64 shift = rhs.LOWER;
        if (((bool) rhs.UPPER) || (shift >= 128)){
            return uint128_0;
        }
        else if (shift == 64){
            return uint128(LOWER, 0);
        }
        else if (shift == 0){
            return *this;
        }
        else if (shift < 64){
            return uint128((UPPER << shift) + (LOWER >> (64 - shift)), LOWER << shift);
        }
        else if ((128 > shift) && (shift > 64)){
            return uint128(LOWER << (shift - 64), 0);
        }
        else{
            return uint128_0;
        }
    }

    uint128 & uint128::operator<<=(const uint128 & rhs){
        *this = *this << rhs;
        return *this;
    }

    uint128 uint128::operator>>(const uint128 & rhs) const{
        const uint64 shift = rhs.LOWER;
        if (((bool) rhs.UPPER) || (shift >= 128)){
            return uint128_0;
        }
        else if (shift == 64){
            return uint128(0, UPPER);
        }
        else if (shift == 0){
            return *this;
        }
        else if (shift < 64){
            return uint128(UPPER >> shift, (UPPER << (64 - shift)) + (LOWER >> shift));
        }
        else if ((128 > shift) && (shift > 64)){
            return uint128(0, (UPPER >> (shift - 64)));
        }
        else{
            return uint128_0;
        }
    }

    uint128 & uint128::operator>>=(const uint128 & rhs){
        *this = *this >> rhs;
        return *this;
    }

    bool uint128::operator!() const{
        return !(bool) (UPPER | LOWER);
    }

    bool uint128::operator&&(const uint128 & rhs) const{
        return ((bool) *this && rhs);
    }

    bool uint128::operator||(const uint128 & rhs) const{
         return ((bool) *this || rhs);
    }

    bool uint128::operator==(const uint128 & rhs) const{
        return ((UPPER == rhs.UPPER) && (LOWER == rhs.LOWER));
    }

    bool uint128::operator!=(const uint128 & rhs) const{
        return ((UPPER != rhs.UPPER) | (LOWER != rhs.LOWER));
    }

    bool uint128::operator>(const uint128 & rhs) const{
        if (UPPER == rhs.UPPER){
            return (LOWER > rhs.LOWER);
        }
        return (UPPER > rhs.UPPER);
    }

    bool uint128::operator<(const uint128 & rhs) const{
        if (UPPER == rhs.UPPER){
            return (LOWER < rhs.LOWER);
        }
        return (UPPER < rhs.UPPER);
    }

    bool uint128::operator>=(const uint128 & rhs) const{
        return ((*this > rhs) | (*this == rhs));
    }

    bool uint128::operator<=(const uint128 & rhs) const{
        return ((*this < rhs) | (*this == rhs));
    }

    uint128 uint128::operator+(const uint128 & rhs) const{
        return uint128(UPPER + rhs.UPPER + ((LOWER + rhs.LOWER) < LOWER), LOWER + rhs.LOWER);
    }

    uint128 & uint128::operator+=(const uint128 & rhs){
        UPPER += rhs.UPPER + ((LOWER + rhs.LOWER) < LOWER);
        LOWER += rhs.LOWER;
        return *this;
    }

    uint128 uint128::operator-(const uint128 & rhs) const{
        return uint128(UPPER - rhs.UPPER - ((LOWER - rhs.LOWER) > LOWER), LOWER - rhs.LOWER);
    }

    uint128 & uint128::operator-=(const uint128 & rhs){
        *this = *this - rhs;
        return *this;
    }

    uint128 uint128::operator*(const uint128 & rhs) const{
        // split values into 4 32-bit parts
        uint64 top[4] = {UPPER >> 32, UPPER & 0xffffffff, LOWER >> 32, LOWER & 0xffffffff};
        uint64 bottom[4] = {rhs.UPPER >> 32, rhs.UPPER & 0xffffffff, rhs.LOWER >> 32, rhs.LOWER & 0xffffffff};
        uint64 products[4][4];

        // multiply each component of the values
        for(int y = 3; y > -1; y--){
            for(int x = 3; x > -1; x--){
                products[3 - x][y] = top[x] * bottom[y];
            }
        }

        // first row
        uint64 fourth32 = (products[0][3] & 0xffffffff);
        uint64 third32  = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
        uint64 second32 = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
        uint64 first32  = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);

        // second row
        third32  += (products[1][3] & 0xffffffff);
        second32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
        first32  += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);

        // third row
        second32 += (products[2][3] & 0xffffffff);
        first32  += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);

        // fourth row
        first32  += (products[3][3] & 0xffffffff);

        // move carry to next digit
        third32  += fourth32 >> 32;
        second32 += third32  >> 32;
        first32  += second32 >> 32;

        // remove carry from current digit
        fourth32 &= 0xffffffff;
        third32  &= 0xffffffff;
        second32 &= 0xffffffff;
        first32  &= 0xffffffff;

        // combine components
        return uint128((first32 << 32) | second32, (third32 << 32) | fourth32);
    }

    uint128 & uint128::operator*=(const uint128 & rhs){
        *this = *this * rhs;
        return *this;
    }

    std::pair <uint128, uint128> uint128::divmod(const uint128 & lhs, const uint128 & rhs) const{
        // Save some calculations /////////////////////
        if (rhs == uint128_0){
            throw std::domain_error("Error: division or modulus by 0");
        }
        else if (rhs == uint128_1){
            return std::pair <uint128, uint128> (lhs, uint128_0);
        }
        else if (lhs == rhs){
            return std::pair <uint128, uint128> (uint128_1, uint128_0);
        }
        else if ((lhs == uint128_0) || (lhs < rhs)){
            return std::pair <uint128, uint128> (uint128_0, lhs);
        }

        std::pair <uint128, uint128> qr (uint128_0, uint128_0);
        for(uint8 x = lhs.bits(); x > 0; x--){
            qr.first  <<= uint128_1;
            qr.second <<= uint128_1;

            if ((lhs >> (x - 1U)) & 1){
                qr.second++;
            }

            if (qr.second >= rhs){
                qr.second -= rhs;
                qr.first++;
            }
        }
        return qr;
    }

    uint128 uint128::operator/(const uint128 & rhs) const{
        return divmod(*this, rhs).first;
    }

    uint128 & uint128::operator/=(const uint128 & rhs){
        *this = *this / rhs;
        return *this;
    }

    uint128 uint128::operator%(const uint128 & rhs) const{
        return divmod(*this, rhs).second;
    }

    uint128 & uint128::operator%=(const uint128 & rhs){
        *this = *this % rhs;
        return *this;
    }

    uint128 & uint128::operator++(){
        return *this += uint128_1;
    }

    uint128 uint128::operator++(int){
        uint128 temp(*this);
        ++*this;
        return temp;
    }

    uint128 & uint128::operator--(){
        return *this -= uint128_1;
    }

    uint128 uint128::operator--(int){
        uint128 temp(*this);
        --*this;
        return temp;
    }

    uint128 uint128::operator+() const{
        return *this;
    }

    uint128 uint128::operator-() const{
        return ~*this + uint128_1;
    }

    const uint64 & uint128::upper() const{
        return UPPER;
    }

    const uint64 & uint128::lower() const{
        return LOWER;
    }

    uint8 uint128::bits() const{
        uint8 out = 0;
        if (UPPER){
            out = 64;
            uint64 up = UPPER;
            while (up){
                up >>= 1;
                out++;
            }
        }
        else{
            uint64 low = LOWER;
            while (low){
                low >>= 1;
                out++;
            }
        }
        return out;
    }

    std::string uint128::str(uint8 base, const unsigned int & len) const{
        if ((base < 2) || (base > 16)){
            throw std::invalid_argument("Base must be in the range [2, 16]");
        }
        std::string out = "";
        if (!(*this)){
            out = "0";
        }
        else{
            std::pair <uint128, uint128> qr(*this, uint128_0);
            do{
                qr = divmod(qr.first, uint128{base});
                out = "0123456789abcdef"[(uint8) qr.second] + out;
            } while (qr.first);
        }
        if (out.size() < len){
            out = std::string(len - out.size(), '0') + out;
        }
        return out;
    }

    uint128 operator<<(const bool & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator<<(const uint8 & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator<<(const uint16 & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator<<(const uint32 & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator<<(const uint64 & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator<<(const int8 & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator<<(const int16 & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator<<(const int32 & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator<<(const int64 & lhs, const uint128 & rhs){
        return uint128(lhs) << rhs;
    }

    uint128 operator>>(const bool & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    uint128 operator>>(const uint8 & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    uint128 operator>>(const uint16 & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    uint128 operator>>(const uint32 & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    uint128 operator>>(const uint64 & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    uint128 operator>>(const int8 & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    uint128 operator>>(const int16 & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    uint128 operator>>(const int32 & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    uint128 operator>>(const int64 & lhs, const uint128 & rhs){
        return uint128(lhs) >> rhs;
    }

    std::ostream & operator<<(std::ostream & stream, const uint128 & rhs){
        if (stream.flags() & stream.oct){
            stream << rhs.str(8);
        }
        else if (stream.flags() & stream.dec){
            stream << rhs.str(10);
        }
        else if (stream.flags() & stream.hex){
            stream << rhs.str(16);
        }
        return stream;
    }

}