package us.ihmc.mockets;

public class TxParams {
    
    public TxParams () 
    {
        _tag = DataChunk.TAG_DEFAULT;
        _priority = DEFAULT_PRIORITY;
        _enqueue_timeout = DEFAULT_ENQUEUE_TIMEOUT;
        _retry_timeout = DEFAULT_RETRY_TIMEOUT;
    }

    public TxParams (int tag, int priority,
                     long enqueue_timeout, long retry_timeout) 
    {
        _tag = tag;
        _priority = priority;
        _enqueue_timeout = enqueue_timeout;
        _retry_timeout = retry_timeout;
    }

    int getTag() 
    {
        return _tag;
    }
    
    int getPriority() 
    {
        return _priority;
    }

    long getEnqueueTimeout() 
    {
        return _enqueue_timeout;
    }
    
    long getRetryTimeout() 
    {
        return _retry_timeout;
    }

    static int DEFAULT_PRIORITY = 5;
    static int MIN_PRIORITY = 1;
    static int MAX_PRIORITY = 10;
    static long DEFAULT_ENQUEUE_TIMEOUT = 0;
    static long DEFAULT_RETRY_TIMEOUT = 0;
        
    private int _tag;
    private int _priority;
    private long _enqueue_timeout;
    private long _retry_timeout;
}
/*
 * vim: et ts=4 sw=4
 */

