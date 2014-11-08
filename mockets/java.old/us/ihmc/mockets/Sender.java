package us.ihmc.mockets;

import java.io.IOException;

public interface Sender
{
    /**
     * This function enqueues the given message into the send queue.
     * 
     * This function support two different timeouts: the "enqueue 
     * timeout" and the "retry timeout".
     *
     * The "enqueue timeout" is the timeout to put the packet in the
     * internal outstanding packet queue. If this timeout expires,
     * the send method throws an exception.
     * 
     * The "retry timeout" is the one that makes the sender discard 
     * any fragments of the message if all the fragments have not been 
     * acknowledged by the receiver. 
     * 
     * Notice that on the receiver side the "retry timeout" value will 
     * play the role of a "garbage collection timeout" for fragments 
     * belonging to a packet that has not been completely received.
     *
     * For timeout values:
     * -1 means wait forever
     *  0 means return immediately
     */
    public void send (byte[] buf, int off, int len)
        throws IOException;
    public void send (byte[] buf, int off, int len, int tag, int priority)
        throws IOException;
    public void send (byte[] buf, int off, int len, TxParams tx_params)
        throws IOException;
    public void send (byte[] buf, int off, int len, int tag, int priority,
                      long enqueue_timeout, long retry_timeout)
        throws IOException;

    /**
     * This function replace all enqueued packets of the given tag id with
     * the given message.
     */ 
    public void replace (byte[] buf, int off, int len, int oldtag, int newtag)
        throws IOException;
    public void replace (byte[] buf, int off, int len, int oldtag, TxParams tx_params)
        throws IOException;
    public void replace (byte[] buf, int off, int len, int oldtag, int newtag, 
                         int priority, long enqueue_timeout, long retry_timeout)
        throws IOException;

    /**
     * This function cancels all queued packet with the given tag id.
     * Returns a RuntimeException if this is not a reliable sender.
     */
    public void cancel (int oldtag);
    
    /**
     * This function sets the default output timeout for this flow.
     */
    void setDefaultEnqueueTimeout (long timeout);
    void setDefaultRetryTimeout (long timeout);
}
/*
 * vim: et ts=4 sw=4
 */

