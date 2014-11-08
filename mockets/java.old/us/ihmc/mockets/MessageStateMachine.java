/**
 * Class MessageStateMachine models the finite state machine for a MessageMocket 
 * connection.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
    
class MessageStateMachine
{
    MessageStateMachine()
    {
        _state = CLOSED;

        _logger = Logger.getLogger ("us.ihmc.mockets");
    }
    
    MessageStateMachine (int state)
    {
        assert isValidState (state);
        _state = state;

        _logger = Logger.getLogger ("us.ihmc.mockets");
    }

    void receivedInit()
    {
        if (_state != CLOSED) {
            throw new StateException ("receivedInit only possible in CLOSED state; " +
                                      "current state is " + currentStateAsString());
        }            
        // _logger.info ("MessageStateMachine::receivedInit");
    }
    
    void abort()
    {
        /* any initial state is valid */
        _state = CLOSED;
    }

    void associate()
    {
        if (_state != CLOSED) {
            throw new StateException ("associate only possible in CLOSED state; " +
                                      "current state is " + currentStateAsString());
        }            
        _state = COOKIE_WAIT;
        // _logger.info ("MessageStateMachine::associate");
    }

    
    void receivedCookieEcho()
    {
        if (_state != CLOSED) {
            throw new StateException ("receivedCookieEcho only possible in CLOSED state; " +
                                      "current state is " + currentStateAsString());
        }            
        _state = ESTABLISHED;
        // _logger.info ("MessageStateMachine::receivedCookieEcho");
    }
    
    void receivedInitAck()
    {
        if (_state != COOKIE_WAIT) {
            throw new StateException ("receivedInitAck only possible in COOKIE_WAIT state; " +
                                      "current state is " + currentStateAsString());
        }            
        _state = COOKIE_ECHOED;
        // _logger.info ("MessageStateMachine::receivedInitAck");
    }
    
    void receivedCookieAck()
    {
        if (_state != COOKIE_ECHOED) {
            throw new StateException ("receivedCookieAck only possible in COOKIE_ECHOED state; " +
                                      "current state is " + currentStateAsString());
        }            
        _state = ESTABLISHED;
        // _logger.info ("MessageStateMachine::receivedCookieAck");
    }

    void shutdown()
    {
        switch (_state) {
            case ESTABLISHED:
                _state = SHUTDOWN_PENDING;
                break;
            default:
                // do nothing, just log a message
                _logger.warning ("MessageStateMachine::shutdown in state " + currentStateAsString());
        }
        //_logger.info ("MessageStateMachine::shutdown");
    }
    
    void receivedShutdown()    
    {
        _logger.warning ("MessageStateMachine::receivedShutdown in state " + currentStateAsString());
        switch (_state) {
            case ESTABLISHED:
            case SHUTDOWN_PENDING:
                _state = SHUTDOWN_RECEIVED;
                break;
            case SHUTDOWN_SENT:
                _state = SHUTDOWN_ACK_SENT;
                break;
            default:
                // do nothing, just log a message
                _logger.warning ("MessageStateMachine::receivedShutdown in state " + currentStateAsString());
        }
        // _logger.info ("MessageStateMachine::receivedShutdown");
    }
    
    void outstandingQueueFlushed()
    {
        switch (_state) {
            case SHUTDOWN_PENDING:
                _state = SHUTDOWN_SENT;
                break;
            case SHUTDOWN_RECEIVED:
                _state = SHUTDOWN_ACK_SENT;
                break;
            default:
                // do nothing, just log a message
                _logger.warning ("MessageStateMachine::outstandingQueueFlushed in state " + currentStateAsString());
        }
        // _logger.info ("MessageStateMachine::outstandingQueueFlushed");
    }

    void receivedShutdownAck()
    {
        //_logger.warning ("MessageStateMachine::receivedShutdownAck in state " + currentStateAsString());
        switch (_state) {
            case SHUTDOWN_SENT:
                _state = CLOSED;
            case SHUTDOWN_ACK_SENT:
                // do nothing
                break;
            default:
                // do nothing, just log a message
                _logger.warning ("MessageStateMachine::receivedShutdownAck in state " + currentStateAsString());
        }
        // _logger.info ("MessageStateMachine::receivedShutdownAck");
    }
    
    void receivedShutdownComplete()
    {
        //_logger.warning ("MessageStateMachine::receivedShutdownComplete in state " + currentStateAsString());
        switch (_state) {
            case SHUTDOWN_SENT:
            case SHUTDOWN_ACK_SENT:
                _state = CLOSED;
                break;
            default:
                // do nothing, just log a message
                _logger.warning ("MessageStateMachine::receivedShutdownComplete in state " + currentStateAsString());
        }
        // _logger.info ("MessageStateMachine::receivedShutdownComplete");
    }
    
    void close()
    {
        //_logger.warning ("MessageStateMachine::close in state " + currentStateAsString());
        _state = CLOSED;
    }
    
    int currentState()
    {
        return _state;
    }

    String currentStateAsString()
    {
        return stateAsString (_state);
    }
        
    static String stateAsString (int state)
    {
        switch (state) {
            case UNDEFINED:
                return "UNDEFINED";
            case CLOSED:
                return "CLOSED";
            case COOKIE_WAIT:
                return "COOKIE_WAIT";
            case COOKIE_ECHOED:
                return "COOKIE_ECHOED";
            case ESTABLISHED:
                return "ESTABLISHED";
            case SHUTDOWN_PENDING:
                return "SHUTDOWN_PENDING";
            case SHUTDOWN_SENT:
                return "SHUTDOWN_SENT";
            case SHUTDOWN_RECEIVED:
                return "SHUTDOWN_RECEIVED";
            case SHUTDOWN_ACK_SENT:
                return "SHUTDOWN_ACK_SENT";
            default:
                return "UNKNOWN";
        }
    }

    static boolean isValidState (int state)
    {
        boolean ret = false;
        
        switch (state) {
            case UNDEFINED:
            case CLOSED:
            case COOKIE_WAIT:
            case COOKIE_ECHOED:
            case ESTABLISHED:
            case SHUTDOWN_PENDING:
            case SHUTDOWN_SENT:
            case SHUTDOWN_RECEIVED:
            case SHUTDOWN_ACK_SENT:
                ret = true;
        }
        
        return ret;
    }

    static final int UNDEFINED = 0;
    static final int CLOSED = 1;
    static final int COOKIE_WAIT = 2;
    static final int COOKIE_ECHOED = 3;
    static final int ESTABLISHED = 4;
    static final int SHUTDOWN_PENDING = 5;
    static final int SHUTDOWN_SENT = 6;
    static final int SHUTDOWN_RECEIVED = 7;
    static final int SHUTDOWN_ACK_SENT = 8;

    private int _state;
    private Logger _logger;
}
/*
 * vim: et ts=4 sw=4
 */

