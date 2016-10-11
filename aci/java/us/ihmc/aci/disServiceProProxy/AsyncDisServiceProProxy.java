package us.ihmc.aci.disServiceProProxy;

import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxy;
import us.ihmc.aci.disServiceProxy.PeerStatusListener;
import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class AsyncDisServiceProProxy extends QueuedDisServiceProProxy implements Runnable
{
    public static final long DEFAULT_POLLING_TIME = 5000;

    private long _pollingTime;

    public AsyncDisServiceProProxy()
    {
        this (DEFAULT_POLLING_TIME);
    }

    public AsyncDisServiceProProxy (long pollingTime)
    {
        super();
        _pollingTime = pollingTime;
    }

    public AsyncDisServiceProProxy (short applicationId)
    {
        this (applicationId, DEFAULT_POLLING_TIME);
    }

    public AsyncDisServiceProProxy (short applicationId, long pollingTime)
    {
        super (applicationId);
        _pollingTime = pollingTime;
    }

    public AsyncDisServiceProProxy (short applicationId, long reinitializationAttemptInterval, long pollingTime)
    {
        super (applicationId, reinitializationAttemptInterval);
        _pollingTime = pollingTime;
    }

    public AsyncDisServiceProProxy (short applicationId, String host, int iPort)
    {
    	this (applicationId, host, iPort, DEFAULT_POLLING_TIME);
    }

    public AsyncDisServiceProProxy (short applicationId, String host, int iPort, long pollingTime)
    {
    	super (applicationId, host, iPort);
        _pollingTime = pollingTime;
    }

    public AsyncDisServiceProProxy (short applicationId, String host, int port,
                                    long reinitializationAttemptInterval, long pollingTime)
    {
        super (applicationId, host, port, reinitializationAttemptInterval);
        _pollingTime = pollingTime;
    }

    public void run()
    {
        while (true) {
            boolean atLeastOne = false;

            QueuedArrivedData ad = getArrivedData();
            if (ad != null) {
                DisseminationServiceProxy.dataArrived (_listeners, ad.msgId, ad.sender, ad.groupName, ad.seqNum,
                                                       ad.objectId, ad.instanceId, ad.mimeType, ad.data,
                                                       ad.metadataLength, ad.tag, ad.priority,
                                                       ad.queryId);
                atLeastOne = true;
            }

            QueuedArrivedChunk ac = getArrivedChunk();
            if (ac != null) {
                DisseminationServiceProxy.chunkArrived (_listeners, ac.msgId, ac.sender, ac.groupName,
                                                        ac.seqNum, ac.objectId, ac.instanceId,
                                                        ac.mimeType, ac.data, ac.nChunks,
                                                        ac.totNChunks, ac.chunhkedMsgId,
                                                        ac.tag, ac.priority,
                                                        ac.queryId);
                atLeastOne = true;
            }

            QueuedArrivedMetadata am = getArrivedMetadata();
            if (am != null) {                
                DisseminationServiceProxy.metadataArrived (_listeners, am.msgId, am.sender, am.groupName, am.seqNum,
                                                           am.objectId, am.instanceId, am.mimeType,
                                                           am.data, am.chunkedData, am.tag,
                                                           am.priority, am.queryId);
                atLeastOne = true;
            }

            QueuedAvailableData vd = getAvailableData();
            if (vd != null) {                
                DisseminationServiceProxy.dataAvailable (_listeners, vd.msgId, vd.sender, vd.groupName, vd.seqNum,
                                                         vd.objectId, vd.instanceId, vd.mimeType,
                                                         vd.refObjId, vd.data, vd.tag, vd.priority,
                                                         vd.queryId);
                atLeastOne = true;
            }

            PeerListUpdate update = getPeerListUpdate();
            if (update != null) {
                for (String peerId : update._newPeersList) {
                    DisseminationServiceProxy.newPeer (_peerStatusListeners, peerId);
                    atLeastOne = true;
                }
                for (String peerId : update._deadPeersList) {
                    DisseminationServiceProxy.deadPeer (_peerStatusListeners, peerId);
                    atLeastOne = true;
                }
            }

            DSProProxyEvent dsproProxyEvent = getDSProProxyEvents();
            if (dsproProxyEvent != null) {
                if (dsproProxyEvent instanceof RegisteredNewPath) {
                    RegisteredNewPath rp = (RegisteredNewPath) dsproProxyEvent;
                    DisServiceProProxy.pathRegistered (_listeners, rp._path, rp._nodeId,
                                                       rp._team, rp._mission);
                }
                else if (dsproProxyEvent instanceof UpdatedPosition) {
                    UpdatedPosition up = (UpdatedPosition) dsproProxyEvent;
                    DisServiceProProxy.positionUpdated (_listeners, up._latitude, up._longitude,
                                                        up._altitude, up._nodeId);
                }
                atLeastOne = true;
            }

            if (!atLeastOne) {
                try {
                    Thread.sleep (_pollingTime);
                }
                catch (InterruptedException ex) {
                    Logger.getLogger(AsyncDisServiceProProxy.class.getName()).log(Level.WARNING, null, ex);
                }
            }
        }
    }

    @Override
    public void registerDisServiceProProxyListener (DisServiceProProxyListener listener)
        throws CommException
    {
        if (listener == null) {
            _LOGGER.severe ("Error:DisseminationServiceProxyListener is null");
            return;
        }
        _listeners.add (listener);
    }

    @Override
    public void registerPeerStatusListener (PeerStatusListener listener)
    {
        if (listener == null) {
            _LOGGER.severe ("Error:PeerStatusListener is null");
            return;
        }
        _peerStatusListeners.add (listener);
    }

    private final List<DisServiceProProxyListener> _listeners = new ArrayList<DisServiceProProxyListener>();
    private final List<PeerStatusListener> _peerStatusListeners = new ArrayList<PeerStatusListener>();
    private static final Logger _LOGGER = Logger.getLogger (AsyncDisServiceProProxy.class.getName());
}
