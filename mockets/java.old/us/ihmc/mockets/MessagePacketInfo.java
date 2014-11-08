package us.ihmc.mockets;

class MessagePacketInfo
{
    // this constructor is used by the transmitter
    MessagePacketInfo (MessagePacket packet, int priority, long retryTimeout, 
                       long lastIOTime, long retransmTimeout)
    {
        if (packet == null)
            throw new IllegalArgumentException ("packet can't be null.");
        _packet = packet;
        
        _control = packet.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL);
        _reliable = packet.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE);
        _sequenced = packet.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED);

        if (priority < TxParams.MIN_PRIORITY || 
            priority > TxParams.MAX_PRIORITY)
            throw new IllegalArgumentException ("priority must be a value between " +
                                                TxParams.MIN_PRIORITY + " and " +
                                                TxParams.MAX_PRIORITY);
        _priority = priority;
        
        if (retryTimeout < 0)
            throw new IllegalArgumentException ("retryTimeout must be a positive value.");
        _retryTimeout = retryTimeout;
        
        if (lastIOTime < 0)
            throw new IllegalArgumentException ("lastIOTime must be a positive value.");        
        _lastIOTime = lastIOTime;
        
        if (retransmTimeout <= 0)
            throw new IllegalArgumentException ("retransmTimeout must be a positive value.");        
        _retransmTimeout = retransmTimeout;
    }    

    // this constructor is used by the receiver
    MessagePacketInfo (MessagePacket packet, long lastIOTime)
    {
        if (packet == null)
            throw new IllegalArgumentException ("packet can't be null.");
        _packet = packet;
        
        _control = packet.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL);
        _reliable = packet.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE);
        _sequenced = packet.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED);

        if (lastIOTime <= 0)
            throw new IllegalArgumentException ("lastIOTime must be a positive value.");        
        _lastIOTime = lastIOTime;
        
        // undefined
        _priority = -1;
        _retryTimeout = -1;
        _retransmTimeout = -1;
    }    

    // this constructor is used by the receiver to build dummy packets
    MessagePacketInfo (long tsn, boolean control, boolean reliable, 
                       boolean sequenced, long lastIOTime)
    {
        _packet = null;
        _control = control;
        _reliable = reliable;
        _sequenced = sequenced;
        
        if (lastIOTime <= 0)
            throw new IllegalArgumentException ("lastIOTime must be a positive value.");        
        _lastIOTime = lastIOTime;
        
        // undefined
        _priority = -1;
        _retryTimeout = -1;
        _retransmTimeout = -1;
    }    

    boolean isControl()
    {
        return _control;
    }
    
    boolean isReliable()
    {
        return _reliable;
    }
    
    boolean isSequenced()
    {
        return _sequenced;
    }
    
    long getLastIOOperationTime() 
    {
        return _lastIOTime;
    }

    void setLastIOOperationTime (long lastIOTime) 
    {
        if (lastIOTime <= 0)
            throw new IllegalArgumentException ("lastIOTime must be a positive value."); 
        _lastIOTime = lastIOTime;
    }

    long getRetransmissionTimeout() 
    {
        return _retransmTimeout;
    }
    
    void setRetransmissionTimeout (long retransmTimeout) 
    {
        if (retransmTimeout <= 0)
            throw new IllegalArgumentException ("retransmTimeout must be a positive value."); 
        _retransmTimeout = retransmTimeout;
    }

    int getPriority() 
    {
        return _priority;
    }

    void setPriority (int priority)
    {
        if (priority < TxParams.MIN_PRIORITY || 
            priority > TxParams.MAX_PRIORITY)
            throw new IllegalArgumentException ("priority must be a value between " +
                                                TxParams.MIN_PRIORITY + " and " +
                                                TxParams.MAX_PRIORITY);
        _priority = priority;
    }    
    
    long getRetryTimeout() 
    {
        return _retryTimeout;
    }
    
    MessagePacket getPacket() 
    {
        return _packet;
    }

    private MessagePacket _packet;
    private int _priority;
    private long _retryTimeout;
    private long _lastIOTime;
    private long _retransmTimeout;
    private boolean _control;
    private boolean _reliable;
    private boolean _sequenced;
}

/*
 * vim: et ts=4 sw=4
 */

