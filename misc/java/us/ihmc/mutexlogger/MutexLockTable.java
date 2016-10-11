package us.ihmc.netutils;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Table representing the list of the current locks
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */

public class MutexLockTable extends ConcurrentHashMap
{
    private static String _tableFilePath;

    public MutexLockTable(String filePath)
    {
        _tableFilePath = filePath;
    }

    public boolean checkForLocks ()
    {
        boolean foundLock = false;

        for (Object value : this.values()) {

            MutexStatus ms = (MutexStatus) value;

            if (((System.currentTimeMillis() - ms.getTimestamp()) / 1000 > 300) && ms.getStatus() != 3) {    //if the
                                                                                                             // Thread is in a tryLock
                                                                                                             // or a lock acquired for more than 5 mins (300 sec)
                foundLock = true;
            }
        }

        if (foundLock) {

            printOnFile(_tableFilePath);
            Logger.getLogger(MutexLockTable.class.getName()).log(Level.INFO,
                    "Table written on file: " + _tableFilePath);
        }

        return foundLock;
    }

    public void printOnFile (String filePath)
    {

        try {
            FileWriter fStream = new FileWriter(filePath);
            BufferedWriter out = new BufferedWriter(fStream);
            out.write("MutexId CurrentLocationId CurrentThreadId LastLockLocationId OwnerThreadId Status Time");
            out.newLine();

            for (Object value : this.values()) {

                MutexStatus ms = (MutexStatus) value;

                if (ms.getStatus() != 3) {

                    out.write(ms.getMutexId() + "\t\t\t");
                    out.write(ms.getCurrentLocationId() + "\t\t\t");
                    out.write(ms.getCurrentThreadId() + "\t\t\t");
                    out.write(ms.getLastLockLocationId() + "\t\t\t");
                    out.write(ms.getOwnerThreadId() + "\t\t\t");

                    if (ms.getStatus() == 1)
                        out.write("TRY LOCK" + "\t\t\t");
                    else
                        out.write("LOCK ACQUIRED" + "\t\t\t");

                    out.write((System.currentTimeMillis() - ms.getTimestamp()) / 1000 + "\t"); //seconds waiting
                    out.newLine();
                }
            }

            out.close();

        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

}