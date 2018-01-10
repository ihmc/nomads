#pragma once
#include "FTypes.h"
#include"StrClass.h"

#include "InterfaceMonitor.h"

namespace IHMC_NETSENSOR
{

    class NetSensorPacket
    {
    public:
        NetSensorPacket();
        NetSensorPacket(const NetSensorPacket &p);

        ~NetSensorPacket();
        //<------------------------------------------------------------------------------------------------------------------------>
        int64 getTime(void);
        NetSensorPacket & operator = (const NetSensorPacket &p);

    private:
        friend void InterfaceMonitor::run(void);
        friend class HandlerThread;

        uint8               ui8Buf[9038U];
        int64               int64RcvTimeStamp;
        int                 classification;
        uint32              received;
        NOMADSUtil::String  sMonitoredInterface;
    };

    static const NetSensorPacket EMPTY_PACKET{ };


    inline NetSensorPacket::NetSensorPacket() : int64RcvTimeStamp(0), classification(0), received(0), sMonitoredInterface("") { }
    inline NetSensorPacket::NetSensorPacket(const NetSensorPacket &p) :
        int64RcvTimeStamp(p.int64RcvTimeStamp), classification(p.classification), received(p.received),
        sMonitoredInterface(p.sMonitoredInterface)
    {
        memcpy (ui8Buf, p.ui8Buf, p.received);
    }

    inline NetSensorPacket::~NetSensorPacket(void) { }

    inline NetSensorPacket & NetSensorPacket::operator = (const NetSensorPacket &p)
    {
        int64RcvTimeStamp = p.int64RcvTimeStamp;
        classification = p.classification;
        received = p.received;
        sMonitoredInterface = p.sMonitoredInterface;
        memcpy (ui8Buf, p.ui8Buf, p.received);

        return (*this);
    }

    inline int64 NetSensorPacket::getTime(void)
    {
        return int64RcvTimeStamp;
    }
}
