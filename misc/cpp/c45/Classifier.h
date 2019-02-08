/*
 * Classifier.h
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
#ifndef CLASSIFIER_H_
#define CLASSIFIER_H_

#include "C45AVList.h"
#include "Prediction.h"
#include "TestInfo.h"

#include "FTypes.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_C45
{
    class Classifier
    {
        public:
            Classifier (void);
            virtual ~Classifier (void);

            virtual int configureClassifier (C45AVList * attributes)=0;

            // Add data to the dataset.
            // Returns 0 if it was successful, an error code > 0 otherwise
            virtual int addNewData (C45AVList * dataset)=0;
            virtual Prediction * consultClassifier (C45AVList * record)=0;
            virtual TestInfo * testClassifierOnData (C45AVList * dataset)=0;
            virtual int64 read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)=0;
            virtual int64 skip (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)=0;
            virtual int64 write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)=0;
            virtual int64 getWriteLength (void)=0;
            virtual uint16 getVersion (void)=0;
    };
}

#endif /*CLASSIFIER_H_*/
