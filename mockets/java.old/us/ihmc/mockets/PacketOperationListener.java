package us.ihmc.mockets;

interface PacketOperationListener
{
    /* returns true if the packet was processed, false if it was 
     * simply ignored because already present */
    boolean packetProcessed (long tsn);
}

/*
 * vim: et ts=4 sw=4
 */

