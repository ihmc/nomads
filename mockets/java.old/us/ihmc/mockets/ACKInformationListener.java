package us.ihmc.mockets;

interface ACKInformationListener
{
    boolean processPacket (long tsn, boolean control, boolean sequenced);
    int processPacketsRange (long begin, long end, boolean control, boolean sequenced);
}
/*
 * vim: et ts=4 sw=4
 */

