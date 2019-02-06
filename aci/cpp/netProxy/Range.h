#ifndef INCL_RANGE_H
#define INCL_RANGE_H

/*
* Range.h
*
* This file is part of the IHMC NetProxy Library/Component
* Copyright (c) 2010-2018 IHMC.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 3 (GPLv3) as published by the Free Software Foundation.
*
* U.S. Government agencies and organizations may redistribute
* and/or modify this program under terms equivalent to
* "Government Purpose Rights" as defined by DFARS
* 252.227-7014(a)(12) (February 2014).
*
* Alternative licenses that allow for use within commercial products may be
* available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
*
* Class that provides different range implementations.
*/

#include <string>

#include "FTypes.h"

namespace ACMNetProxy
{
    class ByteRange;
    class ShortRange;


    class Range
    {
    public:
        virtual uint64 getLowestEnd (void) const = 0;
        virtual uint64 getHighestEnd (void) const = 0;
        virtual uint64 getRangeWidth (void) const;

        virtual bool contains (const Range * const r) const;
        virtual bool overlaps (const Range * const r) const;

        virtual bool operator< (const Range * const r) const;
        virtual bool operator> (const Range * const r) const;
        virtual bool operator== (const Range * const r) const;
        virtual bool operator!= (const Range * const r) const;

        virtual operator std::string (void) const;
    };


    template <typename UINT> class URange : public Range
    {
    public:
        using type = UINT;


        URange (void) :
            _uiLowEnd (0), _uiHighEnd (0) { }
        template <typename UINT_ARG> URange (UINT_ARG uiVal) :
            _uiLowEnd (uiVal), _uiHighEnd (uiVal) { }
        template <typename UINT_ARG> URange (UINT_ARG uiLowVal, UINT_ARG uiHighVal) :
            _uiLowEnd (uiLowVal), _uiHighEnd (uiHighVal) { }
        URange (const URange & br) = default;
        URange (URange && br) :
            _uiLowEnd (br._uiLowEnd), _uiHighEnd (br._uiHighEnd)
        {
            br._uiLowEnd = 0;
            br._uiHighEnd = 0;
        }

        URange & operator= (const URange & br) = default;
        URange & operator= (URange && br);

        uint64 getLowestEnd (void) const;
        uint64 getHighestEnd (void) const;


    private:
        template <typename T> friend URange<T> merge (const URange<T> & br1, const URange<T> & br2);


        UINT _uiLowEnd;
        UINT _uiHighEnd;
    };


    template <typename UINT> URange<UINT> merge (const URange<UINT> & ur1, const URange<UINT> & ur2)
    {
        URange<UINT> res{};
        res._uiLowEnd = (ur1._uiLowEnd <= ur2._uiLowEnd) ? ur1._uiLowEnd : ur2._uiLowEnd;
        res._uiHighEnd = (ur1._uiHighEnd >= ur2._uiHighEnd) ? ur1._uiHighEnd : ur2._uiHighEnd;

        return res;
    }

    inline uint64 Range::getRangeWidth (void) const
    {
        return (getHighestEnd() - getLowestEnd()) + 1;
    }

    inline bool Range::contains (const Range * const r) const
    {
        return (getLowestEnd() <= r->getLowestEnd()) && (getHighestEnd() >= r->getHighestEnd());
    }

    inline bool Range::overlaps (const Range * const r) const
    {
        return (getLowestEnd() <= r->getHighestEnd()) && (r->getLowestEnd() <= getHighestEnd());
    }

    inline bool Range::operator< (const Range * const r) const
    {
        return (getRangeWidth() < r->getRangeWidth()) ||
            ((getRangeWidth() == r->getRangeWidth()) && (getHighestEnd() < r->getLowestEnd()));
    }

    inline bool Range::operator> (const Range * const r) const
    {
        return (getRangeWidth() > r->getRangeWidth()) ||
            ((getRangeWidth() == r->getRangeWidth()) && (getLowestEnd() > r->getHighestEnd()));
    }

    inline bool Range::operator== (const Range * const r) const
    {
        return ((getLowestEnd() <= r->getLowestEnd()) && (getHighestEnd() >= r->getHighestEnd())) ||
            ((getLowestEnd() >= r->getLowestEnd()) && (getHighestEnd() <= r->getHighestEnd()));
    }

    inline bool Range::operator!= (const Range * const r) const
    {
        return !(*this == r);
    }

    inline Range::operator std::string (void) const
    {
        return std::to_string (getLowestEnd()) + '-' + std::to_string (getHighestEnd());
    }

    template <typename UINT> URange<UINT> & URange<UINT>::operator= (URange<UINT> && ur)
    {
        _uiLowEnd = ur._uiLowEnd;
        _uiHighEnd = ur._uiHighEnd;
        ur._uiLowEnd = 0;
        ur._uiHighEnd = 0;

        return *this;
    }

    template <typename UINT> uint64 URange<UINT>::getLowestEnd (void) const
    {
        return _uiLowEnd;
    }

    template <typename UINT> uint64 URange<UINT>::getHighestEnd (void) const
    {
        return _uiHighEnd;
    }

}
#endif  // INCL_RANGE_H