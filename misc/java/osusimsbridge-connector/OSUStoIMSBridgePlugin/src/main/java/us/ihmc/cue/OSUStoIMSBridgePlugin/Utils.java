package us.ihmc.cue.OSUStoIMSBridgePlugin;

import java.io.*;

/**
 * @author Rita Lenzi (rlenzi@ihmc.us) - 9/23/2016
 */
public class Utils
{
    /**
     * Closes the writer if necessary
     * @param writer writer instance
     */
    public static void safeClose (Writer writer)
    {
        if (writer == null) {
            return;
        }

        try {
            writer.flush();
            writer.close();
        }
        catch (IOException e) {
        }
    }

    /**
     * Closes the input stream if necessary
     * @param is input stream instance
     */
    public static void safeClose (InputStream is)
    {
        if (is == null) {
            return;
        }

        try {
            is.close();
        }
        catch (IOException e) {
        }
    }

    /**
     * Closes the reader if necessary
     * @param reader reader instance
     */
    public static void safeClose (Reader reader)
    {
        if (reader == null) {
            return;
        }

        try {
            reader.close();
        }
        catch (IOException e) {
        }
    }

    /**
     * Closes the output stream if necessary
     * @param os output stream instance
     */
    public static void safeClose (OutputStream os)
    {
        if (os == null) {
            return;
        }

        try {
            os.flush();
            os.close();
        }
        catch (IOException e) {
        }
    }
}
