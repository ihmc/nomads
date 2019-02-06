#ifndef INCL_RANGE_TRIE_H
#define INCL_RANGE_TRIE_H

/*
* RangeTrie.h
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

#include <stdexcept>
#include <map>

#include "Range.h"
#include "NetworkAddressRange.h"


namespace ACMNetProxy
{
    template <int Depth, int Total_depth, typename VAL> class RangeTrieNode
    {
    public:
        RangeTrieNode (void);

        void insert (const NetworkAddressRange * const pSrcARD, const NetworkAddressRange * const pDstARD,
                     const VAL & val);
        bool matches (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD) const;
        const VAL & retrieve (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD);


    private:
        std::map<URange<uint8>, RangeTrieNode<Depth - 1, Total_depth, VAL>> _mChildren;
        std::pair<std::pair<const NetworkAddressRange *, const NetworkAddressRange *>, VAL> _pARDVal;
    };


    template <int Total_depth, typename VAL> class RangeTrieNode<5, Total_depth, VAL>
    {
    public:
        RangeTrieNode (void);

        void insert (const NetworkAddressRange * const pSrcARD, const NetworkAddressRange * const pDstARD,
                     const VAL & val);
        bool matches (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD) const;
        const VAL & retrieve (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD);


    private:
        std::map<URange<uint16>, RangeTrieNode<5 - 1, Total_depth, VAL>> _mChildren;
        std::pair<std::pair<const NetworkAddressRange *, const NetworkAddressRange *>, VAL> _pARDVal;
    };


    template <int Total_depth, typename VAL> class RangeTrieNode<0, Total_depth, VAL>
    {
        public:
            RangeTrieNode (void);

            void insert (const NetworkAddressRange * const pSrcARD, const NetworkAddressRange * const pDstARD,
                         const VAL & val);
            bool matches (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD) const;
            const VAL & retrieve (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD);

        private:
            std::map<URange<uint16>, std::pair<std::pair<const NetworkAddressRange *, const NetworkAddressRange *>, const VAL>> _mLeaves;
    };


    // Template implementation
    template <int Depth, int Total_depth, typename VAL>
    RangeTrieNode<Depth, Total_depth, VAL>::RangeTrieNode (void) :
        _pARDVal{nullptr, VAL{}}
    { }

    template <int Depth, int Total_depth, typename VAL>
    void RangeTrieNode<Depth, Total_depth, VAL>::insert (const NetworkAddressRange * const pSrcARD, const NetworkAddressRange * const pDstARD, const VAL & val)
    {
        if ((_mChildren.size() == 0) && (_pARDVal.first.first == nullptr)) {
            _pARDVal.first.first = pSrcARD;
            _pARDVal.first.second = pDstARD;
            _pARDVal.second = val;

            return;
        }

        if (Depth > 5) {
            // Check pSrcARD and _pARDVal.first.first
            if (_mChildren.size() == 0) {
                _mChildren[*(static_cast<const URange<uint8> *> ((*(_pARDVal.first.first))[Total_depth - Depth]))] = RangeTrieNode<Depth - 1, Total_depth, VAL> {};
                _mChildren[*(static_cast<const URange<uint8> *> ((*(_pARDVal.first.first))[Total_depth - Depth]))].insert (_pARDVal.first.first, _pARDVal.first.second,
                                                                                                                          _pARDVal.second);
                _pARDVal.first.first = nullptr;
                _pARDVal.first.second = nullptr;
                _pARDVal.second = VAL{};
            }

            _mChildren[*(static_cast<const URange<uint8> *> ((*pSrcARD)[Total_depth - Depth]))] = RangeTrieNode<Depth - 1, Total_depth, VAL> {};
            _mChildren[*(static_cast<const URange<uint8> *> ((*pSrcARD)[Total_depth - Depth]))].insert (pSrcARD, pDstARD, val);
        }
        else {
            // Check pDstARD and _pARDVal.first.second
            if (_mChildren.size() == 0) {
                _mChildren[*(static_cast<const URange<uint8> *> ((*(_pARDVal.first.second))[Total_depth - Depth]))] = RangeTrieNode<Depth - 1, Total_depth, VAL> {};
                _mChildren[*(static_cast<const URange<uint8> *> ((*(_pARDVal.first.second))[Total_depth - Depth]))].insert (_pARDVal.first.first, _pARDVal.first.second,
                                                                                                                           _pARDVal.second);
                _pARDVal.first.first = nullptr;
                _pARDVal.first.second = nullptr;
                _pARDVal.second = VAL{};
            }

            _mChildren[*(static_cast<const URange<uint8> *> ((*pDstARD)[Total_depth - Depth]))] = RangeTrieNode<Depth - 1, Total_depth, VAL> {};
            _mChildren[*(static_cast<const URange<uint8> *> ((*pDstARD)[Total_depth - Depth]))].insert (pSrcARD, pDstARD, val);
        }
    }

    template <int Depth, int Total_depth, typename VAL>
    bool RangeTrieNode<Depth, Total_depth, VAL>::matches (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD) const
    {
        if ((_mChildren.size == 0) && (_pARDVal.first.first == nullptr)) {
            return false;
        }
        if (_mChildren.size == 0) {
            return (depth > 5) ?
                (_pARDVal.first.first->contains (srcARD, Total_depth - Depth) && _pARDVal.first.second->contains (dstARD)) :
                _pARDVal.first.second->contains (dstARD, Total_depth - Depth);
        }

        const URange<uint8> * const ur = (Depth > 5) ?
            static_cast<const URange<uint8> *> (srcARD[Total_depth - Depth]) :
            static_cast<const URange<uint8> *> (dstARD[Total_depth - Depth]);
        if (_mChildren.count (*ur) == 0) {
            return false;
        }

        return _mChildren.at (*ur).matches (srcARD, dstARD);
    }

    template <int Depth, int Total_depth, typename VAL>
    const VAL & RangeTrieNode<Depth, Total_depth, VAL>::retrieve (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD)
    {
        if ((_mChildren.size == 0) && (_pARDVal.first.first == nullptr)) {
            throw std::exception {std::string {"Element matching addresses "} + scrARD.getAddressRangeStringDescription() +
                " and " + scrARD.getAddressRangeStringDescription() + " not found"};
        }
        if (_mChildren.size == 0) {
            if (((Depth > 5) && _pARDVal.first.first->contains (srcARD, Total_depth - 5) && _pARDVal.first.second->contains (dstARD)) ||
                ((Depth < 5) && _pARDVal.first.second->contains (dstARD, Total_depth - 5))) {
                return _pARDVal.second;
            }

            throw std::exception {std::string {"Element matching addresses "} + scrARD.getAddressRangeStringDescription() +
                " and " + scrARD.getAddressRangeStringDescription() + " not found"};
        }

        const URange<uint8> * const ur = (Depth > 5) ?
            static_cast<const URange<uint8> *> (srcARD[Total_depth - Depth]) :
            static_cast<const URange<uint8> *> (dstARD[Total_depth - Depth]);
        if (_mChildren.count (*ur) == 0) {
            throw std::exception {std::string {"Element matching addresses "} + scrARD.getAddressRangeStringDescription() +
                " and " + scrARD.getAddressRangeStringDescription() + " not found"};
        }

        return _mChildren[*ur].second.retrieve (srcARD, dstARD);
    }


    // Specialization -- Depth = 5
    template <int Total_depth, typename VAL>
    RangeTrieNode<5, Total_depth, VAL>::RangeTrieNode (void) :
        _pARDVal {nullptr, VAL{}}
    { }

    template <int Total_depth, typename VAL>
    void RangeTrieNode<5, Total_depth, VAL>::insert (const NetworkAddressRange * const pSrcARD, const NetworkAddressRange * const pDstARD,
                                                     const VAL & val)
    {
        if ((_mChildren.size() == 0) && (_pARDVal.first.first == nullptr)) {
            _pARDVal.first.first = pSrcARD;
            _pARDVal.first.second = pDstARD;
            _pARDVal.second = val;

            return;
        }
        else if (_mChildren.size() == 0) {
            _mChildren[*(static_cast<const URange<uint16> *> ((*(_pARDVal.first.first))[Total_depth - 5]))] = RangeTrieNode<5 - 1, Total_depth, VAL> {};
            _mChildren[*(static_cast<const URange<uint16> *> ((*(_pARDVal.first.first))[Total_depth - 5]))].insert (_pARDVal.first.first, _pARDVal.first.second,
                                                                                                                   _pARDVal.second);
            _pARDVal.first.first = nullptr;
            _pARDVal.first.second = nullptr;
            _pARDVal.second = VAL{};
        }

        _mChildren[*(static_cast<const URange<uint16> *> ((*pSrcARD)[Total_depth - 5]))] = RangeTrieNode<5 - 1, Total_depth, VAL> {};
        _mChildren[*(static_cast<const URange<uint16> *> ((*pSrcARD)[Total_depth - 5]))].insert (pSrcARD, pDstARD, val);
    }

    template <int Total_depth, typename VAL>
    bool RangeTrieNode<5, Total_depth, VAL>::matches (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD) const
    {
        if ((_mChildren.size == 0) && (_pARDVal.first.first == nullptr)) {
            return false;
        }
        if ((_mChildren.size == 0)) {
            return _pARDVal.first.first->contains (srcARD, Total_depth - 5) && _pARDVal.first.second->contains (dstARD);
        }

        const URange<uint16> * const ur = static_cast<const URange<uint16> *> (srcARD[Total_depth - 5]);
        if (_mChildren.count (*ur) == 0) {
            return false;
        }

        return _mChildren.at (*ur).matches (srcARD, dstARD);
    }

    template <int Total_depth, typename VAL>
    const VAL & RangeTrieNode<5, Total_depth, VAL>::retrieve (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD)
    {
        if ((_mChildren.size == 0) && (_pARDVal.first.first == nullptr)) {
            throw std::exception {std::string {"Element matching addresses "} + scrARD.getAddressRangeStringDescription() +
                " and " + scrARD.getAddressRangeStringDescription() + " not found"};
        }
        if (_mChildren.size == 0) {
            if (_pARDVal.first.first->contains (srcARD, Total_depth - 5) && _pARDVal.first.second->contains (dstARD)) {
                return _pARDVal.second;
            }

            throw std::exception {std::string {"Element matching addresses "} + scrARD.getAddressRangeStringDescription() +
                " and " + scrARD.getAddressRangeStringDescription() + " not found"};
        }

        const URange<uint16> * const ur = static_cast<const URange<uint16> *> (srcARD[Total_depth - 5]);
        if (_mChildren.count (*ur) == 0) {
            throw std::exception {std::string {"Element matching addresses "} + scrARD.getAddressRangeStringDescription() +
                " and " + scrARD.getAddressRangeStringDescription() + " not found"};
        }

        return _mChildren[*ur].second.retrieve (srcARD, dstARD);
    }


    // Specialization -- Depth = 0
    template <int Total_depth, typename VAL>
    RangeTrieNode<0, Total_depth, VAL>::RangeTrieNode (void)
    { }

    template <int Total_depth, typename VAL>
    void RangeTrieNode<0, Total_depth, VAL>::insert (const NetworkAddressRange * const pSrcARD, const NetworkAddressRange * const pDstARD,
                                                     const VAL & val)
    {
        _mLeaves[*(static_cast<const URange<uint16> *> ((*pDstARD)[Total_depth]))] = std::make_pair (std::make_pair (pSrcARD, pDstARD), val);
    }

    template <int Total_depth, typename VAL>
    bool RangeTrieNode<0, Total_depth, VAL>::matches (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD) const
    {
        const URange<uint16> * const ur = static_cast<const URange<uint16> *> (dstARD[Total_depth]);
        if (_mLeaves.count (*ur) == 0) {
            return false;
        }

        return _mLeaves.at (*ur).first.first != nullptr;
    }

    template <int Total_depth, typename VAL>
    const VAL & RangeTrieNode<0, Total_depth, VAL>::retrieve (const NetworkAddressRange & srcARD, const NetworkAddressRange & dstARD)
    {
        return _mLeaves[*(static_cast<const URange<uint16> *> (dstARD[Total_depth]))].second;
    }

}

#endif  // INCL_RANGE_TRIE_H