/*
 * Utils.cs
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

﻿using System;
using System.Collections.Generic;
using System.Text;

namespace us.ihmc.aci.DisService
{
    public class Utils
    {
        private static String ID_SEPARATOR = ":" ;

        public static string normalizeMessageID (String msgId)
        {
            if (msgId == null) {
                throw new Exception ("The message ID is null, it can not be normalized.");
            }
            string[] tokens = msgId.Split (':');
            if (tokens.Length < 3) {
                throw new Exception ("Message ID is in an uncomplete format, it can not be normalized.");
            }
            return tokens[0] + ID_SEPARATOR + tokens[1] + ID_SEPARATOR + tokens[2];
        }

        public static string getChunkMessageID (String sender, String groupName, uint seqNum)
        {
            if (sender == null || groupName == null) {
               throw new Exception ("The message ID is null, the message ID can not be built.");
            }
            String chunkedMsgGrpName = groupName;
            if (!groupName.EndsWith("[od]")) {
                chunkedMsgGrpName = chunkedMsgGrpName + ".[od]";
            }
            return chunkedMsgGrpName + ID_SEPARATOR + sender + ID_SEPARATOR + seqNum;
        }

        public static string getMessageID (String sender, String groupName, uint seqNum)
        {
            if (sender == null || groupName == null) {
               throw new Exception ("The message ID is null, the message ID can not be built.");
            }
            return groupName + ID_SEPARATOR + sender + ID_SEPARATOR + seqNum;
        }
    }
}
