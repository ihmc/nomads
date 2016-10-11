package us.ihmc.netutils;

import us.ihmc.netutils.protocol.Protocol;

/**
 * Connection.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class CommDetails
{
    public static class Builder
    {
        public Builder (String remoteHost, Protocol protocol, int port)
        {
            this._remoteHost = remoteHost;
            this._protocol = protocol;
            this._port = port;
        }

        public Builder configFile (String value)
        {

            this._configFile = value;
            return this;
        }

        public Builder tcpControlPort (int value)
        {

            this._tcpControlPort = value;
            return this;
        }

        public Builder mode (Mode mode)
        {
            this._mode = mode;
            return this;
        }

        public Builder msgSize (int value)
        {

            this._msgSize = value;
            return this;
        }

        public Builder interval (int value)
        {

            this._interval = value;
            return this;
        }

        public CommDetails build ()
        {
            return new CommDetails(this);
        }

        //required parameters
        private final String _remoteHost;
        private final Protocol _protocol;
        private final int _port;
        //optional parameters - initialized to default values
        private String _configFile = Client.DEFAULT_MOCKETS_CONFIG_FILE;
        private int _tcpControlPort = Server.DEFAULT_CTRL_PORT;
        private Mode _mode = Client.DEFAULT_MODE;
        private int _msgSize = Client.DEFAULT_MSG_SIZE_INTERVAL;
        private int _interval = Client.DEFAULT_INTERVAL;
    }

    private CommDetails (Builder builder)
    {
        this._remoteHost = builder._remoteHost;
        this._protocol = builder._protocol;
        this._port = builder._port;

        this._configFile = builder._configFile;
        this._tcpControlPort = builder._tcpControlPort;
        this._mode = builder._mode;
        this._msgSize = builder._msgSize;
        this._interval = builder._interval;
    }

    public String getRemoteHost ()
    {
        return _remoteHost;
    }

    public Protocol getProtocol ()
    {
        return _protocol;
    }

    public int getPort ()
    {
        return _port;
    }

    public int getTcpControlPort ()
    {
        return _tcpControlPort;
    }

    public String getConfigFile ()
    {
        return _configFile;
    }

    public Mode getMode ()
    {
        return _mode;
    }

    public int getMsgSize ()
    {
        return _msgSize;
    }

    public int getInterval ()
    {
        return _interval;
    }

    @Override
    public String toString ()
    {
        return "Comm. " + _protocol + " to remote host " + _remoteHost + ":" + _port + " mode " + _mode
                + " msg. size " + _msgSize + (_mode.equals(Mode.Interval) ? " interval " + _interval : "")
                + (_protocol.type.equals(Protocol.Type.UDP) ? " TCP control port: " + getTcpControlPort() : "")
                + "(optional conf. file: " + _configFile + ")";
    }

    private final String _remoteHost;
    private final Protocol _protocol;
    private final int _port;
    private final String _configFile;
    private final int _tcpControlPort;
    private final Mode _mode;
    private final int _msgSize;
    private final int _interval;
}
