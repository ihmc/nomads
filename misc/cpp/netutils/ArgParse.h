/*
 * CongestionClassifiers.h
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on November 13, 2015, 3:14 PM
 */

#ifndef INCL_ARGUMENT_PARSER_H
#define	INCL_ARGUMENT_PARSER_H

#include "FTypes.h"

namespace IHMC_MISC
{
    struct Arguments
    {
        public:
            Arguments (bool bPrntData, bool bUseSeqNum, bool bUseRelTime,
                       uint16 ui16PktLen, uint32 ui32TransmInterval,
                       uint32 ui32SrcAddr, uint16 ui16SrcPort,
                       uint32 ui32DstAddr, uint16 ui16DstPort);
            Arguments (const Arguments &rhsArgs);
            ~Arguments (void);

            const bool _bPrntData;
            const bool _bUseSeqNum;
            const bool _bUseRelTime;
            const uint16 _ui16PktLen;
            const uint16 _ui16SrcPort;
            const uint16 _ui16DstPort;
            const uint32 _ui32TransmInterval;
            const uint32 _ui32SrcAddr;
            const uint32 _ui32DstAddr;
    };

    class ArgParse
    {
        public:
            static Arguments parseSenderArgs (int argc, const char *argv[]);
            static Arguments parseReceiverArgs (int argc, const char *argv[]);

        private:
            static Arguments parse (int argc, const char *argv[], bool bSender);
    };
}

#endif    // INCL_ARGUMENT_PARSER_H

