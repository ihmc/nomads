/* 
 * SQLAVList.h
 *
 * This file is part of the IHMC DSPro Library/Component
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
 */

#ifndef INCL_SQL_AV_LIST_H
#define INCL_SQL_AV_LIST_H

#include "AVList.h"
#include "MetadataInterface.h"

namespace NOMADSUtil
{
    class String;
}

namespace IHMC_ACI
{
    class SQLAVList : public NOMADSUtil::AVList,
                      public MetadataInterface
    {
        public:
            explicit SQLAVList (unsigned int uiInitialSize = 0U);
            virtual ~SQLAVList (void);

            SQLAVList * concatListsInNewOne (SQLAVList *pFirst, SQLAVList *pSecond);

            /**
             * Returns a copy of this object
             */
            SQLAVList * copyList (void);

            const char * getFieldName (unsigned int uiIndex) const;
            const char * getFieldType (unsigned int uiIndex) const;
            const char * getFieldValueByIndex (unsigned int uiIndex) const;

            MetadataInterface::MetadataWrapperType getMetadataType (void);

            bool isFieldAtIndexUnknown (unsigned int uiIndex) const;
            int setFieldValue (const char *pszFieldName, const char *pszValue);
            int resetFieldValue (const char *pszFieldName);

        protected:
            int findFieldValue (const char *pszFieldName, const char **ppszValue) const;
    };

    inline MetadataInterface::MetadataWrapperType SQLAVList::getMetadataType()
    {
        return MetadataInterface::List;
    }
}

#endif // INCL_SQL_AV_LIST_H

/************************************* How to use SQLAVList class ************************************************/
/*
 * This class is passed as a parameter in DisServicePro and is used to represent metadata information.
 * 
 * There are three input types:
 * 
 *  1) a list of fields and their types, used to configure the MetaData fields when calling DisServicePro::init()
 *  2) a list of fields and their values, used to specify the values for a specific MetaData instance when
 *        calling DisServicePro::pushPro()
 *  3) a list of fields and all their possible values, used to initialize the learning algorithm when calling
 *     DisServicePro::setMetaDataPossibleValues()
 *
 * Each type of input can be represented by the class SQLAVList, but is important to know that each type has its
 * own syntax, that must be followed or the input won't be accepted.
 */
/************************************* 1 - Fields names and types list syntax *************************************/
/*
 * The MetaData structure is flexible and it's possible to specify a variable number of fields. Each field must have a
 * name and a type. To specify a type you must use one of the constants declared in this class. The _UNKNOWN constant
 * is not used to declare types, but to define values (see point 2). This is an example of how declare fields:
 * 
 *                             SQLAVList * fieldStructure = new SQLAVList();
 *                             fieldStructure->addPair("Description", SQLAVList::_TEXT);
 *                             fieldStructure->addPair("Latitude", SQLAVList::_FLOAT);
 *                             fieldStructure->addPair("Time", SQLAVList::_INTEGER64);
 */
/************************************ 2 - Fields names and values list syntax ************************************/
/*
 * The class SQLAVList could also be used to insert value in a MetaData instance. The order of the inserted fields
 * is not important. If one of the values is unknown, there are two possibility: you can just omit the pair
 * "field name"/"unknown value" or you can use the constant SQLAVList::_UNKNOWN to specify it. This is an example
 * of how specify fields values:
 * 
 *                             SQLAVList * fieldValues = new SQLAVList();
 *                             fieldValues->addPair("Description", "map of area D5");
 *                             fieldValues->addPair("Time", SQLAVList::_UNKNOWN);
 *                             fieldValues->addPair("Latitude", 44.184320);
 */
/************************************ 3 - Fields names and all possible values sintax ****************************/
/*
 * The learning algorithm needs to known in advance the possible values of the metadata fields declared with type
 * "TEXT" and used in the learning (not all the "TEXT" fields are used int the learning). Because the metadata fields
 * are choosen at run time, DisServicePro does not known in advance which fields are to be used by the learning algorithm
 * and which not (actually it is only aware of the "fixed fields", but an application can decide to use also other fields).
 * The method setMetaDataPossibleValues() is used also to tell to DisServicePro which fields use in the learning.
 * For "TEXT" fields, it's not mandatory to specify all the possible values for a field, but the learning algorithm will
 * work better if it knowns the values in advance. If is not possible to specify all the values in advance, the algorithm
 * needs to known at least the number of possible values it will see in the future. If this number is not known in advance,
 * it's better to estimate it considering the maximun possible number of values. If you want to use a numeric field
 * (that is not in the fixed fields) in the learning, this field must be inserted in the input for
 * setMetaDataPossibleValues(). Let's explain the sintax with an example:
 * 
 * Imagine that the customized fields added previously with DisServicePro::init() are these:
 * 
 *                             FIELD NAME       FIELD TYPE
 *                             Gain             float
 *                             Description      text
 *                             SequenceID       integer16
 *                             DataType         text
 *                             SourceID         text
 * 
 * And imagine that we want to use "Gain", "DataType" and "SourceID" in the learning. These are the steps to do that:
 * 
 *               - Create a SQLAVList instance:
 * 
 *                             SQLAVList * learningFields = new SQLAVList(3);
 * 
 *               - "Gain" : this is a field with float type so we need just to mention it without specifing values. The
 *                 "value" parameter in SQLAVList::addPair(const char * attribute, const char * value) will not be considered:
 * 
 *                             learningFields->addPair("Gain", "");
 * 
 *               - "DataType" : imagine that this field has well known possible values in advance:
 * 
 *                             learningFields->addPair("DataType", "Image");
 *                             learningFields->addPair("DataType", "Text");
 *                             learningFields->addPair("DataType", "Video");
 *                             learningFields->addPair("DataType", "Map");
 * 
 *               - "SourceID" : imagine that the values of this field are not known in advance, but is known that there
 *                 are maximum 10 values:
 * 
 *                             learningFields->addPair("SourceID", 10);
 */

