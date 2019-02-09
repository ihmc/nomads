/*
 * CallbackHandler.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

package us.ihmc.nms;

import us.ihmc.comm.CommHelper;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.StringUtil;
import us.ihmc.util.proxy.Stub;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class CallbackHandler extends us.ihmc.util.proxy.CallbackHandler
{
    private final Logger LOGGER = LoggerFactory.getLogger(CallbackHandler.class);
    private boolean _terminate = false;

    CallbackHandler (Stub stub, CommHelper commHelper)
    {
        super (stub, commHelper);
    }

    @Override
    public void run() {
        Thread.currentThread().setName ("CallbackHandler");
        //setCallbackThreadId();

        while (!_terminate) {
            try {
                String[] callbackArray = _commHelper.receiveParsed();
                if (callbackArray[0].compareToIgnoreCase ("messageArrived") == 0) {
                    doMessageArrived();
                }

            } catch (Exception ex) {
                try { Thread.sleep(1000); } catch (Exception e) {}
            }
        }
    }

    public void requestTermination() {
        _terminate = true;
    }

    private void doMessageArrived() throws CommException, ProtocolException {
        LOGGER.trace("metadataArrived");
        try {
            String incomingInterface = _commHelper.receiveString();
            long srcIP = _commHelper.readUI32();
            String sSrcIP = String.format("%d.%d.%d.%d",
                    (srcIP & 0xff),
                    (srcIP >> 8 & 0xff),
                    (srcIP >> 16 & 0xff),
                    (srcIP >> 24 & 0xff));
            byte msgType = _commHelper.read8();
            short msgId = _commHelper.read16();
            byte hopCount = _commHelper.read8();
            byte ttl = _commHelper.read8();
            boolean isUnicast = _commHelper.read8() == 1;
            byte[] metadata = null;
            short metadataLen = _commHelper.read16();
            if (metadataLen > 0) {
                metadata = new byte[metadataLen];
                _commHelper.receiveBlob(metadata, 0, metadataLen);
            }
            byte[] data = null;
            short dataLen = _commHelper.read16();
            if (dataLen > 0) {
                data = new byte[dataLen];
                _commHelper.receiveBlob(data, 0, dataLen);
            }
            long timestamp = _commHelper.readI64();
            long groupMsgCount = _commHelper.read64();
            long unicastMsgCount = _commHelper.read64();

            if (_stub instanceof NMSProxy) {
                NMSProxy proxy = (NMSProxy) _stub;
                proxy.messageArrived(incomingInterface, sSrcIP, msgType, msgId, hopCount, ttl, isUnicast,
                        metadata, data, timestamp, groupMsgCount, unicastMsgCount);
            }
        }
        catch (Exception e) {
            if (e instanceof CommException || e instanceof ProtocolException) {
                throw e;     
            }
            else {
                if (LOGGER.isDebugEnabled()) {
                    LOGGER.debug(StringUtil.getStackTraceAsString(e));
                }
                else {
                    LOGGER.warn(e.getMessage());
                }
            }
        }
    }
}
