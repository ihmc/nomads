/* 
 * SubscriptionAdvTable.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * The Subscription Advertisement Table collects all the nodes that subscribe to a certain
 * group. For each node it also keeps track of the Path which is a concatenation of NodeIds
 * (NodeA:NodeB:NodeC).
 * The table has the following structure:
 * -----------------------------
 * | GroupName |  Table of NodeIDs | Lifetime, PathList  |
 * 
 * More specifically, for each group the table maintains a table of Subscribing Nodes which 
 * includes a NodeId and a list of Paths.
 * Each Subscribing Node is subjected to aging so it is removed from the table when its lifetime
 * is greater than _i64LifeTimeUpdateThreshold parameter.
 *
 * Author: Mirko Gilioli (mgilioli@ihmc.us)
 * Created on July 30, 2012, 6:33 AM
 */

#ifndef INCL_SUBSCRIPTION_ADV_TABLE_H
#define	INCL_SUBSCRIPTION_ADV_TABLE_H

#include "PtrLList.h"
#include "StrClass.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    template <class T> class PtrLList;
    template <class T> class StringHashtable;
    class StrClass;
}


namespace IHMC_ACI
{
    class SubscribingNode
    {
        public:
            SubscribingNode (void);
            virtual ~SubscribingNode (void);
            
            int addPath (NOMADSUtil::String *pszPath);
            int removePath (NOMADSUtil::String *pszPath);
            int removePathWithNode (NOMADSUtil::String *pszNodeId);
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getPathList (void);
            int64 getLifeTime (void);
            void refreshLifeTime (void);
            void display (void);
            
        private:
            NOMADSUtil::PtrLList<NOMADSUtil::String> *_pPathList;
            int64 _i64LifeTime;
    };
    
    
    class SubscriptionAdvTableEntry
    {
        public:
            SubscriptionAdvTableEntry (void);
            virtual ~SubscriptionAdvTableEntry (void);
            
            int addSubscribingNode (const char *pszNodeId);
            int addPathToNode (const char *pszNodeId, NOMADSUtil::String *pszPath);
            bool hasSubscribingNode (const char *pszNodeId);
            int removePathFromNode (const char *pszNodeId, NOMADSUtil::String *pszPath);
            int removeSubscribingNode (const char *pszNodeId);
            SubscribingNode * getSubscribingNode (const char *pszNodeId);
            int getAllSubscribingNodes (NOMADSUtil::PtrLList<SubscribingNode> *pSubNodeList);
            int getAllSubscribingNodesKeys (NOMADSUtil::PtrLList<const char> *pSubNodeKeyList);
            bool isEmpty (void);
            int refreshLifeTime (const char *pszSubNodeId);
            void display (void);
            
        private:
            NOMADSUtil::StringHashtable<SubscribingNode> _subNodesTable;          
    };
    
    class SubscriptionAdvTable
    {
        public:
            SubscriptionAdvTable (void);
            virtual ~SubscriptionAdvTable (void);
            
            int addSubscription (const char *pszGroupName);
            int addSubscribingNode (const char *pszGroupName, const char *pszNodeId);
            int addSubscribingNodePath (const char *pszGroupName, const char *pszNodeId,
                                        NOMADSUtil::String *pszPath);
            
            bool hasSubscription (const char *pszGroupName);
            bool hasSubscribingNode (const char *pszGroupName, const char *pszNodeId);
            bool hasSubscribingNode (const char *pszNodeId);
            bool hasSubscribingNodePath (const char *pszGroupName, const char *pszNodeId,
                                         NOMADSUtil::String *pszPath);
            
            SubscriptionAdvTableEntry * getTableEntry (const char *pszGroupName);
            int removeSubscription (const char *pszGroupName);
            int removeSubscribingNode (const char *pszGroupName, const char *pszNodeId);
            int removeSubscribingNodePath (const char *pszGroupName, const char *pszNodeId,
                                           NOMADSUtil::String *pszPath);
            
            void configureTimers (int64 i64LifeTimeUpdateThreshold);
            void display (void); 
            bool isEmpty (void);
            void refreshTable (void);
            void deadNode (const char *pszNodeId);
            
        private:
            static const int64 DEFAULT_LIFETIME_UPDATE_THRESHOLD = 60000;   // Time in Milliseconds
            NOMADSUtil::StringHashtable<SubscriptionAdvTableEntry> _subAdvTable;
            int64 _i64LifeTimeUpdateThreshold;
    };
}


#endif	// INCL_SUBSCRIPTION_ADV_TABLE_H

