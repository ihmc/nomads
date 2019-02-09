package us.ihmc.android.aci.dspro.util;

import java.io.*;
import java.util.ArrayList;

/**
 *  Class <code>StringList</code> allows a text file to be treated as a 
 *  collection of Strings.
 *
 * @author     Enrico Casini (ecasini@ihmc.us)
 */
public class StringList extends ArrayList<String> {

    /**
     *Constructor for the StringList object
     */
    public StringList() {
        super();
    }

    public void read(InputStream in) throws IOException
    {
        read(new InputStreamReader(in));
    }

    /**
     * Sort this <code>StringList</code> in a natural order.
     */
    public void sort()
    {
        java.util.Collections.sort(this);
    }


    public void read(Reader r) throws IOException
    {
        String line = null;
        BufferedReader in = null;
        try {
            in = new BufferedReader(r);
            while ((line = in.readLine()) != null) {
                add(line);
            }
        }
        catch (IOException e) {
            e.printStackTrace();
            throw e;
        }
        finally {
            try {
                in.close();
            }
            catch (IOException e) {
                /* ignore */
            }
        }

    }

    /**
     *  Constructor for the StringList object
     *
     * @param  fileName  The file to open
     */
    public void read(String fileName) throws IOException
    {
        try {
            read(new FileReader(fileName));
        }
        catch(IOException e) {
            e.printStackTrace();
            throw e;
        }
    }

    /**
     *  Save the String to named file
     *
     * @param  fileName  The name of the file to save to
     */
    public void save(String fileName) {
        PrintWriter out = null;
        try {
            out = new PrintWriter(new FileOutputStream(fileName), true);
            for (int i = 0; i < size(); i++) {
                out.println((String) get(i));
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        finally {
            if (out != null) {
                out.close();
            }
        }
    }
} 