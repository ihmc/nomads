/*
 * Utils.java
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
 */

package us.ihmc.aci.disServiceProxy;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class Utils
{
    private static String ID_SEPARATOR = ":" ;

    public static String normalizeMessageID (String msgId) throws Exception
    {
        if (msgId == null) {
            throw new Exception ("The message ID is null, it can not be normalized.");
        }
        String[] tokens = msgId.split (":");
        if (tokens.length < 3) {
            throw new Exception ("Message ID is in an uncomplete format, it can not be normalized.");
        }
        return tokens[0] + ID_SEPARATOR + tokens[1] + ID_SEPARATOR + tokens[2];
    }

    public static String getChunkMessageID (String sender, String groupName, int seqNum) throws Exception
    {
        if (sender == null || groupName == null) {
           throw new Exception ("The message ID is null, the message ID can not be built.");
        }
        if (seqNum < 0) {
            throw new Exception ("Negative sequence ID not supported, the message ID can not be built.");
        }
        String chunkedMsgGrpName = groupName;
        if (!groupName.endsWith("[od]")) {
            chunkedMsgGrpName = chunkedMsgGrpName + ".[od]";
        }
        return chunkedMsgGrpName + ID_SEPARATOR + sender + ID_SEPARATOR + seqNum;
    }

    public static String getMessageID (String sender, String groupName, int seqNum) throws Exception
    {
        if (sender == null || groupName == null) {
           throw new Exception ("The message ID is null, the message ID can not be built.");
        }
        if (seqNum < 0) {
            throw new Exception ("Negative sequence ID not supported, the message ID can not be built.");
        }
        return groupName + ID_SEPARATOR + sender + ID_SEPARATOR + seqNum;
    }
}
