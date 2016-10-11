/*
 * C45TreeInfo.h
 *
 * This file is part of the IHMC C4.5 Decision Tree Library.
 * Copyright (c) 2008-2016 IHMC.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on November 23, 2011, 12:00 PM
 */

#ifndef INCL_C45_TREE_INFO_H
#define INCL_C45_TREE_INFO_H

namespace IHMC_C45
{
    class C45TreeInfo
    {
        public:
            C45TreeInfo();
            C45TreeInfo(int prunedSize, int unprunedSize, int actualWindow, int noTrees);
            virtual ~C45TreeInfo();

            int getPrunedTreeSize(void);
            int getUnprunedTreeSize(void);
            int getActualWindow(void);

            /**
             *  Keep the count of the number of pruned trees costructed over
             *  time.
             */
            int getVersion(void);
            void copyInfo(C45TreeInfo * oldTreeInfo);

        private:
            int _prunedSize;
            int _unprunedSize;
            int _actualWindow;
            int _noTrees;
    };

    inline C45TreeInfo::C45TreeInfo()
    {
        _prunedSize = 0;
        _unprunedSize = 0;
        _actualWindow = 0;
        _noTrees = 0;
    }

    inline C45TreeInfo::C45TreeInfo(int prunedSize, int unprunedSize, int actualWindow, int noTrees)
    {
        _prunedSize = prunedSize;
        _unprunedSize = unprunedSize;
        _actualWindow = actualWindow;
        _noTrees = noTrees;
    }

    inline int C45TreeInfo::getPrunedTreeSize(void)
    {
        return _prunedSize;
    }

    inline int C45TreeInfo::getUnprunedTreeSize(void)
    {
        return _unprunedSize;
    }

    inline int C45TreeInfo::getActualWindow(void)
    {
        return _actualWindow;
    }

    inline int C45TreeInfo::getVersion(void)
    {
        return _noTrees;
    }

    inline void C45TreeInfo::copyInfo(C45TreeInfo * oldTreeInfo)
    {
        _prunedSize = oldTreeInfo->_prunedSize;
        _unprunedSize = oldTreeInfo->_unprunedSize;
        _actualWindow = oldTreeInfo->_actualWindow;
        _noTrees = oldTreeInfo->_noTrees;
    }

    inline C45TreeInfo::~C45TreeInfo()
    {
    }
}

#endif // INCL_C45_TREE_INFO_H
