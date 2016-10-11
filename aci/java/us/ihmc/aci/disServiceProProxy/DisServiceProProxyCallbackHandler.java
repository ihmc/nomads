package us.ihmc.aci.disServiceProProxy;

import java.util.logging.Logger;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxyCallbackHandler;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.StringUtil;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class DisServiceProProxyCallbackHandler extends DisseminationServiceProxyCallbackHandler
{

    public DisServiceProProxyCallbackHandler (DisServiceProProxy proxy, CommHelper commHelper)
    {
        super (proxy, commHelper);
        _proxy = proxy;
    }

    @Override
    public void run()
    {
        setCallbackThreadId (getId());

        while (true) {
            try {
                String[] callbackArray = _commHelper.receiveParsed();

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
                else {
                    _LOGGER.warning (String.format ("ERROR: operation [%s] unknown.", callbackArray[0]));
                }
            }
            catch (Exception e) {
                _LOGGER.severe (StringUtil.getStackTraceAsString (e));
                _proxy.notifyConnectionLoss();
                return; // Terminate the thread
            }
        }
    }

    private void doPathRegisteredCallback()
    {
        _LOGGER.info ("DisseminationServiceProxyCallbackHandler:doPathRegisteredCallback method");
        try {
            NodePath path = NodePath.read(_commHelper);

            byte[] b = _commHelper.receiveBlock();
            String nodeId = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String mission = b != null ? new String (b) : "";

            b = _commHelper.receiveBlock();
            String team = b != null ? new String (b) : "";

            _proxy.pathRegistered (path, nodeId, mission, team);

            _commHelper.sendLine ("OK");
        }
        catch (CommException ce) {
            _LOGGER.severe (StringUtil.getStackTraceAsString (ce));
        }
    }

    private void doPositionUpdatedCallback()
    {
        _LOGGER.info ("DisseminationServiceProxyCallbackHandler:doPositionUpdatedCallback method");
        try {
            float fLatitude =  Float.intBitsToFloat (_commHelper.readI32());
            float fLongitude = Float.intBitsToFloat (_commHelper.readI32());
            float fAltitude =  Float.intBitsToFloat (_commHelper.readI32());

            byte[] b = _commHelper.receiveBlock();
            String nodeId = b != null ? new String (b) : "";

            if (nodeId == null || nodeId.length() == 0) {
                _commHelper.sendLine ("Error");
            }
            else {
                _proxy.positionUpdated (fLatitude, fLongitude, fAltitude, nodeId);
                _commHelper.sendLine ("OK");
            }
        }
        catch (Exception e) {
            _LOGGER.severe (StringUtil.getStackTraceAsString (e));
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

            _proxy.informationMatched (localNodeID, peerNodeID, matchedObjectID, matchedObjectName,
                                       rankDescriptors, partialRanks, weights, comment, operation);

            _commHelper.sendLine ("OK");
        }
        catch (CommException ce) {
            _LOGGER.severe (StringUtil.getStackTraceAsString (ce));
        }
        catch (Exception ce) {
            try {
                _commHelper.sendLine ("ERROR");
            } catch (CommException ex) {}
            _LOGGER.severe (StringUtil.getStackTraceAsString (ce));
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

            _proxy.informationSkipped (localNodeID, peerNodeID, skippedObjectID, skippedObjectName,
                                       rankDescriptors, partialRanks, weights, comment, operation);

            _commHelper.sendLine ("OK");
        }
        catch (CommException ce) {
            System.err.println (StringUtil.getStackTraceAsString (ce));
        }
        catch (Exception ce) {
            try {
                _commHelper.sendLine ("ERROR");
            } catch (CommException ex) {}
            _LOGGER.severe (StringUtil.getStackTraceAsString (ce));
        }
    }

    private DisServiceProProxy _proxy;
    private static final Logger _LOGGER = Logger.getLogger (DisServiceProProxyCallbackHandler.class.getName());
}
