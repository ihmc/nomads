/*
 * SettingFileManager.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.util;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util .Vector ;
import java.util.NoSuchElementException;

/** 
*   Creates a hashtable of hashtables, where each inner ht contains a set of tuples
*   that constitute an entry in the settings file. The outer ht contains this set of
*   inner ht's keyed by the value as specified by the second 'key' parameter
* 
*   @author: Maggie Breedy
*   @version $Revision$
*/
@SuppressWarnings({ "rawtypes", "unchecked" })
public class SettingFileManager
{
	// Class is static - no need to instantiate
    private SettingFileManager()
    {
    }
    
    /** 
     *  Reads the entries from a stream and places them into a hashtable which is indexed
     *  by the specified 'key', and then returned. The values of the returned hashtable
     *  are themselves hashtables which each correspond to an individual Entry.
     */
    public static Hashtable readStream (InputStream is, String key)
    {
        PushbackBufferedReader pbr = new PushbackBufferedReader (new InputStreamReader (is));
        Hashtable dataTable = read (pbr, key);
        return dataTable;
    }
    
    
    
    /** Reads the entries from the settings file designated by 'filePath' and places
     *  them into a hashtable which is indexed by the specified 'key', and then 
     *  returned. The values of the returned hashtable are themselves hashtables which
     *  each correspond to an individual Entry.
     */
    public static Hashtable readFile (String filePath, String key)
    {
        Hashtable dataTable = null;
        try {
            PushbackBufferedReader pbr = new PushbackBufferedReader (new FileReader (filePath));
            dataTable = read (pbr, key);
        }
        catch (FileNotFoundException e) {
            System.out.println ("Warning; The file " + filePath + " is not found");
            e.printStackTrace();
        }
        /*catch (IOException e) {
            e.printStackTrace();
        }*/
        return dataTable;
    }
    
    /** 
     * Reads the specified settings file contents and returns a vector of
     * hashtables, where each hashtable corresponds to a separate Entry - a set
     * of key value pairs.
     */
    public static Vector readFile (String filePath)
    {
        Vector dataTable = new Vector (10);
        try {
            PushbackBufferedReader pbr = new PushbackBufferedReader (new FileReader (filePath));
            String str;
            while ((str = pbr.readLine()) != null) {
                if (str.startsWith ("[Entry]")) {
                    Hashtable entryTable = readEntry (pbr);
                    dataTable.addElement (entryTable);
                }
                else {
                    // Skip until [Entry] is found
                }
            }
            pbr.close();
        } catch (FileNotFoundException e) {
            System.out.println ("Warning; The file " + filePath + " is not found");
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        
        return dataTable;
    }
    
    /**
     *  Reads a line using the pushback buffered reader and returns a hashtable
     *  of individual entries.
     */
    protected static Hashtable read (PushbackBufferedReader pbr, String key)
    {
        Hashtable dataTable = new Hashtable (10);
        try {
            String str;
            while ((str = pbr.readLine()) != null) {
                if (str.startsWith ("[Entry]")) {
                    Hashtable entryTable = readEntry (pbr);
                    String value = (String) entryTable.get (key);
                    dataTable.put (value, entryTable);
                }
                else {
                    // Skip until [Entry] is found
                }
            }
            pbr.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        return dataTable;
    }
    
    /** Writes out the provided hastable of hashtables to the an output stream, where each
     *  hashtable is a series of key value pairs, each to be separated by a "[Entry]"
     *  line.
     */
    public static void writeStream (OutputStream os, Hashtable dataTable)
    {
        PrintWriter pw = new PrintWriter (new OutputStreamWriter (os));
        write (pw, dataTable);
    }
     
    /** Writes out the provided hastable of hashtables to the specified file, where each
     *  hashtable is a series of key value pairs, each to be separated by a "[Entry]"
     *  line.
     */
    public static void writeFile (String filePath, Hashtable dataTable)
    {
        try {
            PrintWriter pw = new PrintWriter(new FileWriter(filePath));
            write (pw, dataTable);
        }
        catch (IOException e) {
            e.printStackTrace();
        }     
    }
    
    /**
     *  Writes the hashtable of hashtables where each hashtable is a series of key value 
     *  pairs.
     */
    protected static void write (PrintWriter pw, Hashtable dataTable)
    {
        Enumeration enu = dataTable.elements();
        while (enu.hasMoreElements()) {
            Hashtable entriesTable = (Hashtable) enu.nextElement();
            pw.println("[Entry]");
            Enumeration e = entriesTable.keys();
            while (e.hasMoreElements()) {
                String name = (String) e.nextElement();
                String value = entriesTable.get(name).toString();
                pw.println (name + "=" + value);
            }
        }
        pw.close();
    }
    
    /** Writes out the provided vector of hashtables to the specified file, where each
     *  hashtable is a series of key value pairs, each to be separated by a "[Entry]"
     *  line.
     */
    public static void writeFile (String filePath, Vector dataTable)
    {
        try {
            PrintWriter pw = new PrintWriter(new FileWriter(filePath));
            Enumeration enu = dataTable.elements();
            while (enu.hasMoreElements()) {
                Hashtable entriesTable = (Hashtable) enu.nextElement();
                pw.println("[Entry]");
                Enumeration e = entriesTable.keys();
                while (e.hasMoreElements()) {
                    String name = (String) e.nextElement();
                    String value = entriesTable.get(name).toString();
                    pw.println (name + "=" + value);
                }
            }
            pw.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }     
    }
    /** Reads individual entries from the file via the provided buffered reader. Will read 
     *  key value pairs and put them into the hashtable until it encounters either the
     *  next [Entry] or is unable to read (EOF). This does allow for embedded '=' signs in
     *  the value, but not the key.
     */
    private static Hashtable readEntry (PushbackBufferedReader pbr)
        throws IOException
    {
        Hashtable policyTable = new Hashtable();
        String str = null;
        while ((str = pbr.readLine()) != null) {
            if (str.startsWith ("[Entry]")) {                
                pbr.pushBackLine (str);
                return policyTable;
            }
            int index = str.indexOf ("=");
            try {
                String name = str.substring (0, index);
                String value = str.substring (index + 1, str.length());
                policyTable.put(name, value);
            }
            catch (NoSuchElementException e) {
                // Ignore any lines that we cannot parse
            }
        }
        return policyTable;
    }
}
