/*
 * DataGenerator.h
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

#ifndef INCL_DATAGENERATOR_H
#define INCL_DATAGENERATOR_H

#include "types.h"

#include <stdio.h>

namespace NOMADSUtil
{
    class String;
}

namespace IHMC_C45
{
    class C45AVList;
    class C45DecisionTree;

    class DataGenerator
    {
        public:
            // xml tags
            static const NOMADSUtil::String _XML_HEADER;
            static const NOMADSUtil::String _XML_DATA_ITEM;         // Tag used to identify one item in the dataset.
            static const NOMADSUtil::String _XML_ATTRIBUTE;         // Identifies an attribute inside the item.
            static const NOMADSUtil::String _XML_ATTRIBUTE_NAME;    // Inside an attribute, identifies the name.
            static const NOMADSUtil::String _XML_ATTRIBUTE_VALUE;   // Inside an attribute, identifies the value.
            static const NOMADSUtil::String _XML_CLASS_NAME;        // Class name. The class is considered as an attribute.

            DataGenerator();
            virtual ~DataGenerator();

            int initializeAttributes(C45AVList * treeAttributes);   // Initialize the attributes names. The given parameter
                                                                    // must be the same of the trees that will be used with
                                                                    // this generator.
                                                                    // Returns a number != 0 if an error occurred.

            int initializeAttributes(const char * pszFileName);     // Initialize attribute names from file.

            int initializeAttributes(const char * pszFileName, C45DecisionTree * decisionTree);
                                                                    // This method initialize both the generator and the
                                                                    // given tree with the attribute names taken from
                                                                    // the given file.

            int configureGenerator(C45AVList * * rules, int noRules, C45AVList * ranges, float noiseLevel);
                                                                    // Configure the generator with rules that will be
                                                                    // used to generate the data and a "noiseLevel" that must
                                                                    // be in the interval [0 - 1]. The parameter "ranges"
                                                                    // contains one range of values for each attribute
                                                                    // declared as "_CONTINUOUS". This method will generate
                                                                    // random values in that range, so it's more realistic.
                                                                    // For details about the input see C45AVList.h
                                                                    // Returns a number != 0 if an error occurred.

            int configureGenerator(const char * fileName);  // Configure the generator to take the dataset from the
                                                            // given file. Returns the dataset length or a number < 0
                                                            // in case of errors.

            C45AVList * generateDataset(int datasetLength); // Generate a dataset and returns it. The function returns
                                                            // a NULL pointer in case of errors.

            int generateDataset(int datasetLength, const char * pszFileName);
                                                        // Generate a dataset and store each item of the dataset
                                                        // in a file in xml format. All the file names will be
                                                        // compose by the given "pszFileName" and a number.
                                                        // The tags used to generate the xml file are defined
                                                        // as costants of DataGenerator.

            const char * getErrorMessage(void); // Returns a message that explains the error occurred.

        private:
            struct _attrRange {  // Save the ranges.
                int _attrNo;
                float _lowerBound;
                float _upperBound;
            } * _pAttrRange;
            int _countContinuous;   // Number of structures in the array.

            struct _ruleStruct {    // Save the rules.
                int _classNo;
                int _MaxAttr;
                int * _pAttrNo;
                int * _pMaxCond;
                short * * _pConditionStatus;	// 0 = equal, 1 = ">", 2 = "<", 3 = range
                                                // 4 = "!=", 5 = ">=", 6 = "<="
                Description * _pConditionValue;
                float * * _pConditionRange;	// used just for ranges
            } * _pRules;
            int _MaxRules;
            char * _pszDefaultClass;

            Configure * _pConfigure;
            CError * _pErrOcc;
            char * _pszErrorMessage;
            float _noiseLevel;

            bool _fileMode;     // true = take data from file
            int _MaxItem;
            int _actualItem;
            Description * _pItem;   // save dataset from file

            int analizeCondition(C45AVList * * rules, int index, int number,
                                 int indexRule, int indexValue, int noRules);

            bool readFromFile(FILE * file, char * buffer, char * del);
    };

    inline const char * DataGenerator::getErrorMessage(void)
    {
        return _pszErrorMessage;
    }
}

#endif // INCL_DATAGENERATOR_H
