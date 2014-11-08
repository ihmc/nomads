#ifndef INCL_ORDERABLE_BUFFER_WRITER_H
#define INCL_ORDERABLE_BUFFER_WRITER_H

/*
 * OrderableBufferWrapper.h
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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
 * The OrderableItem class provides an interface for any item that
 * can be characterized by a sequence number and an item length.
 * The instances of any class that implements this interface can
 * thus be organized according to their sequence number and length.
 *
 * The OrderableBufferWrapper class implements the OrderableItem
 * interface and extends it by adding the possibility to store 
 * a pointer to an object of a type defined at compile-time.
 */

#include "SequentialArithmetic.h"


namespace ACMNetProxy
{
    class OrderableItem
    {
    public:
        OrderableItem (unsigned int uiSequenceNumber, unsigned int uiItemLength);
        explicit OrderableItem (const OrderableItem& rOrderableItem);
        virtual ~OrderableItem (void) {};

        virtual const unsigned int getSequenceNumber (void) const;
        virtual const unsigned int getItemLength (void) const;
        virtual const unsigned int getFollowingSequenceNumber (void) const;
        virtual void setSequenceNumber (unsigned int uiSequenceNumber);
        virtual void setItemLength (unsigned int uhiItemLength);

        virtual bool operator== (const OrderableItem &rhs) const = 0;
        virtual bool operator!= (const OrderableItem &rhs) const = 0;
        virtual bool operator< (const OrderableItem &rhs) const = 0;
        virtual bool operator> (const OrderableItem &rhs) const = 0;
        virtual bool operator<= (const OrderableItem &rhs) const = 0;
        virtual bool operator>= (const OrderableItem &rhs) const = 0;

        virtual bool follows (const OrderableItem &rhs) const = 0;
        virtual bool isFollowedBy (const OrderableItem &rhs) const = 0;
        virtual bool overlaps (const OrderableItem &rhs) const = 0;
        virtual bool includes (const OrderableItem &rhs) const = 0;
        virtual bool isIncludedIn (const OrderableItem &rhs) const = 0;


    protected:
        unsigned int _uiSequenceNumber;
        unsigned int _uiItemLength;
    };


    template <class DataType> class OrderableBufferWrapper : public OrderableItem
    {
    public:
        OrderableBufferWrapper (unsigned int uiSequenceNumber, unsigned int uiItemLength, const DataType *pData);
        explicit OrderableBufferWrapper (const OrderableBufferWrapper& rOrderableBufferWrapper);
        virtual ~OrderableBufferWrapper (void) {};

        using OrderableItem::getSequenceNumber;
        using OrderableItem::getItemLength;
        using OrderableItem::getFollowingSequenceNumber;

        virtual int incrementValues (unsigned int uiValue);
        virtual const DataType * const getData (void) const;
        virtual void setData (DataType * const pData);

        virtual bool operator== (const OrderableItem &rhs) const;
        virtual bool operator!= (const OrderableItem &rhs) const;
        virtual bool operator< (const OrderableItem &rhs) const;
        virtual bool operator> (const OrderableItem &rhs) const;
        virtual bool operator<= (const OrderableItem &rhs) const;
        virtual bool operator>= (const OrderableItem &rhs) const;
        
        virtual bool follows (const OrderableItem &rhs) const;
        virtual bool isFollowedBy (const OrderableItem &rhs) const;
        virtual bool overlaps (const OrderableItem &rhs) const;
        virtual bool includes (const OrderableItem &rhs) const;
        virtual bool isIncludedIn (const OrderableItem &rhs) const;

    protected:
        DataType *_pData;
    };


    inline OrderableItem::OrderableItem (unsigned int uiSequenceNumber, unsigned int uiItemLength)
    {
        _uiSequenceNumber = uiSequenceNumber;
        _uiItemLength = uiItemLength;
    }

    inline const unsigned int OrderableItem::getSequenceNumber (void) const
    {
        return _uiSequenceNumber;
    }

    inline const unsigned int OrderableItem::getItemLength (void) const
    {
        return _uiItemLength;
    }

    inline const unsigned int OrderableItem::getFollowingSequenceNumber (void) const
    {
        return _uiSequenceNumber + _uiItemLength;
    }

    inline void OrderableItem::setSequenceNumber (unsigned int uiSequenceNumber)
    {
        _uiSequenceNumber = uiSequenceNumber;
    }

    inline void OrderableItem::setItemLength (unsigned int uiItemLength)
    {
        _uiItemLength = uiItemLength;
    }

    template <class DataType> inline OrderableBufferWrapper<DataType>::OrderableBufferWrapper (unsigned int uiSequenceNumber, unsigned int uiItemLength, const DataType *pData)
        : OrderableItem (uiSequenceNumber, uiItemLength)
    {
        _pData = const_cast<DataType*> (pData);
    }

    template <class DataType> inline const DataType * const OrderableBufferWrapper<DataType>::getData (void) const
    {
        return _pData;
    }

    template <class DataType> inline void OrderableBufferWrapper<DataType>::setData (DataType * const pData)
    {
        _pData = pData;
    }

    template <class DataType> inline int OrderableBufferWrapper<DataType>::incrementValues (unsigned int uiValue)
    {
        if (uiValue == 0) {
            return 0;
        }
        else if (uiValue > _uiItemLength) {
            uiValue = _uiItemLength;
        }

        _uiSequenceNumber += uiValue;
        _uiItemLength -= uiValue;
        _pData = &_pData[uiValue];

        return uiValue;
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::operator== (const OrderableItem &rhs) const
    {
        return ((getSequenceNumber() == rhs.getSequenceNumber()) && (getItemLength() == rhs.getItemLength()));
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::operator!= (const OrderableItem &rhs) const
    {
        return !(*this == rhs);
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::operator< (const OrderableItem &rhs) const
    {
        return (NOMADSUtil::SequentialArithmetic::lessThan (getSequenceNumber(), rhs.getSequenceNumber()) &&
                NOMADSUtil::SequentialArithmetic::lessThanOrEqual ((getSequenceNumber() + getItemLength()), rhs.getSequenceNumber()));
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::operator> (const OrderableItem &rhs) const
    {
        return  rhs < *this;
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::operator<= (const OrderableItem &rhs) const
    {
        return NOMADSUtil::SequentialArithmetic::lessThanOrEqual (getSequenceNumber(), rhs.getSequenceNumber());
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::operator>= (const OrderableItem &rhs) const
    {
        return rhs <= *this;
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::follows (const OrderableItem &rhs) const
    {
        return rhs.getFollowingSequenceNumber() == _uiSequenceNumber;
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::isFollowedBy (const OrderableItem &rhs) const
    {
        return getFollowingSequenceNumber() == rhs.getSequenceNumber();
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::overlaps (const OrderableItem &rhs) const
    {
        return ((NOMADSUtil::SequentialArithmetic::lessThanOrEqual (getSequenceNumber(), rhs.getSequenceNumber()) &&
                NOMADSUtil::SequentialArithmetic::greaterThan (getFollowingSequenceNumber(), rhs.getSequenceNumber())) ||
                (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (rhs.getSequenceNumber(), getSequenceNumber()) &&
                NOMADSUtil::SequentialArithmetic::greaterThan (rhs.getFollowingSequenceNumber(), getSequenceNumber())));
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::includes (const OrderableItem &rhs) const
    {
        return (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (getSequenceNumber(), rhs.getSequenceNumber()) &&
                NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (getFollowingSequenceNumber(), rhs.getFollowingSequenceNumber()));
    }

    template <class DataType> inline bool OrderableBufferWrapper<DataType>::isIncludedIn (const OrderableItem &rhs) const
    {
        return rhs.includes (*this);
    }

}

#endif  // INCL_ORDERABLE_BUFFER_WRITER_H
