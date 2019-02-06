
#include "ICMPRTTList.h"
#include <forward_list>

using namespace NOMADSUtil;

namespace IHMC_NETSENSOR
{
    ICMPRTTList::ICMPRTTList(uint32 ui32Resolution)
        : _avgRttTIA(ui32Resolution)
    {
    }

    ICMPRTTList::~ICMPRTTList(void)
    {
    }

    uint32 ICMPRTTList::clean(const uint32 ui32CleaningNumber)
    {
        //uint32 containersToDelete[C_MAX_CLEANING_NUMBER];
        bool bKeepCleaning = true;
        std::forward_list <uint32> list;
        if ((_mutex.lock() == NOMADSUtil::Mutex::RC_Ok))
        {
            int64 currentTime = getTimeInMilliseconds();
            uint32 ui32CleaningCounter = 0;

            for (UInt32Hashtable<RTTContainer>::Iterator i = _rttTable.getAllElements(); !i.end();
                i.nextElement())
            {
                RTTContainer *pContainer = i.getValue();

                int64 expiredTime = pContainer->getLastChangeTime() + C_ENTRY_TIME_VALIDITY;
                if (currentTime > expiredTime) {
                    //containersToDelete[ui32CleaningCounter] = i.getKey();
                    list.push_front (i.getKey());
                    ui32CleaningCounter++;
                    bKeepCleaning = (ui32CleaningCounter < ui32CleaningNumber);
                }
            }

            for (auto iter = list.begin(); iter != list.end(); iter++) {
                _rttTable.remove (*iter);
            }
            /*for (uint32 counter = 0; counter < ui32CleaningCounter; counter++) {
                _rttTable.remove(containersToDelete[counter]);
            }*/

            _mutex.unlock();
        }
        return 0;
    }

    uint32 ICMPRTTList::getCount(void)
    {
        if ((_mutex.lock() == NOMADSUtil::Mutex::RC_Ok))
        {
            uint32 _ui32Count = 0;

            for (UInt32Hashtable<RTTContainer>::Iterator i = _rttTable.getAllElements(); !i.end();
                i.nextElement())
            {
                RTTContainer *pContainer = i.getValue();
                if (pContainer->getRcvTime() != 0) {
                    _ui32Count++;
                }
            }

            _mutex.unlock();
            return _ui32Count;
        }
    }

    uint32 ICMPRTTList::getMinRTT(void)
    {
        return _avgRttTIA.getMin();
    }

    uint32 ICMPRTTList::getMostRecentRTT(void)
    {
        if ((_mutex.lock() == NOMADSUtil::Mutex::RC_Ok))
        {
            uint32 mostRecentRTT = 0;
            for (UInt32Hashtable<RTTContainer>::Iterator i = _rttTable.getAllElements(); !i.end();
                i.nextElement())
            {
                RTTContainer *pContainer = i.getValue();
                uint32 currRTT = pContainer->getRTT();
                if (pContainer->getRcvTime() != 0) {
                    mostRecentRTT = currRTT;
                }
            }
            _mutex.unlock();
            return mostRecentRTT;
        }

        return 0;
    }

    float ICMPRTTList::getAverageRTT(void)
    {
        if (_avgRttTIA.getNumValues() == 0) {
            return 0.0;
        }
        return _avgRttTIA.getSum() / _avgRttTIA.getNumValues();
    }

    uint32 ICMPRTTList::getMaxRTT(void)
    {
        if ((_mutex.lock() == NOMADSUtil::Mutex::RC_Ok))
        {
            uint32 maxRTT = 0;
            for (UInt32Hashtable<RTTContainer>::Iterator i = _rttTable.getAllElements(); !i.end();
                i.nextElement())
            {
                RTTContainer *pContainer = i.getValue();
                uint32 currRTT = pContainer->getRTT();
                if (currRTT > maxRTT && pContainer->getRcvTime() != 0) {
                    maxRTT = currRTT;
                }
            }
            _mutex.unlock();
            return maxRTT;
        }

        return 0;
    }

    void ICMPRTTList::print(void)
    {
        if ((_mutex.lock() == NOMADSUtil::Mutex::RC_Ok))
        {

            int size = _rttTable.getCount();
            if (size > 0) {
                printf("Captured values: \n");
                printf("\t\t\tSENT\t\t\tRECEIVED\t\tRTT\n");
                printf("\t\t\t----\t\t\t--------\t\t---\n");

                for (UInt32Hashtable<RTTContainer>::Iterator i = _rttTable.getAllElements(); !i.end();
                    i.nextElement())
                {
                    RTTContainer *pContainer = i.getValue();
                    uint32 currVal = pContainer->getRTT();

                    // Don't print if there's no received time
                    if (pContainer->getRcvTime() > 0) {
                        printf("\t\t\t%lld", pContainer->getSentTime());
                        printf("\t\t%lld", pContainer->getRcvTime());
                        printf("\t\t%d\n", currVal);
                    }
                }

                printf("\n\t\t\t\t\t\t\t\t\tAverage: %f\n\n", _avgRttTIA.getAverage());
            }

            _mutex.unlock();
        }
    }

    void ICMPRTTList::addSentTime (uint32 seqNum, int64 time)
    {
        if ((_mutex.lock() == NOMADSUtil::Mutex::RC_Ok))
        {
            RTTContainer *newContainer = new RTTContainer();
            _rttTable.put(seqNum, newContainer);

            newContainer->setSentTime(time);
            newContainer->updateChangeTime(time);

            _mutex.unlock();
        }
    }

    void ICMPRTTList::addReceivedTime (uint32 seqNum, int64 time)
    {
        if ((_mutex.lock() == NOMADSUtil::Mutex::RC_Ok))
        {
            RTTContainer *pContainer = _rttTable.get(seqNum);
            if (pContainer == nullptr) {
                printf("ICMPRTTLister::SetReceivedTime::No sent time found for seq num: %d\n", seqNum);
                _mutex.unlock();
                return;
            }

            pContainer->setReceivedTime(time);
            pContainer->updateChangeTime(time);

            _avgRttTIA.add(pContainer->getRTT());

            _mutex.unlock();
        }
    }
}