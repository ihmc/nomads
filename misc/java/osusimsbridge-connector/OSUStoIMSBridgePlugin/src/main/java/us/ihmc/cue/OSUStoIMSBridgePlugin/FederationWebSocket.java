package us.ihmc.cue.OSUStoIMSBridgePlugin;

import mil.dod.th.core.log.Logging;
import org.java_websocket.client.WebSocketClient;
import org.java_websocket.drafts.Draft_6455;
import org.java_websocket.handshake.ServerHandshake;
import org.osgi.service.log.LogService;

import java.net.URI;

public class FederationWebSocket extends WebSocketClient
{
    public FederationWebSocket (URI serverUri, String nameMessage,
                                String subsMessage, StringHandler handler, int timeout) {
        super(serverUri, new Draft_6455(), null, timeout);

        _nameMessage = nameMessage;
        _subsMessage = subsMessage;

        _stringHandler = handler;
    }
    @Override
    public void onOpen (ServerHandshake handshakedata) {
        // send message to websocket
        send(_nameMessage);
        send(_subsMessage);
    }

    @Override
    public void onMessage (String message) {
        _stringHandler.handleString(message);
    }

    @Override
    public void onClose (int code, String reason, boolean remote) {
        if(reason != null && !reason.isEmpty()) {
            Logging.log(LogService.LOG_INFO, this.getClass().getName() + "::Connection close for reason: " + reason);
        }
    }

    @Override
    public void onError (Exception ex) {
        if (ex.getMessage() != null) {
            Logging.log(LogService.LOG_ERROR, ex.getMessage());
        }
    }

    private String _nameMessage;
    private String _subsMessage;
    private StringHandler _stringHandler;
}
