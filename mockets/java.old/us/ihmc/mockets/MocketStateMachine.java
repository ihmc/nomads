package us.ihmc.mockets;

/**
 * Models the finite state machine for a mocket connection.
 */
public class MocketStateMachine
{
    public MocketStateMachine()
    {
        _state = State.CLOSED;
    }

    public void passiveOpen()
    {
        if (_state != State.CLOSED) {
            throw new StateException ("passiveOpen only possible in CLOSED state; current state is " + currentStateAsString());
        }
        _state = State.LISTEN;
    }

    public void activeOpen()
    {
        if (_state != State.CLOSED) {
            throw new StateException ("activeOpen only possible in CLOSED state; current state is " + currentStateAsString());
        }
        _state = State.SYN_SENT;
    }

    public void synReceived()
    {
        if (_state != State.LISTEN) {
            throw new StateException ("synReceived only possible in LISTEN state; current state is " + currentStateAsString());
        }
        _state = State.SYN_RCVD;
    }

    public void synAckReceived()
    {
        if ((_state != State.SYN_SENT) && (_state != State.SYN_RCVD)) {
            throw new StateException ("synAckReceived only possible in SYN_SENT or SYN_RCVD state; current state is " +
                                      currentStateAsString());
        }
        _state = State.ESTABLISHED;
    }

    public void finReceived()
    {
        if (_state == State.ESTABLISHED) {
            _state = State.CLOSE_WAIT;
        }
        else if (_state == State.FIN_WAIT_1) {
            _state = State.TIME_WAIT;
        }
        else if (_state == State.FIN_WAIT_2) {
            _state = State.TIME_WAIT;
        }
        else {
            throw new StateException ("finReceived only possible in ESTABLISHED or FIN_WAIT_2 state; current state is " +
                                      currentStateAsString());
        }
    }

    public void close()
    {
        if (_state == State.ESTABLISHED) {
            _state = State.FIN_WAIT_1;
        }
        else if (_state == State.CLOSE_WAIT) {
            _state = State.LAST_ACK;
        }
        else {
            throw new StateException ("close only possible in ESTABLISHED or CLOSE_WAIT state; current state is " +
                                      currentStateAsString());
        }
    }

    public void finAckReceived()
    {
        if (_state == State.LAST_ACK) {
            _state = State.CLOSED;
        }
        else if (_state == State.FIN_WAIT_1) {
            _state = State.FIN_WAIT_2;
        }
        else {
            throw new StateException ("finAckReceived only possible in LAST_ACK or FIN_WAIT_1 state; current state is " +
                                      currentStateAsString());
        }
    }

    public int currentState()
    {
        return _state;
    }

    public String currentStateAsString()
    {
        switch (_state) {
            case State.UNDEFINED:
                return "UNDEFINED";
            case State.CLOSED:
                return "CLOSED";
            case State.LISTEN:
                return "LISTEN";
            case State.SYN_RCVD:
                return "SYN_RCVD";
            case State.SYN_SENT:
                return "SYN_SENT";
            case State.ESTABLISHED:
                return "ESTABLISHED";
            case State.CLOSE_WAIT:
                return "CLOSE_WAIT";
            case State.LAST_ACK:
                return "LAST_ACK";
            case State.FIN_WAIT_1:
                return "FIN_WAIT_1";
            case State.FIN_WAIT_2:
                return "FIN_WAIT_2";
            case State.CLOSING:
                return "CLOSING";
            case State.TIME_WAIT:
                return "TIME_WAIT";
            default:
                return "UNKNOWN";
        }
    }

    public class State
    {
        public static final int UNDEFINED = 0;
        public static final int CLOSED = 1;
        public static final int LISTEN = 2;
        public static final int SYN_RCVD = 3;
        public static final int SYN_SENT = 4;
        public static final int ESTABLISHED = 5;
        public static final int CLOSE_WAIT = 6;
        public static final int LAST_ACK = 7;
        public static final int FIN_WAIT_1 = 8;
        public static final int FIN_WAIT_2 = 9;
        public static final int CLOSING = 10;
        public static final int TIME_WAIT = 11;

    }

    public class StateException extends RuntimeException
    {
        public StateException()
        {
        }

        public StateException (String msg)
        {
            super (msg);
        }
    }

    private int _state;
}
