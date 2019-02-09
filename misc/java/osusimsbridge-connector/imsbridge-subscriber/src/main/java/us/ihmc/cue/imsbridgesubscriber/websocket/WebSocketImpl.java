package us.ihmc.cue.imsbridgesubscriber.websocket;


import org.java_websocket.client.WebSocketClient;
import org.java_websocket.drafts.Draft;
import org.java_websocket.handshake.ServerHandshake;

import java.net.URI;
import java.util.function.Consumer;

/**
 * The implementation of the {@link WebSocketClient} class for connecting to a remote endpoint. The {@link URI} in the
 * constructor is used for the connection. Callbacks are used for whenever some vents occur in the websocket.
 */
public class WebSocketImpl extends WebSocketClient
{
    public WebSocketImpl (URI serverUri, Draft protocolDraft) {
        super(serverUri, protocolDraft);
    }

    /**
     * Sets a {@link Runnable} for when the socket is closed. A runnable is used instead of a {@link Consumer} because the
     * <code>onClose</code> function has multiple parameters
     * @param runnable Runnable for when <code>onClose</code> is called
     * @return Return the instance of the class
     */
    public WebSocketImpl setOnClose(Consumer<Boolean> runnable){
        _onClose = runnable;
        return this;
    }

    /**
     * Sets a {@link Consumer} for when a message is received.
     * @param handler The {@link Consumer} for handling strings.
     * @return Return the instance of the class
     */
    public WebSocketImpl setOnMessage(Consumer<String> handler){
        _onMessage = handler;
        return this;
    }

    /**
     * Sets a {@link Consumer} for when an error occurs in the websocket
     * @param exceptionConsumer The {@link Consumer} for handling {@link Exception}
     * @return Return the instance of the class
     */
    public WebSocketImpl setOnError(Consumer<Exception> exceptionConsumer){
        _onError = exceptionConsumer;
        return this;
    }


    /**
     * Sets a {@link Consumer} for when the socket is first opened
     * @param onOpen The {@link Consumer} for the {@link ServerHandshake}
     * @return Return the instance of the class
     */
    public WebSocketImpl setOnOpen(Consumer<ServerHandshake> onOpen){
        _onOpen = onOpen;
        return this;
    }

    /**
     * The <code>onOpen</code> function is called when the websocket connects
     * @param handshakedata {@link ServerHandshake}
     */
    @Override
    public void onOpen (ServerHandshake handshakedata) {
        _onOpen.accept(handshakedata);
    }

    /**
     * The <code>onMessage</code> is called when a message is received from the remote endpoint.
     * @param message
     */
    @Override
    public void onMessage (String message) {
        _onMessage.accept(message);
    }


    /**
     * The <code>onClose</code> is called when the websocket closes from either end.
     * @param code The HTTP code
     * @param reason The string reason for the code
     * @param remote Returns whether the remote initiated the close or not
     */
    @Override
    public void onClose (int code, String reason, boolean remote) {
        _onClose.accept(remote);
    }

    /**
     * The <code>onError</code> is called when the websocket throws an error
     * @param ex {@link Exception}
     */
    @Override
    public void onError (Exception ex) {
        _onError.accept(ex);
    }

    private Consumer<String> _onMessage;
    private Consumer<ServerHandshake> _onOpen;
    private Consumer<Exception> _onError;
    private Consumer<Boolean> _onClose;



}

