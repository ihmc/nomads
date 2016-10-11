package us.ihmc.netutils;

import org.apache.log4j.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * ClientCallbackHandler.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ClientCallbackHandler implements Runnable
{
    ClientCallbackHandler (Client client, CommHelperAdapter commHelper)
    {
        _client = client;
        _commHelper = commHelper;
    }

    void start ()
    {
        (new Thread(this, "ClientCallbackThread-" + _client.getId())).start();
    }

    public void closeConnection ()
    {
        if (_commHelper != null) _commHelper.closeConnection();
    }

    @Override
    public void run ()
    {
        while (!STOP.get()) {
            String[] requestCallBacks = null;
            try {
                requestCallBacks = _commHelper.receiveParsed();
                RequestType requestType = RequestType.valueOf(requestCallBacks[0]);

                switch (requestType) {
                    case onMessage:
                        doOnMessageCallback();
                        break;
                    case onStats:
                        doOnStatsCallback();
                        break;
                }
            }
            catch (CommException e) {
                LOG.error(ConnErrorHandler.getConnectionError(), e);
                _client.setConnected(false);
                break; //terminate
            }
            catch (IllegalArgumentException e) {
                LOG.error(ConnErrorHandler.getProtocolError("Illegal request"));
                _client.setConnected(false);
                break; //terminate
            }
        }
    }

    private void doOnMessageCallback ()
    {
        try {
            int dataLength = _commHelper.read32();
            byte[] message = _commHelper.receiveBlob(dataLength);
            _client.onMessage(_client.getId(), message); //send to listeners
            _client.stats.msgReceived.incrementAndGet();
            // _commHelper.sendLine(Request.confirm.toString());
            LOG.info(RequestType.onMessage + " callback successful");
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(), e);
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getConnectionError(), e);
        }
    }


    private void doOnStatsCallback ()
    {
        try {
            int msgSent = _commHelper.read32();
            int msgReceived = _commHelper.read32();
            int throughput = _commHelper.read32();

            Stats stats = new Stats(Stats.Type.Server);
            stats.msgSent.set(msgSent);
            stats.msgReceived.set(msgReceived);
            stats.throughputReceived.set(throughput);

            _client.onStats(_client.getId(), stats); //send to listeners
            //_commHelper.sendLine(Request.confirm.toString());
            LOG.info(RequestType.onStats + " callback successful");
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(), e);
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getConnectionError(), e);
        }
    }

    private final Client _client;
    private final CommHelperAdapter _commHelper;
    private final static Logger LOG = Logger.getLogger(ClientCallbackHandler.class);

    final AtomicBoolean STOP = new AtomicBoolean(false);
}
