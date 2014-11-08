/**
 * The MessagePacket class is used to build outgoing and parse incoming packets
 * of the message mocket framework.
 *
 * @author Mauro Tortonesi 
 */
package us.ihmc.mockets;

import java.net.DatagramPacket;
import java.util.logging.Logger;
import java.util.LinkedList;
import java.util.ListIterator;
import us.ihmc.util.ByteConverter;


class MessagePacket
{
    MessagePacket()
    {
        _packet = null;

        // this is a message-oriented packet
        _flags = HEADER_PROTOCOL_VERSION | HEADER_FLAG_MSGPKT;
        _windowSize = 0;
        _validation = 0;
        _seqNum = 0;

        _incoming = false;

        _offset = HEADER_SIZE;
        _len = HEADER_SIZE;
        _tag = DataChunk.TAG_UNDEFINED;

        _sackChunk = null;
        _sackChunkStatus = SACK_CHUNK_NOT_PRESENT;
        _deliveryPrerequisites = null;

        _logger = Logger.getLogger ("us.ihmc.mockets");
    }

    MessagePacket (byte[] packet, int len)
    {
        if (packet.length < len)
            throw new IllegalArgumentException ("packet.length and len are misaligned!");
        
        if (len < HEADER_SIZE)
            throw new IllegalArgumentException ("Packet too short: " + len +
                                                " bytes (a valid packet " + 
                                                "must be at least " + 
                                                HEADER_SIZE + " bytes)!");
        
        _packet = (byte[]) packet.clone();
        
        _flags = ByteConverter.from2BytesToUnsignedShortInt (_packet, 0);
        
        int version = _flags & 0xF000;
        if (version != HEADER_PROTOCOL_VERSION)
            // ok, throwing an exception here is maybe too much, but we want to catch
            // errors quickly during the first development phases
            throw new IllegalArgumentException ("Packet has a wrong protocol version!");
        
        int flags = _flags & 0x0FFF;
        if ((flags & (~HEADER_FLAGS_MASK)) != 0)
            // ok, throwing an exception here is maybe too much, but we want to catch
            // errors quickly during the first development phases
            throw new IllegalArgumentException ("Packet has an unknown flag set!");
        
        if ((flags & HEADER_FLAG_MSGPKT) == 0)
            // ok, throwing an exception here is maybe too much, but we want to catch
            // errors quickly during the first development phases
            throw new IllegalArgumentException ("This is not a Message Packet!");
        
        _windowSize = ByteConverter.from2BytesToUnsignedShortInt (_packet, 2);
        
        _validation = ByteConverter.from4BytesToUnsignedInt (_packet, 4);
        if (_validation == 0)
            throw new IllegalArgumentException ("Packet has an invalid validation tag!");
        
        _seqNum = ByteConverter.from4BytesToUnsignedInt (_packet, 8);
        
        _incoming = true;

        _offset = HEADER_SIZE;
        _len = len;
        // the _tag data member is not used for incoming packets
        _tag = DataChunk.TAG_UNDEFINED;
        
        // if we have delivery prerequisites, read them
        if ((flags & HEADER_FLAG_DELIVERY_DEPS) != 0) {
            _deliveryPrerequisites = new DeliveryPrerequisites (_packet, HEADER_SIZE);
            _offset += DeliveryPrerequisites.DELIVERY_PREREQUISITES_SIZE;
        } else {
            _deliveryPrerequisites = null;
        }
        
        _sackChunk = null;
        _sackChunkStatus = SACK_CHUNK_UNDEF_STATE;
        
        _logger = Logger.getLogger ("us.ihmc.mockets");
    }

    void setVersion (int version)
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming;
        // sanity check
        assert (version & 0x0FFF) == 0 : "Version is just a 4 bit field!";
        
        _flags = (_flags & 0x0FFF) + (version & 0xF000);
    }

    void clearFlags() 
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming; 
        
        _flags = _flags & 0xF000;
    }
    
    void setFlags (int flag)
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming;
        // sanity checks
        assert (flag & 0xF000) == 0 : "Flags is just a 12 bit field!";
        assert (flag & HEADER_FLAGS_MASK) != 0 : "Unknown/unsupported flag!";

        _flags |= (flag & HEADER_FLAGS_MASK);
    }

    boolean isFlagSet (int flag) 
    {
        // sanity checks
        assert (flag & 0xF000) == 0 : "Flags is just a 12 bit field!";
        assert (flag & HEADER_FLAGS_MASK) != 0 : "Unknown/unsupported flag!";

        return (_flags & flag) != 0;
    }

    void setValidation (long validation)
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming; 
        // sanity check
        assert (validation != 0) : "Validation field cannot be zero!";

        _validation = validation;
    }

    long getValidation()
    {
        return _validation;
    }

    void setSequenceNumber (long sequenceNumber)
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming; 
        
        _seqNum = sequenceNumber;
    }
    
    long getSequenceNumber()
    {
        return _seqNum;
    }
    
    void setWindowSize (int windowSize)
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming;
        
        _windowSize = windowSize;
    }
    
    int getWindowSize()
    {
        // make sure this IS an incoming packet
        assert _incoming;
        
        return _windowSize;
    }
    
    void allocateSpaceForDeliveryPrerequisites()
    {
        if ((_flags & HEADER_FLAG_SEQUENCED) == 0 &&
            (_flags & HEADER_FLAG_CONTROL) == 0) 
            return;

        assert ((_flags & HEADER_FLAG_CONTROL) != 0) && 
               ((_flags & HEADER_FLAG_RELIABLE) == 0) && 
               ((_flags & HEADER_FLAG_SEQUENCED) == 0);
        
        DeliveryPrerequisites dp = new DeliveryPrerequisites (0, 0);

        setDeliveryPrerequisites (dp);
    }
    
    // dp can be null
    void setDeliveryPrerequisites (DeliveryPrerequisites dp)
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming;
       
        if (_offset != (_deliveryPrerequisites == null ? HEADER_SIZE : HEADER_SIZE + DeliveryPrerequisites.DELIVERY_PREREQUISITES_SIZE)) {
            throw new IllegalStateException ("A chunk has already been written, so it is too late to set the delivery prerequisite.");
        }
        
        _deliveryPrerequisites = dp;
        if (dp != null) {
            _flags |= HEADER_FLAG_DELIVERY_DEPS;
            _offset = HEADER_SIZE + DeliveryPrerequisites.DELIVERY_PREREQUISITES_SIZE;
        } else {
            // dp == null, so unset any previous delivery prerequisites
            _flags &= (HEADER_FLAGS_MASK ^ HEADER_FLAG_DELIVERY_DEPS);
            _offset = HEADER_SIZE;
        }
        _len = _offset;
    }

    DeliveryPrerequisites getDeliveryPrerequisites()
    {
        return _deliveryPrerequisites;
    }
    
    Chunk getFirstChunk()
    {
        if (!_incoming) 
            throw new RuntimeException ("Can only process incoming packets!");
        
        // initialize parsing
        _offset = HEADER_SIZE;
        if (_deliveryPrerequisites != null) {
            _offset += DeliveryPrerequisites.DELIVERY_PREREQUISITES_SIZE;
        }
        
        return getNextChunk ();
    }

    Chunk getNextChunk()
    {
        if (!_incoming) 
            throw new RuntimeException ("Can only process valid incoming packets!");
        
        assert _len - _offset >= 0;
        
        //_logger.info ("_offset = " + _offset + ", _len " + _len + "!");

        if (_len - _offset < Chunk.CHUNK_HEADER_SIZE) {
            //_logger.info ("No more chunks found!");
            
            if (_len - _offset != 0)
                // ok, throwing an exception here is maybe too much, but we want to catch
                // errors quickly during the first development phases
                throw new RuntimeException ("Found " + (_len - _offset) + 
                                            " spurious bytes of data!");
                
            return null;
        }

        Chunk ret = null;
        int chkType = ByteConverter.from2BytesToUnsignedShortInt (_packet, _offset);
        int chkLen = ByteConverter.from2BytesToUnsignedShortInt (_packet, _offset + 2);

        assert _seqNum == ByteConverter.from4BytesToUnsignedInt (_packet, 8);
        
        if (_offset + chkLen > _len) 
            // ok, throwing an exception here is maybe too much, but we want to catch
            // errors quickly during the first development phases
            throw new RuntimeException ("Information in chunk (type " + chkType + 
                                        ", packet " + _seqNum + ", len " + _len + 
                                        ", offset " + _offset + ") length field is invalid! (" + chkLen + 
                                        " found, " + (_len - _offset) + " max expected)");
        
        switch (chkType) {
            case Chunk.CHUNK_TYPE_SACK:
                //_logger.info ("SACK chunk found!");
                assert chkLen >= SACKChunk.SACK_CHUNK_HEADER_SIZE;
                _sackChunk = new SACKChunk (_packet, _offset);
                _sackChunkStatus = SACK_CHUNK_PRESENT;
                ret = _sackChunk;
                break;
            case Chunk.CHUNK_TYPE_HEARTBEAT:
                //_logger.info ("HEARTBEAT chunk found!");
                assert chkLen >= HeartbeatChunk.HEARTBEAT_CHUNK_LENGTH;
                ret = new HeartbeatChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_CANCELLED_PACKETS: 
                //_logger.info ("CANCELLED PACKETS chunk found!");
                assert chkLen >= CancelledPacketsChunk.CANCELLED_PACKETS_CHUNK_HEADER_SIZE;
                ret = new CancelledPacketsChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_DATA:
                //_logger.info ("DATA chunk found in packet " + _seqNum + ", offset " + _offset + ", len " + chkLen);
                assert chkLen >= DataChunk.DATA_CHUNK_HEADER_SIZE;
                ret = new DataChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_INIT:
                //_logger.info ("INIT chunk found!");
                assert (chkLen == InitChunk.INIT_CHUNK_LENGTH);
                ret = new InitChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_INIT_ACK:
                //_logger.info ("INIT-ACK chunk found!");
                assert chkLen == InitAckChunk.INIT_ACK_CHUNK_LENGTH;
                ret = new InitAckChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_COOKIE_ECHO:
                //_logger.info ("COOKIE-ECHO chunk found!");
                assert chkLen == CookieEchoChunk.COOKIE_ECHO_CHUNK_LENGTH;
                ret = new CookieEchoChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_COOKIE_ACK:
                //_logger.info ("COOKIE-ACK chunk found!");
                assert chkLen == CookieAckChunk.COOKIE_ACK_CHUNK_LENGTH;
                ret = new CookieAckChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_SHUTDOWN:
                //_logger.info ("SHUTDOWN chunk found!");
                assert chkLen >= ShutdownChunk.SHUTDOWN_CHUNK_LENGTH;
                ret = new ShutdownChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_SHUTDOWN_ACK:
                //_logger.info ("SHUTDOWN-ACK chunk found!");
                assert chkLen >= ShutdownAckChunk.SHUTDOWN_ACK_CHUNK_LENGTH;
                ret = new ShutdownAckChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_SHUTDOWN_COMPLETE:
                //_logger.info ("SHUTDOWN-COMPLETE chunk found!");
                assert chkLen >= ShutdownCompleteChunk.SHUTDOWN_COMPLETE_CHUNK_LENGTH;
                ret = new ShutdownCompleteChunk (_packet, _offset);
                break;
            case Chunk.CHUNK_TYPE_ABORT:
                //_logger.info ("ABORT chunk found!");
                assert chkLen >= AbortChunk.ABORT_CHUNK_LENGTH;
                ret = new AbortChunk (_packet, _offset);
                break;
            default:
                // ok, throwing an exception here is maybe too much, but we want to catch
                // errors quickly during the first development phases
                throw new RuntimeException ("Found unknown (" + chkType + 
                                            ") chunk type in packet " + _seqNum +
                                            ", offset " + _offset + ", len " + chkLen + "!");
        }

        _offset += chkLen;

        return ret;
    }

    synchronized void writeToDatagramPacket (DatagramPacket dp)
    {
        assert dp != null;
        assert !_incoming; // make sure this IS NOT an incoming packet
        assert _validation != 0; // sanity check on validation field
        assert _len > HEADER_SIZE; // check this is not an empty packet
        
        // get data buffer if needed
        if (_packet == null) {
            _packet = new byte[MessageMocket.MAXIMUM_MTU];
        }

        // write header
        ByteConverter.fromUnsignedShortIntTo2Bytes (_flags, _packet, 0);
        ByteConverter.fromUnsignedShortIntTo2Bytes (_windowSize, _packet, 2);
        ByteConverter.fromUnsignedIntTo4Bytes (_validation, _packet, 4);
        ByteConverter.fromUnsignedIntTo4Bytes (_seqNum, _packet, 8);
       
        // write delivery prerequisites if needed
        if (_deliveryPrerequisites != null) {
            _deliveryPrerequisites.write (_packet, HEADER_SIZE);
        }

        if (_sackChunkStatus == SACK_CHUNK_PRESENT) {
            assert _sackChunk != null;
            _sackChunk.write (_packet, _offset);
            dp.setData (_packet, 0, _len + _sackChunk.getLength());
        } else {
            dp.setData (_packet, 0, _len);
        }
    }
        
    synchronized boolean setSACKChunk (SACKChunk ck) 
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming; 
        assert _len == _offset;
        
        // check we have enough space for SACK chunk
        if (ck != null) {
            // get data buffer if needed
            if (_packet == null) {
                _packet = new byte[MessageMocket.MAXIMUM_MTU];
            }
            if (ck.getLength() > _packet.length - _offset)
                return false;
        }
        
        _sackChunk = ck;
        _sackChunkStatus = SACK_CHUNK_PRESENT;

        return true;
    }
    
    synchronized void unsetSACKChunk() 
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming; 
        
        _sackChunk = null;
        _sackChunkStatus = SACK_CHUNK_NOT_PRESENT;
    }
    
    synchronized boolean addChunk (Chunk ck) 
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming; 
        assert _len == _offset;
        
        if (ck.getType() == Chunk.CHUNK_TYPE_SACK) { // or ck instanceof(SACKChunk)...
            throw new RuntimeException ("Use the setSACKChunk method to add a SACK chunk instead of addChunk.");
        }
        
        if (ck.getType() == Chunk.CHUNK_TYPE_DATA) { // or ck instanceof(DataChunk)...
            DataChunk dtChk = (DataChunk) ck;
            int tag = dtChk.getTagID();            
            if (_tag == DataChunk.TAG_UNDEFINED) {
                _tag = tag;
            } else if (_tag != tag) {
                throw new RuntimeException ("Cannot have Data chunks with different tag IDs.");
            }
        }

        // get data buffer if needed
        if (_packet == null) {
            _packet = new byte[MessageMocket.MAXIMUM_MTU];
        }

        synchronized (ck) {
            int size = ck.getLength();
            int freeSpace = _packet.length - _offset;
            if (_sackChunkStatus == SACK_CHUNK_PRESENT) {
                assert _sackChunk != null;
                freeSpace -= _sackChunk.getLength();
            }
            if (size > freeSpace)
                return false;

            ck.write (_packet, _offset);
            
            _offset += size;
            _len += size;
        }
        
        return true;
    }

    synchronized int getTagID() 
    {
        // make sure this IS NOT an incoming packet
        assert !_incoming; 
        return _tag;
    }
    
    synchronized int getSize() 
    {
        return _len;
    }
   
    synchronized int getSizeWithoutSACKChunk()
    {
        // make sure this IS an incoming packet
        assert _incoming;
       
        int len = _len;

        if (_sackChunkStatus == SACK_CHUNK_UNDEF_STATE) {
            assert _sackChunk == null;
            // fast scan for SACK chunk
            int offset = HEADER_SIZE;
            if (_deliveryPrerequisites != null) {
                offset += DeliveryPrerequisites.DELIVERY_PREREQUISITES_SIZE;
            }
            do {
                int chkType = ByteConverter.from2BytesToUnsignedShortInt (_packet, offset);
                int chkLen  = ByteConverter.from2BytesToUnsignedShortInt (_packet, offset + 2);
                
                if (chkType == Chunk.CHUNK_TYPE_SACK) {
                    _sackChunk = new SACKChunk (_packet, offset);
                    _sackChunkStatus = SACK_CHUNK_PRESENT;
                    len -= _sackChunk.getLength();
                    break;
                }

                offset += chkLen;
            } while (offset < _len);
        } else if (_sackChunkStatus == SACK_CHUNK_PRESENT) {
            assert _sackChunk != null;
            len -= _sackChunk.getLength();
        }

        return len;
    }
   
    static boolean isValidChunkType (int type) 
    {
        boolean ret = false;

        switch (type) {
        case Chunk.CHUNK_TYPE_SACK:
        case Chunk.CHUNK_TYPE_HEARTBEAT:
        case Chunk.CHUNK_TYPE_CANCELLED_PACKETS: 
        case Chunk.CHUNK_TYPE_DATA:
        case Chunk.CHUNK_TYPE_INIT:
        case Chunk.CHUNK_TYPE_INIT_ACK:
        case Chunk.CHUNK_TYPE_COOKIE_ECHO:
        case Chunk.CHUNK_TYPE_COOKIE_ACK:
        case Chunk.CHUNK_TYPE_SHUTDOWN:
        case Chunk.CHUNK_TYPE_SHUTDOWN_ACK:
        case Chunk.CHUNK_TYPE_SHUTDOWN_COMPLETE:
        case Chunk.CHUNK_TYPE_ABORT:
            ret = true;
            break;
        }

        return ret;
    }

    static final int HEADER_SIZE                     = 12;
    static final int HEADER_FLAG_RELIABLE            = 0x001;
    static final int HEADER_FLAG_SEQUENCED           = 0x002;
    static final int HEADER_FLAG_MSGPKT              = 0x004;
    static final int HEADER_FLAG_CONTROL             = 0x008;
    static final int HEADER_FLAG_DELIVERY_DEPS       = 0x010;
    static final int HEADER_FLAG_FIRST_FRAGMENT      = 0x020;
    static final int HEADER_FLAG_LAST_FRAGMENT       = 0x040;
    static final int HEADER_FLAG_MORE_FRAGMENTS      = 0x080;
    private static final int HEADER_PROTOCOL_VERSION = (0x1 << 12);
    private static final int HEADER_FLAGS_MASK       = HEADER_FLAG_RELIABLE |
                                                       HEADER_FLAG_SEQUENCED |
                                                       HEADER_FLAG_MSGPKT |
                                                       HEADER_FLAG_CONTROL |
                                                       HEADER_FLAG_DELIVERY_DEPS |
                                                       HEADER_FLAG_FIRST_FRAGMENT |
                                                       HEADER_FLAG_LAST_FRAGMENT |
                                                       HEADER_FLAG_MORE_FRAGMENTS;

    private byte[] _packet;
    private int _flags;
    private long _validation;
    private long _seqNum;
    private int _windowSize;
    private boolean _incoming;
    private DeliveryPrerequisites _deliveryPrerequisites;
    private int _offset;
    private int _len;
    private int _tag;
    private Logger _logger;

    private SACKChunk _sackChunk;
    private int _sackChunkStatus;
    private static final int SACK_CHUNK_UNDEF_STATE           = 0;
    private static final int SACK_CHUNK_NOT_PRESENT           = 1;
    private static final int SACK_CHUNK_PRESENT               = 2;
}

/*
 * vim: et ts=4 sw=4
 */
