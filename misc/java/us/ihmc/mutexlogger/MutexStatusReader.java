package us.ihmc.netutils;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * Class that reads logs sent by DSPro LoggingMutex class.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */

public class MutexStatusReader
{
    DataInputStream _dis;

    public MutexStatusReader(String filePath) throws FileNotFoundException
    {
        _dis = new DataInputStream(new FileInputStream(new File (filePath)));
    }

    public MutexStatus read () throws IOException
    {
        //initialize new buffer
        byte[] buf = new byte[MutexStatus.MUTEX_BUFFER_SIZE];

        try
        {
            _dis.read(buf);
        }
        catch (IOException e) {
            e.printStackTrace();
            return null;
        }

        //build a MutexStatus object from the data received
        MutexStatus ms = MutexStatus.getMutexStatusFromByteArray(buf);

        return ms;
    }

    public void close ()
    {
        try {
            _dis.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }
}
