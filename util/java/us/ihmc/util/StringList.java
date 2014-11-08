/*
 * StringList.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.util;

import java.io.*;
import java.util.ArrayList;

/**
 *  Class <code>StringList</code> allows a text file to be treated as a 
 *  collection of Strings.
 *
 * @author     Enrico Casini (ecasini@ihmc.us)
 */
public class StringList extends ArrayList<String>
{
	private static final long serialVersionUID = 1L;

	/**
     *Constructor for the StringList object
     */
    public StringList() {
        super();
    }

    public void read(InputStream in) {
        read(new InputStreamReader(in));
    }


    public void read(Reader r) {
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
    public void read(String fileName) {
        try {
            read(new FileReader(fileName));
        }
        catch(IOException e) {
            e.printStackTrace();
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
