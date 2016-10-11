package us.ihmc.netutils;

import java.util.concurrent.atomic.AtomicInteger;

/**
 * Stats.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Stats
{
    public enum Type
    {
        Client(0),
        Server(1);

        Type (int code)
        {
            _code = code;
        }

        public int code ()
        {
            return _code;
        }

        public Type fromCode (int code)
        {
            switch (code) {
                case 0:
                    return Client;
                case 1:
                    return Server;
                default:
                    throw new IllegalArgumentException("Unable to recognize code " + code);
            }

        }

        private final int _code;
    }

    public Stats (Type statsType)
    {
        this.statsType = statsType;
        this.msgSent = new AtomicInteger(0);
        this.msgReceived = new AtomicInteger(0);
        this.throughputSent = new AtomicInteger(0);
        this.throughputReceived = new AtomicInteger(0);
    }

    public int getPacketLoss ()
    {
        int packetLoss = msgSent.get() - msgReceived.get();
        return (packetLoss > 0 && !this.statsType.equals(Type.Client)) ? packetLoss : 0;
    }

    public int getPacketLossPercent ()
    {
        return (getPacketLoss() > 0 && msgSent.get() > 0) ? (getPacketLoss() * 100 / msgSent.get()) : 0;
    }

    @Override
    public String toString ()
    {
        return statsType + " stats. Msg sent: " + msgSent.get() + ", msg received: " + msgReceived.get()
                + " Packet loss " + getPacketLoss()
                + "(" + getPacketLossPercent() + "%), throughput sent: " + throughputSent.get() + ", " +
                "received: " + throughputReceived.get();
    }

    public final Type statsType;
    public final AtomicInteger msgSent;
    public final AtomicInteger msgReceived;
    public final AtomicInteger throughputSent;
    public final AtomicInteger throughputReceived;
}
