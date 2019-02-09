package us.ihmc.aci.envMonitor.provider.sim;

import java.io.Serializable;

/**
 * TestConfiguration
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 * @ $Date$
 * @ Created on Apr 22, 2005 at 10:20:52 AM
 */
public class TestConfiguration implements Serializable
{
    public int MANET_GATEWAY_BUFFER_QUEUE_SIZE = 60;
    public int MANET_SOCKET_BUFFER_HANDLER_QUEUE_SIZE = 30;
    public int RREQ_RREQ_MIN_DELAY = 500;
    public int VALID_ROUTE_LIFETIME = 4000;  //ms
    public int ACTIVE_ROUTE_LIFETIME = 6000;  //ms
    public int ECHO_CACHE_TIMEOUT = 2000;
    public int SEQUENCE_NUMBER_THRESHOLD = 10;

    public int MTU = 1400;                // Maximum transmission unit for packet payload
    public int BUFFERING_TIME = 100;      // Default maximum buffering time before transmitting data (in milliseconds)
    public int KEEPALIVE_TIME = 1000;     // Maximum inactivity time after which an empty packet is transmitted
    public int RECEIVE_TIMEOUT = 1200;    // Timeout placed on the underlying datagram socket
    public int SYN_RESEND_TIMEOUT = 250;  // Timeout to resend a SYN packet (in milliseconds)
    public int CONNECT_TIMEOUT = 9000;    // Total default time that this mocket will try to connect to the other endpoint
    public int CONNECT_RETRIES = 10;      // Number of retransmissions of the SYN packet before doubling the SYN_RESEND_TIMEOUT (connect(...) method).
    public int ACK_TIMEOUT = 250;         // Timeout to retransmit a packet (in milliseconds)
	public int FIN_ACK_TIMEOUT = 3000;    // Timeout to wait for the FINACK
    public int CLOSE_WAIT_TIME = 2000;    // Time to wait to respond to FIN requests when doing a close
	public int MAX_PACKET_SIZE = 2048;    // Max packet size (must be larger than MTU + Header size)
    public int RCVR_BUFFER_SIZE = 49152;  // Receiver buffer size (in bytes)
    public int SLIDING_WINDOW_SIZE = 32;  // transmitter sliding window size (in packets - should be less than RCVR_BUFFER_SIZE / MTU)
	public int PACKET_TRANSMIT_INTERVAL = 10; //Time in millis that the transmitter will wait between trasmitting one packet and the next.
    public int SYN_HISTORY_SIZE = 64;
    public int SYN_VALIDITY_WINDOW = 10000;

    public String toString()
    {
        return "TestConfiguration_version_"+ _versionNumber;
    }

    public long _versionNumber = 0;
}
