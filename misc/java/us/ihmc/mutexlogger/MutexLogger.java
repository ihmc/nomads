package us.ihmc.netutils;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Class that coordinates actions on the MutexLockTable and MutexStatusReader objects.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class MutexLogger extends Thread
{
    private MutexLockTable _mLockTable;
    private MutexStatusReader _msReader;
    private Thread _tCheckLocks;

    /**
     * Default constructor. Initialize a new MutexLogger in File mode.
     *
     * @param mutexSrcRawFile the raw byte source file for mutex status
     * @param mutexDestLogFile the destination log file, plain text
     * @throws FileNotFoundException
     */
    public MutexLogger (String mutexSrcRawFile, String mutexDestLogFile) throws FileNotFoundException
    {
        _msReader = new MutexStatusReader(mutexSrcRawFile);
        _mLockTable = new MutexLockTable(mutexDestLogFile);
    }
    
    @SuppressWarnings("unchecked")	
    @Override
    public void run ()
    {
        _tCheckLocks = new Thread(new Runnable()
        {
            @Override
            public void run ()
            {

                while (_tCheckLocks != null) {

                    _mLockTable.checkForLocks();

                    try {
                        Thread.sleep(10000);
                        Logger.getLogger(MutexLockTable.class.getName()).log(Level.INFO, "Checking Mutex table for " +
                                "locks...(" + _mLockTable.size() + " objects)");
                    }
                    catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        });

        //starting check thread.
        _tCheckLocks.start();

        while (!this.isInterrupted()) {

            try {
                MutexStatus ms = _msReader.read();
                //fetch key fields
                int mutexId = ms.getMutexId();
                int currentLocationId = ms.getCurrentLocationId();
                int currentThreadId = ms.getCurrentThreadId();

                _mLockTable.put(new MutexKey(mutexId, currentLocationId, currentThreadId), ms);
            }
            catch (IOException e) {
                e.printStackTrace();
            }
        }

        //close reader
        _msReader.close();
    }

    public static void main (String[] args)
    {

        MutexLogger mutexLogger = null;

        try {
            mutexLogger = new MutexLogger("mutex.raw", "mutex.log");
        }
        catch (FileNotFoundException e) {
            e.printStackTrace();
        }

        if (mutexLogger != null) {
            mutexLogger.start();
        }
        else {
            System.out.println("Error: MutexLogger was not initialized correctly");
        }
    }
}
