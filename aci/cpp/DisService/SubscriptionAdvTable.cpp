/*
 * SubscriptionAdvTable.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "SubscriptionAdvTable.h"
#include "NLFLib.h"
#include "DArray2.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//
//  SubscribingNode
//

SubscribingNode::SubscribingNode()
{
    _pPathList = new NOMADSUtil::PtrLList<NOMADSUtil::String>();
}

SubscribingNode::~SubscribingNode()
{
    NOMADSUtil::String *pCurr, *pNext, *pDel;
    pNext = _pPathList->getFirst();
    while ((pCurr = pNext) != NULL) {
        pNext = _pPathList->getNext();
        pDel = _pPathList->remove (pCurr);
        delete pDel;
    }

    delete _pPathList;   
}

NOMADSUtil::PtrLList<NOMADSUtil::String> * SubscribingNode::getPathList()
{
    return _pPathList;
}

int SubscribingNode::addPath (NOMADSUtil::String *pszPath)
{
    if (pszPath == NULL) {
        return -1;
    }
    
    _pPathList->prepend (pszPath);
    
    return 0;
}

int SubscribingNode::removePath (NOMADSUtil::String *pszPath)
{
    if (pszPath == NULL) {
        return -1;
    }
    
    NOMADSUtil::String *pszRet = _pPathList->remove (pszPath);
    if (pszRet == NULL) {
        return -1;
    }
    
    delete pszRet;
    return 0;
}

int SubscribingNode::removePathWithNode (NOMADSUtil::String *pszNodeId)
{
    if (pszNodeId == NULL) {
        return -1;
    }

    NOMADSUtil::DArray2<NOMADSUtil::String *> pathToDelete;
    int i = 0;
    
    // Search the NodeId in the list of Path. If found it then delete the path
    for (NOMADSUtil::String *pszPath = _pPathList->getFirst();
         pszPath != NULL; pszPath = _pPathList->getNext()) {
        
        if (pszPath->indexOf (pszNodeId->c_str()) >= 0) {
            pathToDelete[i++] = pszPath;
        }
    }

    for (unsigned int k = 0; k < pathToDelete.size(); k++) {
        if (pathToDelete.used (k)) {
            String *pDel = _pPathList->remove (pathToDelete[k]);
            if (pDel != NULL) {
                delete pDel;
            }
        }
    }

    return 0;
}

int64 SubscribingNode::getLifeTime()
{
    return _i64LifeTime;
}

void SubscribingNode::refreshLifeTime()
{
    _i64LifeTime = getTimeInMilliseconds();
}

//
//  SubscriptionAdvTableEntry
//

SubscriptionAdvTableEntry::SubscriptionAdvTableEntry()
{
    _subNodesTable.configure (true, true, true, true);
}

SubscriptionAdvTableEntry::~SubscriptionAdvTableEntry()
{
    NOMADSUtil::StringHashtable<SubscribingNode>::Iterator i = _subNodesTable.getAllElements();
    for (;!i.end(); i.nextElement()) {
        SubscribingNode *pDel = _subNodesTable.remove (i.getKey());
        if (pDel) {
            delete pDel;
            pDel = NULL;
        }
    }
}

int SubscriptionAdvTableEntry::addSubscribingNode (const char *pszNodeId)
{
    if (pszNodeId == NULL) {
        return -1;
    }

    SubscribingNode *pSubNode = new SubscribingNode ();
    if (pSubNode == NULL) {
        return -1;
    }
    pSubNode->refreshLifeTime();
    _subNodesTable.put (pszNodeId, pSubNode);
    
    return 0;
}

int SubscriptionAdvTableEntry::addPathToNode (const char *pszNodeId, NOMADSUtil::String *pszPath)
{
    if (pszNodeId == NULL || pszPath == NULL) {
        return -1;
    }
    
    SubscribingNode *pRet = _subNodesTable.get (pszNodeId);
    if (pRet == NULL) {
        return -1;
    }
    pRet->refreshLifeTime();
    pRet->addPath (pszPath);
    
    return 0;
}

bool SubscriptionAdvTableEntry::hasSubscribingNode (const char *pszNodeId)
{
    if (pszNodeId == NULL) {
        return false;
    }
    
    if (_subNodesTable.get (pszNodeId) == NULL) {
        return false;
    }
    
    return true;
}

SubscribingNode * SubscriptionAdvTableEntry::getSubscribingNode (const char *pszNodeId)
{
    if (pszNodeId == NULL) {
        return NULL;
    }
    
    return _subNodesTable.get (pszNodeId);
}

int SubscriptionAdvTableEntry::getAllSubscribingNodes (NOMADSUtil::PtrLList<SubscribingNode> *pSubNodeList)
{
    if (pSubNodeList == NULL) {
        return -1;
    }
    
    for (NOMADSUtil::StringHashtable<SubscribingNode>::Iterator i = _subNodesTable.getAllElements();
         !i.end(); i.nextElement()) {
        
        pSubNodeList->prepend (i.getValue());
    }

    return 0;
}

int SubscriptionAdvTableEntry::getAllSubscribingNodesKeys (NOMADSUtil::PtrLList<const char> *pSubNodeKeyList)
{
    if (pSubNodeKeyList == NULL) {
        return -1;
    }
    
    for (NOMADSUtil::StringHashtable<SubscribingNode>::Iterator i = _subNodesTable.getAllElements();
         !i.end(); i.nextElement()) {
        
        pSubNodeKeyList->prepend (i.getKey());
    }
    
    return 0;
}

int SubscriptionAdvTableEntry::removeSubscribingNode (const char *pszNodeId)
{
    if (pszNodeId == NULL) {
        return -1;
    }
    
    SubscribingNode *pRet = _subNodesTable.remove (pszNodeId);
    if (pRet == NULL) {
        return -1;
    }
    
    delete pRet;
    return 0;
}

int SubscriptionAdvTableEntry::removePathFromNode (const char *pszNodeId, NOMADSUtil::String *pszPath)
{
    if (pszNodeId == NULL || pszPath == NULL) {
        return -1;
    }
    
    SubscribingNode *pRet = _subNodesTable.get (pszNodeId);
    if (pRet == NULL) {
        return -1;
    }
    
    if (pRet->removePath (pszPath) < 0) {
        return -1;
    }
    
    return 0;
}

bool SubscriptionAdvTableEntry::isEmpty()
{
    if (_subNodesTable.getCount() == 0) {
        return true;
    }
    return false;
}

int SubscriptionAdvTableEntry::refreshLifeTime (const char* pszSubNodeId)
{
    if (pszSubNodeId == NULL) {
        return -1;
    }
    
    SubscribingNode *pRet = _subNodesTable.get (pszSubNodeId);
    if (pRet == NULL) {
        return -1;
    }
    
    pRet->refreshLifeTime();
    
    return 0;
}
//
//  SubscriptionAdvTable
//

SubscriptionAdvTable::SubscriptionAdvTable ()
{
    _subAdvTable.configure (true, true, true, true);
}

SubscriptionAdvTable::~SubscriptionAdvTable()
{
    
}

int SubscriptionAdvTable::addSubscription (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return -1;
    }
    
    SubscriptionAdvTableEntry *pEntry = new SubscriptionAdvTableEntry ();
    if (pEntry == NULL) {
        return -1;
    }
    
    _subAdvTable.put (pszGroupName, pEntry);
    return 0;
}

int SubscriptionAdvTable::addSubscribingNode (const char *pszGroupName, const char *pszNodeId)
{
    if (pszGroupName == NULL || pszNodeId == NULL) {
        return -1;
    }

    SubscriptionAdvTableEntry *pEntry = _subAdvTable.get (pszGroupName);
    // If it does not exist the entry create it.
    if (pEntry == NULL) {
        pEntry = new SubscriptionAdvTableEntry ();
        if (pEntry == NULL) {
            return -1;
        }
        _subAdvTable.put (pszGroupName, pEntry);
    }  

    if (pEntry->addSubscribingNode (pszNodeId) < 0) {
        return -1;
    }

    return 0;
}

int SubscriptionAdvTable::addSubscribingNodePath (const char *pszGroupName, const char *pszNodeId, NOMADSUtil::String *pszPath)
{
    if (pszGroupName == NULL || pszNodeId == NULL || pszPath == NULL) {
        return -1;
    }
    
    SubscriptionAdvTableEntry *pEntry = _subAdvTable.get (pszGroupName);
    // If it does not exist the entry create it.
    if (pEntry == NULL) {
        pEntry = new SubscriptionAdvTableEntry ();
        if (pEntry == NULL) {
            return -1;
        }
        _subAdvTable.put (pszGroupName, pEntry);
    }
     
    SubscribingNode *pSubNode = pEntry->getSubscribingNode (pszNodeId);
    if (pSubNode == NULL) {
        pEntry->addSubscribingNode (pszNodeId);
    }
    
    pEntry->addPathToNode (pszNodeId, pszPath);
    
    return 0;
    
}

bool SubscriptionAdvTable::hasSubscription (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return false;
    }
    
    SubscriptionAdvTableEntry *pEntry = _subAdvTable.get (pszGroupName);
    if (pEntry == NULL) {
        return false;
    }
    
    return true;
}

bool SubscriptionAdvTable::hasSubscribingNode (const char *pszGroupName, const char *pszNodeId)
{
    if (pszGroupName == NULL || pszNodeId == NULL) {
        return false;
    }
    
    SubscriptionAdvTableEntry *pEntry = _subAdvTable.get (pszGroupName);
    if (pEntry == NULL) {
        return false;
    }
    
    if (!pEntry->hasSubscribingNode (pszNodeId)) {
        return false;
    }
    
    return true;
}

bool SubscriptionAdvTable::hasSubscribingNode (const char *pszNodeId)
{
    if (pszNodeId == NULL) {
        return false;
    }
    
    NOMADSUtil::StringHashtable<SubscriptionAdvTableEntry>::Iterator i = _subAdvTable.getAllElements();
    for (; !i.end(); i.nextElement()) {
        SubscriptionAdvTableEntry *pEntry = i.getValue();
        if (pEntry->hasSubscribingNode (pszNodeId)) {
            return true;
        }
    }
    
    return false;
}

bool SubscriptionAdvTable::hasSubscribingNodePath (const char *pszGroupName, const char *pszNodeId, NOMADSUtil::String *pszPath)
{
    if (pszGroupName == NULL || pszNodeId == NULL || pszPath == NULL) {
        return false;
    }
    
    SubscriptionAdvTableEntry *pEntry = _subAdvTable.get (pszGroupName);
    if (pEntry == NULL) {
        return false;
    }
    
    SubscribingNode *pSubNode = pEntry->getSubscribingNode (pszNodeId);
    if (pSubNode == NULL) {
        return false;
    }
    
    NOMADSUtil::PtrLList<NOMADSUtil::String> *pPathList = pSubNode->getPathList();
    for (NOMADSUtil::String *pszCurrPath = pPathList->getFirst(); pszCurrPath != NULL;
         pszCurrPath = pPathList->getNext()) {
        
        if (strcmp (pszCurrPath->c_str(), pszPath->c_str()) == 0) {
            return true;
        }
    }
    
    return false;
}

SubscriptionAdvTableEntry * SubscriptionAdvTable::getTableEntry (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return NULL;
    }
    
    return _subAdvTable.get (pszGroupName);
}

int SubscriptionAdvTable::removeSubscribingNode (const char *pszGroupName, const char *pszNodeId)
{
    if (pszGroupName == NULL || pszNodeId == NULL) {
        return -1;
    }
    
    SubscriptionAdvTableEntry *pEntry = _subAdvTable.get (pszGroupName);
    if (pEntry == NULL) {
        return -1;
    }
    
    return pEntry->removeSubscribingNode (pszNodeId);
}

int SubscriptionAdvTable::removeSubscription (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return -1;
    }
    
    SubscriptionAdvTableEntry *pEntry = _subAdvTable.remove (pszGroupName);
    if (pEntry == NULL) {
        return -1;
    }
    
    delete pEntry;
    return 0;
}

int SubscriptionAdvTable::removeSubscribingNodePath (const char *pszGroupName, const char *pszNodeId, NOMADSUtil::String *pszPath)
{
    if (pszGroupName == NULL || pszNodeId == NULL || pszPath == NULL) {
        return -1;
    }
    
    SubscriptionAdvTableEntry *pEntry = _subAdvTable.get (pszGroupName);
    if (pEntry == NULL) {
        return -1;
    }
    
    return pEntry->removePathFromNode (pszNodeId, pszPath);
}

void SubscriptionAdvTable::refreshTable()
{
    
    NOMADSUtil::DArray2<const char *> tableEntryToRemove;
    NOMADSUtil::DArray2<NOMADSUtil::String *> subNodesDeleted;
    int h = 0;
    int r = 0;
    
    for (NOMADSUtil::StringHashtable<SubscriptionAdvTableEntry>::Iterator i = _subAdvTable.getAllElements();
         !i.end(); i.nextElement()) {
            
        int j = 0;
        SubscriptionAdvTableEntry *pEntry = i.getValue();
        NOMADSUtil::PtrLList<SubscribingNode> subNodeList;
        NOMADSUtil::DArray2<const char *> subNodesToRemove;
        NOMADSUtil::PtrLList<const char> subNodeKeyList;
        
        pEntry->getAllSubscribingNodes (&subNodeList);
        pEntry->getAllSubscribingNodesKeys (&subNodeKeyList);
        // Check for every Subscribing Node if they are expired.
        const char *pszKey = subNodeKeyList.getFirst();
        for (SubscribingNode *pNode = subNodeList.getFirst(); pNode != NULL;
             pNode = subNodeList.getNext(), pszKey = subNodeKeyList.getNext()) {
            
            if (getTimeInMilliseconds() - pNode->getLifeTime() > _i64LifeTimeUpdateThreshold) {
                subNodesToRemove[j++] = pszKey;
                subNodesDeleted[h++] = new NOMADSUtil::String (pszKey);
            }
        }
        // Remove all the expired Node
        for (unsigned int k = 0; k < subNodesToRemove.size(); k++) {
            pEntry->removeSubscribingNode (subNodesToRemove[k]);
        }

        if (pEntry->isEmpty()) {
            tableEntryToRemove[r++] = i.getKey();
        }
    }
    
    // Delete empty entries
    for (unsigned int i = 0; i < tableEntryToRemove.size(); i++) {
        SubscriptionAdvTableEntry *pEntry = _subAdvTable.remove (tableEntryToRemove[i]);
        if (pEntry != NULL) {
            delete pEntry;
        }
    }
    
    // Delete all the paths containing the deleted Nodes.
    if (subNodesDeleted.size() > 0) {
        for (NOMADSUtil::StringHashtable<SubscriptionAdvTableEntry>::Iterator i = _subAdvTable.getAllElements();
            !i.end(); i.nextElement()) {

            SubscriptionAdvTableEntry *pEntry = i.getValue();
            NOMADSUtil::PtrLList<SubscribingNode> subNodeList;
            pEntry->getAllSubscribingNodes (&subNodeList);
            for (SubscribingNode *pNode = subNodeList.getFirst(); pNode != NULL;
                pNode = subNodeList.getNext()) {

                for (unsigned int j = 0; j < subNodesDeleted.size(); j++) {
                    pNode->removePathWithNode (subNodesDeleted[j]);
                }
            }
        }
        
        // Destroy subNodesDeleted 
        for (unsigned int j = 0; j < subNodesDeleted.size(); j++) {
            if (subNodesDeleted.used (j)) {
                delete subNodesDeleted[j];
            }
        }
    }
}

void SubscriptionAdvTable::configureTimers (int64 i64LifeTimeUpdateThreshold)
{
    if (i64LifeTimeUpdateThreshold == 0) {
        _i64LifeTimeUpdateThreshold = DEFAULT_LIFETIME_UPDATE_THRESHOLD;
    }
    else {
        _i64LifeTimeUpdateThreshold = i64LifeTimeUpdateThreshold;
    }
}

bool SubscriptionAdvTable::isEmpty()
{
    if (_subAdvTable.getCount() == 0) {
        return true;
    }
    return false;
}

void SubscriptionAdvTable::deadNode (const char *pszNodeId)
{
    if (pszNodeId == NULL) {
        return;
    }
    
    // Remove all the paths that contain the dead node
    if (!hasSubscribingNode (pszNodeId)) {
        NOMADSUtil::StringHashtable<SubscriptionAdvTableEntry>::Iterator i = _subAdvTable.getAllElements();
        for (; !i.end(); i.nextElement()) {
            SubscriptionAdvTableEntry *pEntry = i.getValue();
            NOMADSUtil::PtrLList<SubscribingNode> subNodeList;
            pEntry->getAllSubscribingNodes (&subNodeList);
            for (SubscribingNode *pSubNode = subNodeList.getFirst(); pSubNode != NULL;
                 pSubNode = subNodeList.getNext()) {
                
                String sNodeToRemove (pszNodeId);
                pSubNode->removePathWithNode (&sNodeToRemove);
            }
        }
    }
    // TODO
    // To improve performance you can delete also the node that has subscribed to a group
    // instead of let them being expired
}

void SubscriptionAdvTable::display()
{
    NOMADSUtil::StringHashtable<SubscriptionAdvTableEntry>::Iterator i = _subAdvTable.getAllElements();
    for (; !i.end(); i.nextElement()) {
        printf ("--------------- Subscription = %s ------------- \n", i.getKey());
        SubscriptionAdvTableEntry *pEntry = i.getValue();
        pEntry->display();
        printf ("\n");
    }
}

void SubscriptionAdvTableEntry::display()
{
    NOMADSUtil::StringHashtable<SubscribingNode>::Iterator i = _subNodesTable.getAllElements();
    for (; !i.end(); i.nextElement()) {
        SubscribingNode *pNode = i.getValue();
        printf ("Node id = %s [%lld] : ", i.getKey(), pNode->getLifeTime());
        pNode->display();
        printf ("\n");
    }
}

void SubscribingNode::display()
{
    int i = 0;
    for (NOMADSUtil::String *pPath = _pPathList->getFirst();
         pPath != NULL; pPath = _pPathList->getNext()) {
        
        printf (" Path %d = %s\t", ++i, pPath->c_str());
    }
}

