/*
 * DSProProxyCallbackHandler.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import java.util.Collection;
import java.util.LinkedList;
import us.ihmc.aci.dspro2.util.LoggerInterface;

import us.ihmc.aci.dspro2.util.LoggerWrapper;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.sync.ConcurrentProxyCallbackHandler;
import us.ihmc.util.StringUtil;

/**
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 * @author  Maggie Breedy <mbreedy@ihmc.us>
 * @version $Revision: 1.34 $
 */
public class DSProProxyCallbackHandler extends ConcurrentProxyCallbackHandler
{
    public DSProProxyCallbackHandler (DSProProxy proxy, CommHelper commHelper)
    {
        super (proxy);
        _commHelper = commHelper;
        _proxy = proxy;
    }

    @Override
    public void run()
    {
        Thread.currentThread().setName ("DSProProxyCallbackHandler");
        setCallbackThreadId();

        while (true) {
            try {
                String[] callbackArray = _commHelper.receiveParsed();

                if (callbackArray[0].equals ("dataArrivedCallback")) {
                    doDataArrivedCallback();
                }
                else if (callbackArray[0].equals ("metadataArrivedCallback")) {
                    doMetadataArrivedCallback();
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
                else if (callbackArray[0].equals ("searchReplyArrivedCallback")) {
                    doSearchReplyArrivedCallback();
                }
                else if (callbackArray[0].equals ("volatileSearchReplyArrivedCallback")) {
                    doVolatileSearchReplyArrivedCallback();
                }
                else if (callbackArray[0].equals ("pathRegisteredCallback")) {
                    doPathRegisteredCallback();
                }
                else if (callbackArray[0].equals ("positionUpdatedCallback")) {
                    doPositionUpdatedCallback();
                }
                else if (callbackArray[0].equals ("informationMatchedCallback")) {
                    doInformationMatchedCallback();
                }
                else if (callbackArray[0].equals ("informationSkippedCallback")) {
                    doInformationSkippedCallback();
                }

                else if (callbackArray[0].equals ("contextUpdateMessageArrivedCallback")) {
                    doContextUpdateMessageArrived();
                }
                else if (callbackArray[0].equals ("contextVersionMessageArrivedCallback")){
                    doContextVersionMessageArrived();
                }
                else if (callbackArray[0].equals ("messageRequestMessageArrivedCallback")) {
                    doMessageRequestMessageArrived();
                }
                else if (callbackArray[0].equals ("chunkRequestMessageArrivedCallback")) {
                    doChunkRequestMessageArrived();
                }
                else if (callbackArray[0].equals ("positionMessageArrivedCallback")) {
                    doPositionMessageArrived();
                }
                else if (callbackArray[0].equals ("searchMessageArrivedCallback")) {
                    doSearchMessageArrived();
                }
                else if (callbackArray[0].equals ("topologyReplyMessageArrivedCallback")) {
                    doTopologyReplyMessageArrived();
                }
                else if (callbackArray[0].equals ("topologyRequestMessageArrivedCallback")) {
                    doTopologyRequestMessageArrived();
                }
                else if (callbackArray[0].equals ("updateMessageArrivedCallback")) {
                    doUpdateMessageArrived();
                }
                else if (callbackArray[0].equals ("versionMessageArrivedCallback")) {
                    doVersionMessageArrived();
                }
                else if (callbackArray[0].equals ("waypointMessageArrivedCallback")) {
                    doWaypointMessageArrived();
                }
                else if (callbackArray[0].equals ("wholeMessageArrivedCallback")) {
                    doWholeMessageArrived();
                }

                else {
                    LOG.warn ("ERROR: operation [" + callbackArray[0] + "] unknown.");
                }
            }
            catch (CommException ce) {
                _proxy.notifyConnectionLoss();
                LOG.warn (StringUtil.getStackTraceAsString (ce));
                return; // Terminate the thread
            }
            catch (Exception ce) {
                LOG.warn (StringUtil.getStackTraceAsString (ce));
            }
        }
    }

    private void doPathRegisteredCallback()
    {
        LOG.info ("DSProProxyCallbackHandler::doPathRegisteredCallback method!");
        try {
            NodePath path = NodePath.read(_commHelper);

            byte[] b = _commHelper.receiveBlock();
            String nodeId = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String mission = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String team = b != null ? new String (b) : "";

            _commHelper.sendLine ("OK");

            _proxy.pathRegistered (path, nodeId, mission, team);
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
        catch (Exception ce) {
            try {
                _commHelper.sendLine ("ERROR");
            } catch (CommException ex) {}
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    private void doPositionUpdatedCallback()
    {
        LOG.info ("DSProProxyCallbackHandler::doPositionUpdatedCallback method!");
        try {
            float fLatitude =  Float.intBitsToFloat (_commHelper.readI32());
            float fLongitude = Float.intBitsToFloat (_commHelper.readI32());
            float fAltitude =  Float.intBitsToFloat (_commHelper.readI32());

            byte[] b = _commHelper.receiveBlock();
            String nodeId = b != null ? new String (b) : "";

            if (nodeId == null || nodeId.length() == 0) {
                _commHelper.sendLine ("ERROR");
            }
            else {
                _commHelper.sendLine ("OK");
                _proxy.positionUpdated (fLatitude, fLongitude, fAltitude, nodeId);
            }
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
        catch (Exception ce) {
            try {
                _commHelper.sendLine ("ERROR");
            } catch (CommException ex) {}
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    private void doInformationMatchedCallback()
    {
        try {
            byte[] b = _commHelper.receiveBlock();
            String localNodeID = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String peerNodeID = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String matchedObjectID = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String matchedObjectName = b != null ? new String (b) : "";

            byte len = _commHelper.read8();
            String[] rankDescriptors = null;
            float[] partialRanks = null;
            float[] weights = null;
            if (len > 0) {
                rankDescriptors = new String[len];
                partialRanks = new float[len];
                weights = new float[len];
                for (byte i = 0; i < len; i++) {
                    b = _commHelper.receiveBlock();
                    rankDescriptors[i] = b != null ? new String (b) : "";

                    partialRanks[i] = Float.intBitsToFloat(_commHelper.readI32());
                    weights[i] = Float.intBitsToFloat(_commHelper.readI32());
                }
            }

            b = _commHelper.receiveBlock();
            String comment  = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String operation  = b != null ? new String (b) : "";

            _commHelper.sendLine ("OK");

            _proxy.informationMatched (localNodeID, peerNodeID, matchedObjectID, matchedObjectName,
                                       rankDescriptors, partialRanks, weights, comment, operation);
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
        catch (Exception ce) {
            try {
                _commHelper.sendLine ("ERROR");
            } catch (CommException ex) {}
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    private void doInformationSkippedCallback()
    {
        try {
            byte[] b = _commHelper.receiveBlock();
            String localNodeID = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String peerNodeID = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String skippedObjectID = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String skippedObjectName = b != null ? new String (b) : "";

            byte len = _commHelper.read8();
            String[] rankDescriptors = null;
            float[] partialRanks = null;
            float[] weights = null;
            if (len > 0) {
                rankDescriptors = new String[len];
                partialRanks = new float[len];
                weights = new float[len];
                for (byte i = 0; i < len; i++) {
                    b = _commHelper.receiveBlock();
                    rankDescriptors[i] = b != null ? new String (b) : "";

                    partialRanks[i] = Float.intBitsToFloat(_commHelper.readI32());
                    weights[i] = Float.intBitsToFloat(_commHelper.readI32());
                }
            }

            b = _commHelper.receiveBlock();
            String comment  = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String operation  = b != null ? new String (b) : "";

            _commHelper.sendLine ("OK");

            _proxy.informationSkipped (localNodeID, peerNodeID, skippedObjectID, skippedObjectName,
                                       rankDescriptors, partialRanks, weights, comment, operation);
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
        catch (Exception ce) {
            try {
                _commHelper.sendLine ("ERROR");
            } catch (CommException ex) {}
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    private void doContextUpdateMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.contextUpdateMessageArrived (sender, publisher);
    }

    private void doContextVersionMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.contextVersionMessageArrived (sender, publisher);
        
    }

    private void doMessageRequestMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.messageRequestMessageArrived (sender, publisher);
    }

    private void doChunkRequestMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.chunkRequestMessageArrived (sender, publisher);
    }

    private void doPositionMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.positionMessageArrived (sender, publisher);
    }

    private void doSearchMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.searchMessageArrived (sender, publisher);
    }

    private void doTopologyReplyMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.topologyReplyMessageArrived (sender, publisher);
    }

    private void doTopologyRequestMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.topologyRequestMessageArrived (sender, publisher);
    }

    private void doUpdateMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.updateMessageArrived (sender, publisher);
    }

    private void doVersionMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.versionMessageArrived (sender, publisher);
    }

    private void doWaypointMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.waypointMessageArrived (sender, publisher);
    }

    private void doWholeMessageArrived()
        throws CommException
    {
        byte[] b = _commHelper.receiveBlock();
        String sender = b != null ? new String (b) : "";
        b = _commHelper.receiveBlock();
        String publisher = b != null ? new String (b) : "";
        _proxy.wholeMessageArrived (sender, publisher);
    }

    private void doDataArrivedCallback()
    {
        LOG.info ("DSProProxyCallbackHandler::doDataArrivedCallback method!");
        try {
            byte[] b = _commHelper.receiveBlock();
            if (b == null) {
                return;
            }
            String id = new String (b);

            b = _commHelper.receiveBlock();
            String groupname = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String objectId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String instanceId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String annotatedObjMsgId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String mimeType = b != null ? new String (b) : null;

            int dataLength = _commHelper.read32();
            byte[] data = new byte [dataLength];
            _commHelper.receiveBlob (data, 0, dataLength);

            short chunkNumber = _commHelper.read8();
            short totChunksNumber = _commHelper.read8();

            b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            _commHelper.sendLine ("OK");

            _proxy.dataArrived (id, groupname, objectId, instanceId, annotatedObjMsgId,
                                mimeType, data, chunkNumber, totChunksNumber, queryId);
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
        catch (Exception ce) {
            try {
                _commHelper.sendLine ("ERROR");
            } catch (CommException ex) {}
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    private void doMetadataArrivedCallback()
    {
        LOG.info ("DSProProxyCallbackHandler::doMetadataArrivedCallback method!");
        System.out.println ("NOMADS:DSProProxyCallbackHandler: metadataArrived");
        
        try {
            byte[] b = _commHelper.receiveBlock();
            if (b == null) {
                return;
            }
            String id = new String (b);

            b = _commHelper.receiveBlock();
            String groupname = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String objectId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String instanceId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String XMLMetadata = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String referredDataId = b != null ? new String (b) : null;

            b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            _commHelper.sendLine ("OK");

            System.out.println ("NOMADS:DSProProxyCallbackHandler: metadataArrived. Calling _proxy");
            _proxy.metadataArrived (id, groupname, objectId, instanceId,
                                    XMLMetadata, referredDataId, queryId);
            
            System.out.println ("NOMADS:DSProProxyCallbackHandler: metadataArrived. Called _proxy");
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
        catch (Exception ce) {
            try {
                _commHelper.sendLine ("ERROR");
            } catch (CommException ex) {}
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    private void doNewPeerCallback()
    {
        LOG.info ("DSProProxyCallbackHandler::doNewPeerCallback method!");
        try {
            byte[] b = _commHelper.receiveBlock();
            String peerID = b != null ? new String (b) : "";

            _commHelper.sendLine ("OK");
            _proxy.newNeighbor (peerID);
        }
        catch (Exception ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    private void doDeadPeerCallback()
    {
        LOG.info ("DSProProxyCallbackHandler::doDeadPeerCallback method!");
        try {
            byte[] b = _commHelper.receiveBlock();
            String peerID = b != null ? new String (b) : "";

            _commHelper.sendLine ("OK");
            _proxy.deadNeighbor (peerID);
        }
        catch (Exception ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    public void doSearchArrivedCallback()
        throws ProtocolException
    {
        LOG.info ("DSProProxyCallbackHandler::doSearchArrivedCallback method!");
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

            int queryLen = _commHelper.read32();
            byte[] query = _commHelper.receiveBlob (queryLen);

            _proxy.searchArrived (queryId, groupName, querier, queryType, queryQualifiers, query);
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    public void doSearchReplyArrivedCallback()
        throws ProtocolException
    {
        LOG.info ("DSProProxyCallbackHandler::doSearchReplyArrivedCallback (1) method!");
        try {
            byte[] b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            Collection<String> matchingNodeIds = new LinkedList<String>();
            byte[] matchingMsgId = _commHelper.receiveBlock();
            while (matchingMsgId != null) {
                matchingNodeIds.add (new String (matchingMsgId));
                matchingMsgId = _commHelper.receiveBlock();
            }

            b = _commHelper.receiveBlock();
            String responderNodeId = b != null ? new String (b) : null;

            _proxy.searchReplyArrived (queryId, matchingNodeIds, responderNodeId);
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
        
    }

    public void doVolatileSearchReplyArrivedCallback()
        throws ProtocolException
    {
        LOG.info ("DSProProxyCallbackHandler::doSearchReplyArrivedCallback (2) method!");
        try {
            byte[] b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;

            byte[] reply = _commHelper.receiveBlock();

            b = _commHelper.receiveBlock();
            String responderNodeId = b != null ? new String (b) : null;

            _proxy.searchReplyArrived (queryId, reply, responderNodeId);
        }
        catch (CommException ce) {
            LOG.warn (StringUtil.getStackTraceAsString (ce));
        }
    }

    private DSProProxy _proxy;
    private CommHelper _commHelper;
    private final static LoggerInterface LOG = LoggerWrapper.getLogger (DSProProxyCallbackHandler.class);
}
