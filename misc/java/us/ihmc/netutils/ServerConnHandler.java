package us.ihmc.netutils;

import org.apache.log4j.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;

import java.util.NoSuchElementException;

/**
 * ServerConnHandler.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ServerConnHandler implements Runnable
{
    ServerConnHandler (final Server server, final CommHelperAdapter commHelper)
    {
        _server = server;
        _commHelper = commHelper;
    }

    void start ()
    {
        (new Thread(this)).start();
    }

    @Override
    public void run ()
    {
        LOG.info(String.format("Thread started"));
        int i32request;
        Request request = null;
        String requestStr;

        try {
            i32request = _commHelper.read32();
            RequestType requestType = RequestType.fromI32code(i32request);

            switch (requestType) {
                case register:
                    _commHelper.sendLine(RequestType.confirm.toString());
                    requestStr = _commHelper.receiveLine();
                    request = Request.parse(requestStr);
                    doRegister(_commHelper, request.getClientId(), request.getClientName());
                    break;
                case registerCallback:
                    _commHelper.sendLine(RequestType.confirm.toString());
                    requestStr = _commHelper.receiveLine();
                    request = Request.parse(requestStr);
                    doRegisterCallback(_commHelper, request.getClientId());
                    break;
                case streamSize:
                    doStreamServerChild(_commHelper, i32request);
                    break;
                default:
                    throw new ProtocolException(String.format("Expected %s or %s but received %s",
                            RequestType.register.toString(), RequestType.registerCallback.toString(),
                            requestType.toString()));
            }
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_server.getPort()), e);
            _commHelper.closeConnection();
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError());
            _commHelper.closeConnection();
        }
    }

    void doStreamServerChild (final CommHelperAdapter commHelper, int streamSize)
    {

        ServerChild serverChild = new ServerChild((short) 0, "StreamServerChild", streamSize, commHelper,
                _server); //new server child
        _server.getChildren().put((short) 0, serverChild);
        serverChild.start();
    }

    void doRegister (final CommHelperAdapter commHelper, short clientId, String clientName)
    {
        while (_server.getChildren().containsKey(clientId)) {
            if (_server.getPort() == Server.DEFAULT_CTRL_PORT) {
                //assume it has to be 0 to couple with UDP
                break;
            }
            clientId++;
        }

        ServerChild serverChild = new ServerChild(clientId, clientName, commHelper, _server); //new server child
        _server.getChildren().put(clientId, serverChild);

        try {
            commHelper.sendLine(String.format("%s %s", RequestType.confirm.toString(), clientId));
            LOG.debug("Registered connection with clientId: " + clientId + " and clientName: " + clientName);
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_server.getPort()), e);
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError());
        }

        serverChild.start();
    }

    void doRegisterCallback (final CommHelperAdapter commHelper, final short clientId)
    {
        ServerChild serverChild = _server.getChildren().get(clientId);

        try {
            if (serverChild == null) {
                String error = String.format("%s: client with id %s not found", RequestType.error.toString(), clientId);
                LOG.error(error);
                commHelper.sendLine(error);
                commHelper.closeConnection();
                return;
            }

            serverChild.setCallbackCommHelper(commHelper);
            commHelper.sendLine(RequestType.confirm.toString());
            LOG.debug("Registered callback connection with clientId: " + clientId);
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_server.getPort()), e);
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError());
        }
    }

    private final Server _server;
    private final CommHelperAdapter _commHelper;
    private static final Logger LOG = Logger.getLogger(ServerConnHandler.class);
}
