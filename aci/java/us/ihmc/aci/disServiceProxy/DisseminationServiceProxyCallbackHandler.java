/*
 * DisseminationServiceProxyCallbackHandler.java
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

import java.util.logging.Logger;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.ManageableThread;
import us.ihmc.util.StringUtil;

/**
 * DisseminationServiceProxyCallbackHandler
 *
 * Date: Jun 2, 2008
 * Time: 10:22:35 AM
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision: 1.41 $
 */

public class DisseminationServiceProxyCallbackHandler extends ManageableThread
{
    public DisseminationServiceProxyCallbackHandler (DisseminationServiceProxy proxy, CommHelper commHelper)
    {
        if (proxy == null || commHelper == null) {
            _logger.severe ("Error: The proxy or the commHelper is null");
            return;
        }

        _commHelper = commHelper;
        _proxy = proxy;
    }

    @Override
    public void run()
    {
        setCallbackThreadId (getId());

        while (true) {
            try {
                String[] callbackArray = _commHelper.receiveParsed();
                _logger.info (String.format ("DisseminationServiceProxyCallbackHandler:callbackArray [0]: %s", callbackArray[0]));
                if (callbackArray[0].equals ("dataArrivedCallback")) {
                    doDataArrivedCallback();
                }
                else if (callbackArray[0].equals ("chunkArrivedCallback")) {
                    doChunkArrivedCallback();
                }
                else if (callbackArray[0].equals ("metadataArrivedCallback")) {
                    doMetadataArrivedCallback();
                }
                else if (callbackArray[0].equals ("dataAvailableCallback")) {
                    doDataAvailableCallback();
                }
                else if (callbackArray[0].equals ("newPeerCallback")) {
                    doNewPeerCallback();
                }
                else if (callbackArray[0].equals ("deadPeerCallback")) {
                    doDeadPeerCallback();
                }
                else if (callbackArray[0].equals ("searchArrivedCallback")) {
                    doSearchArrivedCallback();
                }
                else {
                    _logger.warning (String.format ("ERROR: operation %s unknown.", callbackArray[0]));
                }
            }
            catch (Exception ce) {
                _logger.severe (StringUtil.getStackTraceAsString (ce));
                _proxy.notifyConnectionLoss();
                return; // Terminate the thread
            }
        }
    }

    protected void doDataArrivedCallback()
    {
        try {
            String sender = _commHelper.receiveLine();
            _logger.info (String.format ("DisseminationServiceProxyCallbackHandler:doDataArrivedCallback: sender: %s", sender));
            String groupname = _commHelper.receiveLine();
            int seqNum = _commHelper.read32();

            byte[] b = _commHelper.receiveBlock();
            String objectId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String instanceId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String mimeType = b != null ? new String (b) : null;

            int dataLength = _commHelper.read32();
            int metadataLength = _commHelper.read32();
            byte[] data = new byte [dataLength];
            _commHelper.receiveBlob (data, 0, dataLength);

            short tag = _commHelper.read16();
            byte priority = _commHelper.read8();

            b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            try {
                _proxy.dataArrived (Utils.getMessageID(sender, groupname, seqNum), sender,
                                    groupname, seqNum, objectId, instanceId, mimeType, data,
                                    metadataLength, tag, priority, queryId);
            }
            catch (Exception e) {
                _logger.severe (StringUtil.getStackTraceAsString (e));
            }

            _commHelper.sendLine ("OK");
        }
        catch (CommException ce) {
            _logger.severe (StringUtil.getStackTraceAsString (ce));
        }
        catch (ProtocolException pe) {
            _logger.severe (StringUtil.getStackTraceAsString (pe));
        }
    }

    protected void doChunkArrivedCallback()
    {
        try {
            String sender = _commHelper.receiveLine();
            _logger.info (String.format ("DisseminationServiceProxyCallbackHandler:doChunkArrivedCallback: sender: %s", sender));
            String groupname = _commHelper.receiveLine();
            int seqNum = _commHelper.read32();

            byte[] b = _commHelper.receiveBlock();
            String objectId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String instanceId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String mimeType = b != null ? new String (b) : null;

            int dataLength = _commHelper.read32();
            byte[] data = new byte [dataLength];
            _commHelper.receiveBlob (data, 0, dataLength);
            short nChunks = _commHelper.read8();
            short totNChunks = _commHelper.read8();
            String chunkedMsgId = _commHelper.receiveLine();
            short tag = _commHelper.read16();

            byte priority = _commHelper.read8();

            b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            try {
            _proxy.chunkArrived (Utils.getChunkMessageID(sender, groupname, seqNum), sender, groupname,
                                 seqNum, objectId, instanceId, mimeType, data, nChunks, totNChunks,
                                 chunkedMsgId, tag, priority, queryId);
            }
            catch (Exception e) {
                 _logger.severe (StringUtil.getStackTraceAsString (e));
            }

            _commHelper.sendLine ("OK");
        }
        catch (CommException ce) {
            _logger.severe (StringUtil.getStackTraceAsString (ce));
        }
        catch (ProtocolException pe) {
            _logger.severe (StringUtil.getStackTraceAsString (pe));
        }
    }

    protected void doMetadataArrivedCallback()
    {
        try {
            String sender = _commHelper.receiveLine();
            _logger.info (String.format ("DisseminationServiceProxyCallbackHandler:doMetadataArrivedCallback: sender: %s", sender));
            String groupname = _commHelper.receiveLine();
            int seqNum = _commHelper.read32();

            byte[] b = _commHelper.receiveBlock();
            String objectId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String instanceId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String mimeType = b != null ? new String (b) : null;

            int dataLength = _commHelper.read32();
            byte[] data = new byte [dataLength];
            _commHelper.receiveBlob (data, 0, dataLength);
            byte tmp = _commHelper.read8();
            boolean dataChunk = (tmp == 1);
            short tag = _commHelper.read16();
            byte priority = _commHelper.read8();

            b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            try {
                _proxy.metadataArrived (Utils.getMessageID(sender, groupname, seqNum),
                                        sender, groupname, seqNum, objectId, instanceId,
                                        mimeType, data, dataChunk, tag, priority, queryId);
            }
            catch (Exception e) {
                 _logger.severe (StringUtil.getStackTraceAsString (e));
            }

            _commHelper.sendLine ("OK");
        }
        catch (CommException ce) {
            _logger.severe (StringUtil.getStackTraceAsString (ce));
        }
        catch (ProtocolException pe) {
            _logger.severe (StringUtil.getStackTraceAsString (pe));
        }
    }

    protected void doDataAvailableCallback()
    {
        try {
            String sender = _commHelper.receiveLine();
            _logger.info (String.format ("DisseminationServiceProxyCallbackHandler:doDataAvailableCallback: sender: %s", sender));
            String groupname = _commHelper.receiveLine();
            int seqNum = _commHelper.read32();

            byte[] b = _commHelper.receiveBlock();
            String objectId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String instanceId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String mimeType = b != null ? new String (b) : null;

            int len = _commHelper.read32();
            String id = (len > 0 ? new String (_commHelper.receiveBlob(len)) : null);

            int dataLength = _commHelper.read32();
            byte[] data = new byte [dataLength];
            _commHelper.receiveBlob (data, 0, dataLength);
            short tag = _commHelper.read16();
            byte priority = _commHelper.read8();

            b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            try {
                _proxy.dataAvailable (Utils.getMessageID(sender, groupname, seqNum),
                                      sender, groupname, seqNum, objectId, instanceId,
                                      mimeType, id, data,tag, priority, queryId);
            }
            catch (Exception e) {
                 _logger.severe (StringUtil.getStackTraceAsString (e));
            }
            _commHelper.sendLine ("OK");

        }
        catch (CommException ce) {
            _logger.severe (StringUtil.getStackTraceAsString (ce));
        }
        catch (ProtocolException pe) {
            _logger.severe (StringUtil.getStackTraceAsString (pe));
        }
    }

    public void doNewPeerCallback()
    {
        try {
            String peerID = new String (_commHelper.receiveBlock());
            _logger.info (String.format ("DisseminationServiceProxyCallbackHandler:doNewPeerCallback: %s", peerID));
            _proxy.newPeer(peerID);

            _commHelper.sendLine ("OK");

        }
        catch (CommException ce) {
            _logger.severe (StringUtil.getStackTraceAsString (ce));
        }
    }

    public void doDeadPeerCallback()
    {
        
        try {
            String peerID = new String (_commHelper.receiveBlock());
            _logger.info (String.format ("DisseminationServiceProxyCallbackHandler:doDeadPeerCallback: %s", peerID));
            _proxy.deadPeer(peerID);

            _commHelper.sendLine ("OK");

        }
        catch (CommException ce) {
            _logger.severe (StringUtil.getStackTraceAsString (ce));
        }
    }

    public void doSearchArrivedCallback()
        throws ProtocolException
    {
        try {
            byte[] b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String groupName = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String querier = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String queryType = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String queryQualifiers = b != null ? new String (b) : null;

            byte[] query = null;
            int queryLen = _commHelper.read32();
            if (queryLen > 0) {
                query = new byte[queryLen];
                _commHelper.receiveBlob (queryLen);
            }

            _proxy.searchArrived (queryId, groupName, querier, queryType, queryQualifiers, query);

            _commHelper.sendLine ("OK");
        }
        catch (CommException ce) {
            _logger.severe (StringUtil.getStackTraceAsString (ce));
        }
    }

    protected void setCallbackThreadId (long callbackTheadId)
    {
        _proxy.setCallbackThreadId (callbackTheadId);
    }

    protected CommHelper _commHelper;
    private DisseminationServiceProxy _proxy;
    private static final Logger _logger = Logger.getLogger (DisseminationServiceProxyCallbackHandler.class.getName());
}
