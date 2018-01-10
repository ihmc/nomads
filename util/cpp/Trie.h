/*
* AtomicVar.h
*
* This file is part of the IHMC Util Library
* Copyright (c) 1993-2016 IHMC.
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
* Implements a template Trie data structure.
*
* Tries are efficient solutions to index and search strings,
* find the longest prefix match (e.g., IP lookup in routing table),
* and to implement auto-completion (e.g., web browsers and text editors).
*
* authors : Alessandro Morelli          amorelli@ihmc.us
*/

#ifndef INCL_TRIE_H
#define INCL_TRIE_H

#include "FTypes.h"
#include "PtrLList.h"


using namespace NOMADSUtil;

namespace NOMADSUtil {

    template <typename CHAR, typename VAL>
    class Trie
    {
        class Node
        {
        public:
            Node (const CHAR & cchar) :
                _cChar(cchar), _val(), _bHasVal(false) {};
            Node (const CHAR & cchar, const VAL & val) :
                _cChar(cchar), _val(val), _bHasVal(true) {};
            ~Node (void) { _plChildren.removeAll (true); }

            Node (const Node & rNode) = delete;         // Cannot copy construct
            Node (Node && urNode) = delete;             // Cannot move construct
            Node & operator= (const Node & rNode);
            Node & operator= (Node && urNode);

            bool hasData (void) const { return _bHasVal; }
            CHAR getIndex (void) const { return _cChar; }
            VAL getData (void) const { return _val; }
            void setData (const VAL & val);

            void appendChild (Node *pNode) { _plChildren.append (pNode); }
            Node * findChild (const CHAR & cchar);

        private:
            friend class Trie<CHAR, VAL>;

            Node (void) :
                _cChar(), _val(), _bHasVal(false) {};

            const CHAR _cChar;
            VAL _val;
            bool _bHasVal;
            PtrLList<Node> _plChildren;
        };


    public:
        Trie() { pRoot = new Node(); }
        ~Trie() { delete pRoot; }

        void addVal (const CHAR * const strIndex, const uint16 length, const VAL & val);
        bool searchVal (const CHAR * const strIndex, const uint16 length, VAL & outVal);
        bool longestPrefixSearch (const CHAR * const strIndex, const uint16 length, VAL & outVal);

    private:
        Node* pRoot;
    };


    template <typename CHAR, typename VAL>
    typename Trie<CHAR, VAL>::Node & Trie<CHAR, VAL>::Node::operator= (const typename Trie<CHAR, VAL>::Node & rNode)
    {
        if (this == &rNode) {
            return *this;
        }

        _cChar = rNode._cChar;
        _val = rNode._val;
        _plChildren.removeAll (true);
        _plChildren.insertAllCopies (&rNode._plChildren);
    }

    template <typename CHAR, typename VAL>
    typename Trie<CHAR, VAL>::Node & Trie<CHAR, VAL>::Node::operator= (typename Trie<CHAR, VAL>::Node && rNode)
    {
        _cChar = rNode._cChar;
        rNode._cChar = CHAR();
        _val = rNode._val;
        rNode._val = VAL();

        _plChildren.removeAll (true);
        _plChildren.insertAll (&rNode._plChildren);
        rNode._plChildren.removeAll();
    }

    template <typename CHAR, typename VAL>
    void Trie<CHAR, VAL>::Node::setData (const VAL & val)
    {
        _val = val;
        _bHasVal = true;
    }

    template <typename CHAR, typename VAL>
    typename Trie<CHAR, VAL>::Node * Trie<CHAR, VAL>::Node::findChild (const CHAR & cchar)
    {
        using TrieNode = typename Trie<CHAR, VAL>::Node;

        for (TrieNode * tn = _plChildren.getFirst(); tn != nullptr; tn = _plChildren.getNext()) {
            if (tn->getIndex() == cchar) {
                return tn;
            }
        }

        return nullptr;
    }

    template <typename CHAR, typename VAL>
    void Trie<CHAR, VAL>::addVal (const CHAR * const strIndex, const uint16 length, const VAL & val)
    {
        using TrieNode = typename Trie<CHAR, VAL>::Node;

        TrieNode * pCurrent = pRoot;
        if (length == 0) {
            pCurrent->setData (val);     // Empty pattern
            return;
        }

        for (int i = 0; i < length; ++i) {
            TrieNode * child = pCurrent->findChild (strIndex[i]);
            if (child) {
                pCurrent = child;
            }
            else {
                TrieNode * tmp = new TrieNode(strIndex[i]);
                pCurrent->appendChild (tmp);
                pCurrent = tmp;
            }

            if (i == (length - 1)) {
                pCurrent->setData (val);
            }
        }
    }

    template <typename CHAR, typename VAL>
    bool Trie<CHAR, VAL>::searchVal (const CHAR * const strIndex, const uint16 length, VAL & outVal)
    {
        using TrieNode = typename Trie<CHAR, VAL>::Node;

        TrieNode * pCurrent = pRoot;
        while (pCurrent) {
            for (int i = 0; i < length; ++i) {
                TrieNode * tmp = pCurrent->findChild (strIndex[i]);
                if (!tmp) {
                    return false;
                }
                pCurrent = tmp;
            }

            if (pCurrent->hasData()) {
                outVal = pCurrent->getData();
                return true;
            }
            return false;
        }

        return false;
    }

    template <typename CHAR, typename VAL>
    bool Trie<CHAR, VAL>::longestPrefixSearch (const CHAR * const strIndex, const uint16 length, VAL & outVal)
    {
        using TrieNode = typename Trie<CHAR, VAL>::Node;

        TrieNode * pCurrent = pRoot, * pLPM = nullptr;
        if (!pCurrent) {
            return false;
        }

        for (int i = 0; i < length; ++i) {
            if (pCurrent->hasData()) {
                pLPM = pCurrent;
            }

            TrieNode * tmp = pCurrent->findChild (strIndex[i]);
            if (!tmp) {
                if (pLPM) {
                    // Longest prefix search found a shorter prefix match
                    outVal = pLPM->getData();
                    return true;
                }
                return false;
            }
            pCurrent = tmp;
        }
        if (pCurrent->hasData()) {
            pLPM = pCurrent;
        }

        if (pLPM) {
            outVal = pLPM->getData();
            return true;
        }
        return false;
    }

}

#endif
