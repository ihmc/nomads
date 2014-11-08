package us.ihmc.mockets;

import java.util.logging.Logger;

/**
 * The ReceiverBuffer class maintains a byte-oriented buffer to hold data received.
 * The buffer allows non-contiguous data chunks to be enqueued, upto the maximum
 * size of the buffer.
 */
public class ReceiverByteBuffer
{
    /**
     * Create a ReceiverByteBuffer with the specified maximum size
     */
    public ReceiverByteBuffer (int maxSize)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _maxSize = maxSize;
        _firstByteIndex = 0;
        _contiguousByteCount = 0;
    }

    /**
     * Enqueue the data passed in starting at the specified position<p>
     * NOTE: This method does not enqueue partial chunks of data
     * <p>
     * @param buf       the data to enqueue
     * @param startPos  the starting position (i.e., the index of the first byte) of the data
     *                  NOTE: This is not the starting position in the byte array, but the
     *                  position of the data in the byte array in the overall stream of data
     * <p>
     * @return          true if the data was enqueued, false if there was no room available
     */
    public boolean enqueue (byte[] buf, int startPos)
    {
        if (startPos < _firstByteIndex) {
            // Cannot enqueue data sequenced at a point below what has already been read
            _logger.fine ("RBB.enqueue: cannot buffer data at sequence number " + startPos + " below what has already been read " + _firstByteIndex);
            return false;
        }
        if ((startPos + buf.length - _firstByteIndex) >= _maxSize) {
            // Insufficient room to hold data
            _logger.fine ("RBB.enqueue: insufficient room to buffer data");
            return false;
        }
        if (_head == null) {
            _head = new Chunk();
            _head.startPos = startPos;
            _head.buf = buf;
            _head.next = null;
        }
        else if (_head.startPos > startPos) {
            if ((startPos + buf.length) > _head.startPos) {
                // Data overlap not allowed
                _logger.fine ("RBB.enqueue: cannot buffer overlapping data (1)");
                return false;
            }
            Chunk temp = new Chunk();
            temp.buf = buf;
            temp.startPos = startPos;
            temp.next = _head;
            _head = temp;
        }
        else {
            Chunk temp = _head;
            Chunk newChunk = new Chunk();
            newChunk.buf = buf;
            newChunk.startPos = startPos;
            while (true) {
                if (temp.next == null) {
                    if ((temp.startPos + temp.buf.length) > startPos) {
                        // Data overlap not allowed
                        _logger.fine ("RBB.enqueue: cannot buffer overlapping data (2)");
                        return false;
                    }
                    newChunk.next = null;
                    temp.next = newChunk;
                    break;
                }
                else if (temp.next.startPos > startPos) {
                    if ((startPos + buf.length) > temp.next.startPos) {
                        // Data overlap not allowed
                        _logger.fine ("RBB.enqueue: cannot buffer overlapping data (3)");
                        return false;
                    }
                    else if ((temp.startPos + temp.buf.length) > startPos) {
                        // Data overlap not allowed
                        _logger.fine ("RBB.enqueue: cannot buffer overlapping data (4)");
                        return false;
                    }
                    newChunk.next = temp.next;
                    temp.next = newChunk;
                    break;
                }
                temp = temp.next;
            }
        }
        computeContiguousByteCount();
        return true;
    }

    /**
     * Dequeue the data in the buffer upto the specified number of bytes
     * <p>
     * @param buf       the buffer into which the data should be dequeued
     * @param off       the offset into the buffer at which the data should be dequeued
     * @param len       the maximum number of bytes to dequeue (should not be larger than the size of the buffer)
     *
     * @return          the actual number of bytes dequeued
     */
    public int dequeue (byte[] buf, int off, int len)
    {
        return dequeueRecursive (buf, off, len);
    }

    /**
     * Retrieve the maximum size of the buffer
     * <p>
     * @return          the maximum size
     */
    public int getMaxSize()
    {
        return _maxSize;
    }

    public int getFirstByteIndex()
    {
        return _firstByteIndex;
    }

    /**
     * Retrieve the number of contiguous bytes available
     * <p>
     * @return          the number of contiguous bytes available
     */
    public int getContiguousByteCount()
    {
        return _contiguousByteCount;
    }

    public void displayState()
    {
        System.out.println ("**** State ****");
        System.out.println ("_firstByteIndex = " + _firstByteIndex + "; _contiguousByteCount = " + _contiguousByteCount +
                            "; _maxSize = " + _maxSize);
        Chunk temp = _head;
        while (temp != null) {
            System.out.println ("  Chunk start = " + temp.startPos + "; length = " + temp.buf.length);
            temp = temp.next;
        }
        System.out.println();
    }

    public static void main (String[] args)
    {
        // Driver to exercise the class
        ReceiverByteBuffer rbb = new ReceiverByteBuffer (1024);
        rbb.displayState();
        if (rbb.enqueue (new byte [10], 5)) {
            System.out.println ("Enqueued 10 bytes starting at 5 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 10 bytes starting at 5 - Bad");
            rbb.displayState();
        }
        if (rbb.enqueue (new byte [10], 10)) {
            System.out.println ("Enqueued 10 bytes starting at 10 - Bad");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 10 bytes starting at 10 - Good");
            rbb.displayState();
        }
        if (rbb.enqueue (new byte [10], 20)) {
            System.out.println ("Enqueued 10 bytes starting at 20 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 10 bytes starting at 20 - Bad");
            rbb.displayState();
        }
        if (rbb.enqueue (new byte [5], 0)) {
            System.out.println ("Enqueued 5 bytes starting at 0 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 5 bytes starting at 0 - Bad");
            rbb.displayState();
        }
        if (rbb.enqueue (new byte [5], 15)) {
            System.out.println ("Enqueued 5 bytes starting at 15 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 5 bytes starting at 15 - Bad");
            rbb.displayState();
        }

        int count;
        byte[] buf = new byte[1024];

        count = rbb.dequeue (buf, 0, 10);
        System.out.println ("Asked to dequeue 10 bytes and got " + count + "; supposed to get 10");
        rbb.displayState();

        count = rbb.dequeue (buf, 0, 40);
        System.out.println ("Asked to dequeue 40 bytes and got " + count + "; supposed to get 20");
        rbb.displayState();

        if (rbb.enqueue (new byte [10], 5)) {
            System.out.println ("Enqueued 10 bytes starting at 5 - bad");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 10 bytes starting at 5 - good");
            rbb.displayState();
        }

        if (rbb.enqueue (new byte [500], 30)) {
            System.out.println ("Enqueued 500 bytes starting at 30 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 500 bytes starting at 30 - Bad");
            rbb.displayState();
        }

        if (rbb.enqueue (new byte [200], 600)) {
            System.out.println ("Enqueued 200 bytes starting at 600 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 200 bytes starting at 600 - Bad");
            rbb.displayState();
        }

        count = rbb.dequeue (buf, 0, 700);
        System.out.println ("Asked to dequeue 700 bytes and got " + count + "; supposed to get 500");
        rbb.displayState();

        count = rbb.dequeue (buf, 0, 10);
        System.out.println ("Asked to dequeue 10 bytes and got " + count + "; supposed to get 0");
        rbb.displayState();

        if (rbb.enqueue (new byte [50], 550)) {
            System.out.println ("Enqueued 50 bytes starting at 550 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 50 bytes starting at 550 - Bad");
            rbb.displayState();
        }
        if (rbb.enqueue (new byte [20], 530)) {
            System.out.println ("Enqueued 20 bytes starting at 530 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 20 bytes starting at 530 - Bad");
            rbb.displayState();
        }

        count = rbb.dequeue (buf, 0, 200);
        System.out.println ("Asked to dequeue 200 bytes and got " + count + "; supposed to get 200");
        rbb.displayState();

        if (rbb.enqueue (new byte [500], 850)) {
            System.out.println ("Enqueued 500 bytes starting at 850 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 500 bytes starting at 850 - Bad");
            rbb.displayState();
        }

        if (rbb.enqueue (new byte [60], 800)) {
            System.out.println ("Enqueued 60 bytes starting at 800 - Bad");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 60 bytes starting at 800 - Good");
            rbb.displayState();
        }

        if (rbb.enqueue (new byte [500], 1400)) {
            System.out.println ("Enqueued 500 bytes starting at 1400 - Bad");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 500 bytes starting at 1400 - Good");
            rbb.displayState();
        }

        if (rbb.enqueue (new byte [50], 800)) {
            System.out.println ("Enqueued 50 bytes starting at 800 - Good");
            rbb.displayState();
        }
        else {
            System.out.println ("Failed to enqueue 50 bytes starting at 800 - Bad");
            rbb.displayState();
        }
    }

    private int dequeueRecursive (byte[] buf, int off, int len)
    {
        if (len <= 0) {
            return 0;
        }
        if (_head == null) {
            return 0;
        }
        if (_head.startPos != _firstByteIndex) {
            return 0;
        }
        if (_head.buf.length <= len) {
            Chunk temp = _head;
            _head = _head.next;
            System.arraycopy (temp.buf, 0, buf, off, temp.buf.length);
            _firstByteIndex += temp.buf.length;
            _contiguousByteCount -= temp.buf.length;
            return temp.buf.length + dequeueRecursive (buf, off+temp.buf.length, len-temp.buf.length);
        }
        else {
            System.arraycopy (_head.buf, 0, buf, off, len);
            byte[] newBuf = new byte [_head.buf.length - len];
            System.arraycopy (_head.buf, len, newBuf, 0, _head.buf.length - len);
            _head.buf = newBuf;
            _head.startPos += len;
            _firstByteIndex += len;
            _contiguousByteCount -= len;
            return len;
        }
    }

    private void computeContiguousByteCount()
    {
        if (_head == null) {
            // No Change needed - there is no data
        }
        else if (_head.startPos > _firstByteIndex) {
            // Missing data at the beginning - no change needed
        }
        else {
            _contiguousByteCount = 0;
            Chunk temp = _head;
            while (true) {
                _contiguousByteCount += temp.buf.length;
                if ((temp.next == null) || ((temp.startPos + temp.buf.length) != temp.next.startPos)) {
                    break;
                }
                temp = temp.next;
            }
        }
    }

    private class Chunk
    {
        public int startPos;
        public byte[] buf;
        public Chunk next;
    }

    private int _maxSize;
    private int _firstByteIndex;
    private int _contiguousByteCount;
    private Chunk _head;

    private Logger _logger;
}
