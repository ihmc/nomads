package us.ihmc.mockets;

import java.io.IOException;
import java.util.logging.Logger;

class SenderImpl implements Sender 
{
    SenderImpl (MessageMocket mocket, boolean sequenced, boolean reliable)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _mocket = mocket;
        _transmitter = _mocket.getTransmitter();
        _sequenced = sequenced;
        _reliable = reliable;
        _tag = DataChunk.TAG_DEFAULT;
        _priority = TxParams.DEFAULT_PRIORITY;
        _enqueueTimeout = TxParams.DEFAULT_ENQUEUE_TIMEOUT;
        _retryTimeout = TxParams.DEFAULT_RETRY_TIMEOUT;
    }

    SenderImpl (MessageMocket mocket, boolean sequenced, boolean reliable, TxParams params)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _mocket = mocket;
        _transmitter = _mocket.getTransmitter();
        _sequenced = sequenced;
        _reliable = reliable;
        _tag = params.getTag();
        _priority = params.getPriority();
        _enqueueTimeout = params.getEnqueueTimeout();
        _retryTimeout = params.getRetryTimeout();
    }

    public void send (byte[] buf, int off, int len)
        throws IOException
    {
        _transmitter.send (_reliable, _sequenced, buf, off, len, _tag, 
                           _priority, _enqueueTimeout, _retryTimeout);
    }

    public void send (byte[] buf, int off, int len, int tag, int priority) 
        throws IOException
    {
        _transmitter.send (_reliable, _sequenced, buf, off, len, tag, 
                           priority, _enqueueTimeout, _retryTimeout);
    }
    
    public void send (byte[] buf, int off, int len, TxParams tx_params)
        throws IOException
    {
        if (!_reliable && tx_params.getTag() != DataChunk.TAG_DEFAULT)
            throw new IllegalArgumentException ("Message overriding is not " + 
                                                "supported for unreliable flows.");
        _transmitter.send (_reliable, _sequenced, buf, off, len, 
                           tx_params.getTag(), tx_params.getPriority(), 
                           tx_params.getEnqueueTimeout(), tx_params.getRetryTimeout());
    }
    
    public void send (byte[] buf, int off, int len, int tag, int priority,
                      long enqueueTimeout, long retryTimeout)
        throws IOException
    {
        if (!_reliable && tag != DataChunk.TAG_DEFAULT)
            throw new IllegalArgumentException ("Message overriding is not " + 
                                                "supported for unreliable flows.");
        _transmitter.send (_reliable, _sequenced, buf, off, len, tag, 
                           priority, enqueueTimeout, retryTimeout);
    }
    
    public void replace (byte[] buf, int off, int len, int oldtag, int newtag)
        throws IOException 
    {
        if (!_reliable)
            throw new IllegalArgumentException ("Message overriding is not " + 
                                                "supported for unreliable flows.");
        _transmitter.replace (_sequenced, buf, off, len, oldtag, newtag,
                              _priority, _enqueueTimeout, _retryTimeout);
    }
    
    public void replace (byte[] buf, int off, int len, int oldtag, TxParams tx_params)
        throws IOException 
    {
        if (!_reliable)
            throw new IllegalArgumentException ("Message overriding is not " + 
                                                "supported for unreliable flows.");
        _transmitter.replace (_sequenced, buf, off, len, oldtag,
                              tx_params.getTag(), tx_params.getPriority(), 
                              tx_params.getEnqueueTimeout(), tx_params.getRetryTimeout());
    }

    public void replace (byte[] buf, int off, int len, int oldtag, int newtag, 
                         int priority, long enqueueTimeout, long retryTimeout)
        throws IOException 
    {
        if (!_reliable)
            throw new IllegalArgumentException ("Message overriding is not " + 
                                                "supported for unreliable flows.");
        _transmitter.replace (_sequenced, buf, off, len, oldtag, newtag, 
                              priority, enqueueTimeout, retryTimeout);
    }
    
    public void cancel (int tag) 
    {
        if (!_reliable)
            throw new IllegalArgumentException ("Message overriding is not " + 
                                                "supported for unreliable flows.");
        _transmitter.cancel (_sequenced, tag); 
    }
    
    public void setDefaultEnqueueTimeout (long timeout) 
    {
        _enqueueTimeout = timeout;
    }
    
    public void setDefaultRetryTimeout (long timeout) 
    {
        _retryTimeout = timeout;
    }
    
    private MessageMocket _mocket;
    private MessageTransmitter _transmitter;
    private boolean _sequenced;
    private boolean _reliable;
    private int _tag;
    private int _priority;
    private long _enqueueTimeout;
    private long _retryTimeout;
    private Logger _logger;
}
/*
 * vim: et ts=4 sw=4
 */
