package us.ihmc.mockets;

import java.net.InetAddress;

import us.ihmc.util.ByteConverter;

/**
 * The StreamPacket class is used to build packets by filling in appropriate
 * information into the header fields.
 *
 * Format for the packet header is as follows:
 * byte(s)     content
 * -------     -------
 * 0-1         flags (Syn, SynAck, Fin, FinAck)
 * 2           control sequence number
 * 3           control acknowledgement number
 * 4-7         sequence number
 * 8-11        acknowledgement number
 * 12-13       advertized window size
 * 14-15       payload size
 * 16-         payload
 */
public class StreamPacket
{
    public StreamPacket()
    {
        _flags = 0;
        _ctrlSeqNum = 0;
        _ctrlAckNum = 0;
        _seqNum = 0;
        _ackNum = 0;
        _windowSize = 0;
        _payloadSize = 0;
        _packet = null;
        _lastTransTime = 0;
        _remoteAddr = null;
        _remotePort = 0;
    }

    public StreamPacket (byte[] packet)
    {
        _packet = packet;
        if (_packet.length < HEADER_SIZE) {
            throw new IllegalArgumentException ("packet too short - " + _packet.length + " bytes - must be at least " + HEADER_SIZE + " bytes");
        }
        _flags = ByteConverter.from2BytesToUnsignedShortInt (_packet, 0);
        _ctrlSeqNum = ByteConverter.from1ByteToUnsignedByte (_packet, 2);
        _ctrlAckNum = ByteConverter.from1ByteToUnsignedByte (_packet, 3);
        _seqNum = ByteConverter.from4BytesToUnsignedInt (_packet, 4);
        _ackNum = ByteConverter.from4BytesToUnsignedInt (_packet, 8);
        _windowSize = ByteConverter.from2BytesToUnsignedShortInt (_packet, 12);
        _payloadSize = ByteConverter.from2BytesToUnsignedShortInt (_packet, 14);
        if (_payloadSize > (_packet.length - HEADER_SIZE)) {
            throw new IllegalArgumentException ("corrupt packet - payload size in header " + _payloadSize + " does not match payload size of " + (_packet.length - HEADER_SIZE));
        }
        _lastTransTime = 0;
        _remoteAddr = null;
        _remotePort = 0;
    }

    void setSYNBit()
    {
        _flags |= HEADER_FLAG_SYN;
    }

    boolean getSYNBit()
    {
        return ((_flags & HEADER_FLAG_SYN) != 0);
    }

    void setSYNAckBit()
    {
        _flags |= HEADER_FLAG_SYNACK;
    }

    boolean getSYNAckBit()
    {
        return ((_flags & HEADER_FLAG_SYNACK) != 0);
    }

    void setFINBit()
    {
        _flags |= HEADER_FLAG_FIN;
    }

    boolean getFINBit()
    {
        return ((_flags & HEADER_FLAG_FIN) != 0);
    }

    void setFINAckBit()
    {
        _flags |= HEADER_FLAG_FINACK;
    }

    boolean getFINAckBit()
    {
        return ((_flags & HEADER_FLAG_FINACK) != 0);
    }

    boolean isCtrlPacket()
    {
        return (_flags != 0);
    }

    void setCtrlSeqNum (short cSeq)
    {
        _ctrlSeqNum = cSeq;
    }

    short getCtrlSeqNum()
    {
        return _ctrlSeqNum;
    }

    void setCtrlAckNum (short cAck)
    {
        _ctrlAckNum = cAck;
    }

    short getCtrlAckNum()
    {
        return _ctrlAckNum;
    }

    void setSeqNum (long seq)
    {
        _seqNum = seq;
    }

    long getSeqNum()
    {
        return _seqNum;
    }

    void setAckNum (long ack)
    {
        _ackNum = ack;
    }

    long getAckNum()
    {
        return _ackNum;
    }

    void setAdvertizedWindowSize (int windowSize)
    {
        _windowSize = windowSize;
    }

    int getAdvertizedWindowSize()
    {
        return _windowSize;
    }

    void setPayload (byte[] payload, int off, int len)
    {
        if (payload == null) {
            throw new NullPointerException ("payload");
        }
        _payloadSize = len;
        _packet = new byte [HEADER_SIZE + _payloadSize];
        if (_payloadSize > 0) {
            System.arraycopy (payload, 0, _packet, HEADER_SIZE, _payloadSize);
        }
    }

    public byte[] getPayload()
    {
        byte[] payload = new byte[_payloadSize];
        System.arraycopy(_packet, HEADER_SIZE, payload, 0, _payloadSize);
        return payload;
    }

    public int getPayloadSize()
    {
        return _payloadSize;
    }

    int getPacketSize()
    {
        return _payloadSize + HEADER_SIZE;
    }

    void setLastTransmitTime (long lastTransTime)
    {
        _lastTransTime = lastTransTime;
    }

    long getLastTransmitTime()
    {
        return _lastTransTime;
    }

    void setRemoteAddress (InetAddress remoteAddr)
    {
        _remoteAddr = remoteAddr;
    }

    public InetAddress getRemoteAddress()
    {
        return _remoteAddr;
    }

    void setRemotePort (int remotePort)
    {
        _remotePort = remotePort;
    }

    public int getRemotePort()
    {
        return _remotePort;
    }

    byte[] getPacket()
    {
        if (_packet == null) {
            _packet = new byte [HEADER_SIZE];
        }
        ByteConverter.fromUnsignedShortIntTo2Bytes (_flags, _packet, 0);
        ByteConverter.fromUnsignedByteTo1Byte (_ctrlSeqNum, _packet, 2);
        ByteConverter.fromUnsignedByteTo1Byte (_ctrlAckNum, _packet, 3);
        ByteConverter.fromUnsignedIntTo4Bytes (_seqNum, _packet, 4);
        ByteConverter.fromUnsignedIntTo4Bytes (_ackNum, _packet, 8);
        ByteConverter.fromUnsignedShortIntTo2Bytes (_windowSize, _packet, 12);
        ByteConverter.fromUnsignedShortIntTo2Bytes (_payloadSize, _packet, 14);
        return _packet;
    }

    static int getHeaderSize()
    {
        return HEADER_SIZE;
    }

    private static final int HEADER_SIZE = 16;
    private static final int HEADER_FLAG_SYN = 0x01;
    private static final int HEADER_FLAG_SYNACK = 0x02;
    private static final int HEADER_FLAG_FIN = 0x04;
    private static final int HEADER_FLAG_FINACK = 0x08;

    private int _flags;
    private short _ctrlSeqNum;
    private short _ctrlAckNum;
    private long _seqNum;
    private long _ackNum;
    private int _windowSize;
    private int _payloadSize;
    private byte[] _packet;

    private long _lastTransTime;
    private InetAddress _remoteAddr;
    private int _remotePort;
}
