#ifndef _MANAGED_MOCKET_STATS_H
#define _MANAGED_MOCKET_STATS_H

#include <Mocket.h>

namespace us {
    namespace ihmc {
        namespace mockets {
            public ref class ManagedMocketStats
            {
                public:
                    ManagedMocketStats (MocketStats *realStats);
                    !ManagedMocketStats();
                    virtual ~ManagedMocketStats();

                    uint32 getRetransmitCount (void);
                    uint32 getSentPacketCount (void);
                    uint32 getSentByteCount (void);
                    uint32 getReceivedPacketCount (void);
                    uint32 getReceivedByteCount (void);
                    uint32 getDuplicatedDiscardedPacketCount (void);
                    uint32 getNoRoomDiscardedPacketCount (void);

                private:
                    MocketStats *_pRealStats;
            }; // ref class ManagedMocketStats
        } //namespace mockets
    }//namespace ihmc
}//namespace us

#endif //_MANAGED_STATS_H