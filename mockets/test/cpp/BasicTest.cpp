#include <stdio.h>

#include "Packet.h"
#include "PacketAccessors.h"
#include "PacketMutators.h"

int main (int argc, char *argv[])
{
    Packet packet(1500);
    SAckChunkMutator mutator = packet.addSAckChunk (1, 2, 3);
    mutator.selectReliableSequencedFlow();
    mutator.startAddingRanges();
    mutator.addRange (12, 14);
    mutator.addRange (18, 25);
    mutator.addRange (27, 28);
    mutator.doneAddingRanges();
    mutator.startAddingTSNs();
    mutator.addTSN (16);
    mutator.addTSN (32);
    mutator.doneAddingTSNs();
    const char *pBuf = packet.getPacket();
    unsigned short usSize = packet.getPacketSize();
    for (unsigned short us = 0; us < usSize; us++) {
        printf ("%03d ", (int) pBuf[us]);
    }
    printf ("\n");
    Packet packet2 ((char*) pBuf, usSize);
    packet2.resetChunkIterator();
    unsigned short usChunkType = packet2.getChunkType();
    printf ("first chunk type = %d\n", (int) usChunkType);
    if (usChunkType == Packet::CT_SAck) {
        SAckChunkAccessor accessor = packet2.getSAckChunk();
        while (accessor.haveMoreBlocks()) {
            while (accessor.haveMoreElements()) {
                uint8 ui8Type = accessor.getBlockType();
                switch (ui8Type) {
                    case SAckChunkAccessor::BT_RANGE_CONTROL:
                        printf ("control range - %lu, %lu\n", accessor.getStartTSN(), accessor.getEndTSN());
                        break;
                    case SAckChunkAccessor::BT_RANGE_RELIABLE_SEQUENCED:
                        printf ("reliable sequenced range - %lu, %lu\n", accessor.getStartTSN(), accessor.getEndTSN());
                        break;
                    case SAckChunkAccessor::BT_RANGE_RELIABLE_UNSEQUENCED:
                        printf ("reliable unsequenced range - %lu, %lu\n", accessor.getStartTSN(), accessor.getEndTSN());
                        break;
                    case SAckChunkAccessor::BT_SINGLE_CONTROL:
                        printf ("control single - %lu\n", accessor.getTSN());
                        break;
                    case SAckChunkAccessor::BT_SINGLE_RELIABLE_SEQUENCED:
                        printf ("reliable sequenced single - %lu\n", accessor.getTSN());
                        break;
                    case SAckChunkAccessor::BT_SINGLE_RELIABLE_UNSEQUENCED:
                        printf ("reliable unsequenced single - %lu\n", accessor.getTSN());
                        break;
                }
                accessor.advanceToNextElement();
            }
            accessor.advanceToNextBlock();
        }
    }
    return 0;
}
