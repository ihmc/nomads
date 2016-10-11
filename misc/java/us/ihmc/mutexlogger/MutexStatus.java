package us.ihmc.netutils;

import java.io.ByteArrayInputStream;

/**
 * MutexKey is a class designed to have a unique representation of of a Mutex action (lock, tryLock or unlock)
 * for a certain Thread.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
class MutexKey
{

    protected int _mutexId;
    protected int _currentLocationId;
    protected int _currentThreadId;

    MutexKey ()
    {
        _mutexId = 0;
        _currentLocationId = 0;
        _currentThreadId = 0;
    }

    public MutexKey (int mutexId, int currentLocationId, int threadId)
    {

        this._mutexId = mutexId;
        this._currentLocationId = currentLocationId;
        this._currentThreadId = threadId;
    }

    @Override
    public boolean equals (Object obj)
    {
        if (obj == this)
            return true;
        if (!(obj instanceof MutexKey))
            return false;

        MutexKey mk = (MutexKey) obj;

        return mk._mutexId == _mutexId && mk._currentLocationId == _currentLocationId && mk._currentThreadId ==
                _currentThreadId;
    }

    @Override
    public int hashCode ()
    {

        StringBuffer strBuf = new StringBuffer();

        //make hashCode depends on the two fields
        strBuf.append(_mutexId);
        strBuf.append(_currentLocationId);
        strBuf.append(_currentThreadId);

        return strBuf.toString().hashCode();
    }
}

/**
 *  MutexStatus is the class defining the object informations and structure
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class MutexStatus extends MutexKey
{
    public static final int MUTEX_BUFFER_SIZE = 23; //Sum of the bytes taken by the fields
    private int _lastLockLocationId;
    private int _ownerThreadId;
    private int _status; // Status = 1 for trying to lock, 2 for lock succeeded, 3 for unlocked
    private long _timestamp; //time were the object was received

    public MutexStatus (int mutexId, int currentLocationId, int lastLockLocationId, int ownerThreadId,
                        int currentThreadId, int status)
    {
        this._mutexId = mutexId;
        this._currentLocationId = currentLocationId;
        this._lastLockLocationId = lastLockLocationId;
        this._ownerThreadId = ownerThreadId;
        this._currentThreadId = currentThreadId;
        this._status = status;
        //time stamp of the generation of this MutexStatus
        setTimestamp();
    }

    public static MutexStatus getMutexStatusFromByteArray (byte[] buf)
    {
        //mutex status mapping from byte array
        byte[] mutexId = new byte[2];
        byte[] currentLocationId = new byte[2];
        byte[] lastLockLocationId = new byte[2];
        byte[] ownerThreadId = new byte[8];
        byte[] currentThreadId = new byte[8];
        byte[] status = new byte[1];

        ByteArrayInputStream bis = new ByteArrayInputStream(buf);
        bis.read(mutexId, 0, 2);
        bis.read(currentLocationId, 0, 2);
        bis.read(lastLockLocationId, 0, 2);
        bis.read(ownerThreadId, 0, 8);
        bis.read(currentThreadId, 0, 8);
        bis.read(status, 0, 1);

        return new MutexStatus(byteArrayToInt(mutexId), byteArrayToInt(currentLocationId),
                byteArrayToInt(lastLockLocationId), byteArrayToInt(ownerThreadId),
                byteArrayToInt(currentThreadId), byteArrayToInt(status));
    }

    private void setTimestamp ()
    {

        this._timestamp = System.currentTimeMillis();
    }

    public static int byteArrayToInt (byte[] buf)
    {
        long value = 0;
        for (int i = 0; i < buf.length; i++) {
            value += (buf[i] & 0xff) << (8 * i);
        }

        return (int) value;
    }

    public int getMutexId ()
    {
        return _mutexId;
    }

    public int getCurrentLocationId ()
    {
        return _currentLocationId;
    }

    public int getLastLockLocationId ()
    {
        return _lastLockLocationId;
    }

    public int getOwnerThreadId ()
    {
        return _ownerThreadId;
    }

    public int getCurrentThreadId ()
    {
        return _currentThreadId;
    }

    public int getStatus ()
    {
        return _status;
    }

    public long getTimestamp ()
    {
        return _timestamp;
    }

}
