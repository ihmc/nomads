package us.ihmc.aci.kernel;

import java.io.IOException;
import com.ibm.JikesRVM.VM_FileSystem;
import com.ibm.JikesRVM.VM_SysCall;
import com.ibm.JikesRVM.VM_ThreadEventConstants;
import com.ibm.JikesRVM.VM_TimeoutException;

public class RVMServiceInputStream extends ServiceInputStream
{
    private int socketFD;
    private boolean closed;
    private int receiveTimeout;

    RVMServiceInputStream (int sd)
    {
        socketFD = sd;
        closed = false;
        receiveTimeout = 0;
    }

    /**
     *
     */
    public int read() throws IOException
    {
        if (closed) {
            throw new IOException("stream closed");
        }

        byte[] buffer = new byte[1];
        int result = arrayRead (buffer, 0, 1);
        return (-1 == result) ? result : ((int) buffer[0])&0xFF;
    }

    /**
     *
     */
    void setTimeout (int timeout)
    {
        receiveTimeout = timeout;
    }

    /**
     * In the IP stack, read at most count bytes off the socket
     * into the buffer, at the offset.
     * If the timeout is zero, block indefinitely waiting
     * for data, otherwise wait the specified period (in milliseconds).
     *
     * @param             buffer          the buffer to read into
     * @param             offset          the offset into the buffer
     * @param             count           the max number of bytes to read
     * @return            int             the actual number of bytes read
     * @exception IOException     thrown if an error occurs while reading
     */
    private synchronized int arrayRead (byte[] buffer, int offset, int count) throws IOException
    {
        if (count == 0) {
            return 0;
        }

        double totalWaitTime = (receiveTimeout > 0)
                               ? ((double) receiveTimeout) / 1000.0
                               : VM_ThreadEventConstants.WAIT_INFINITE;

        int rc;
        try {
            rc = VM_FileSystem.readBytes (socketFD, buffer, offset, count, totalWaitTime);
        //      System.out.println(Thread.currentThread().getName()+": [Read from fd ="+socketFD+"] => "+rc);
        }
        catch (VM_TimeoutException e) {
            throw new IOException("socket receive timed out");
        }

        return (rc == 0) ? -1 : rc;
    }

    /**
     *
     */
    public int read (byte b[], int off, int len) throws IOException
    {
        if (closed) {
            throw new IOException("stream closed");
        }

        return arrayRead(b, off, len);
    }

    /**
     * Answer the number of bytes that may be read from this
     * socket without blocking.  This call does not block.
     *
     * @return int   the number of bytes that may be read without blocking
     * @exception IOException if an error occurs while peeking
     */
    public synchronized int available() throws IOException
    {
        if (closed) {
            throw new IOException("stream closed");
        }

        return VM_FileSystem.bytesAvailable(socketFD);
    }

    /**
     *
     * @throws IOException
     */
    public void close() throws IOException
    {
        if (!closed) {
            if (VM_SysCall.sysIsValidFD(socketFD) == 0) {
                VM_SysCall.sysNetSocketClose(socketFD);
            }

            closed = true;
        }
    }
}
