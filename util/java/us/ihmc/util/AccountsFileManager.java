/*
 * AccountsFileManager.java
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
 */

package us.ihmc.util;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Vector;

/**
 *  AccountsFileManager is a utility class to get or store information 
 *  about from the accounts file.
 *
 *  @author: Maggie Breedy (NOMADS team)
 *  @version: $Revision: 1.7 $
 */

@SuppressWarnings({ "rawtypes", "unchecked" })
public class AccountsFileManager
{
    /** Reads an Input Stream via the SettingFileManager.readStream.
     *  The hashtable of hashtables is keyed by the entryKey, a concatenation of userid
     *  and rank, where rank serves to differentiate between individual account entries
     *  with the same userid. Each ht is then placed into the final structure via
     *  the addDataToTable method, where the top level ht contains (userid, vector) pairs
     *  where each vector contains the hashtables of the individual account entries 
     *  associated with said userid.
     */
    public static Hashtable readAccountsStream (InputStream is)
    {        
         // Read the Accounts file.
	    String keyLabel = "entryKey";
	    Hashtable itemTable = SettingFileManager.readStream (is, keyLabel);
	    if (itemTable != null) {
            Enumeration enu = itemTable.keys();
	        while (enu.hasMoreElements()) {
	            String key = (String) enu.nextElement();
	            StringTokenizer st = new StringTokenizer(key, "_");
	            String userid = null;
	            String rankNum = null;
	            while (st.hasMoreTokens()) {
	                userid = (String) st.nextToken();
	                rankNum = (String) st.nextToken();
	            }
                if ((userid == null) || (rankNum == null)) {
                    //perhaps we should just discard this entry as opposed to aborting the entire process.
                    return null;
	            }
                Hashtable ht = new Hashtable (7);
	            ht = (Hashtable) itemTable.get(key);
	            _newItemHashtable = addDataToTable (_newItemHashtable, userid, rankNum, ht);
            }
        }
        else {
	        System.out.println ("<--->The Item table is empty");
	        itemTable = null;
	    }
        return _newItemHashtable;
    }
    
    /** Reads the contents of the specified accounts file via the SettingFileManager.readFile.
     *  The hashtable of hashtables is keyed by the entryKey, a concatenation of userid
     *  and rank, where rank serves to differentiate between individual account entries
     *  with the same userid. Each ht is then placed into the final structure via
     *  the addDataToTable method, where the top level ht contains (userid, vector) pairs
     *  where each vector contains the hashtables of the individual account entries 
     *  associated with said userid.
     */
    public static Hashtable readAccountsFile (String accountsFileName)
    {        
         // Read the Accounts file.
	    String keyLabel = "entryKey";
	    Hashtable itemTable = SettingFileManager.readFile (accountsFileName, keyLabel);
	    if (itemTable != null) {
            Enumeration enu = itemTable.keys();
	        while (enu.hasMoreElements()) {
	            String key = (String) enu.nextElement();
	            StringTokenizer st = new StringTokenizer(key, "_");
	            String userid = null;
	            String rankNum = null;
	            while (st.hasMoreTokens()) {
	                userid = (String) st.nextToken();
	                rankNum = (String) st.nextToken();
	            }
                if ((userid == null) || (rankNum == null)) {
                    //perhaps we should just discard this entry as opposed to aborting the entire process.
                    return null;
	            }
                Hashtable ht = new Hashtable (7);
	            ht = (Hashtable) itemTable.get(key);
	            _newItemHashtable = addDataToTable (_newItemHashtable, userid, rankNum, ht);
            }
        }
        else {
	        System.out.println ("<--->The Item table is empty");
	        itemTable = null;
	    }
        return _newItemHashtable;
    }
    
    /** 
     * Adds the data from the file to the specified topLevelHT which contains vectors of hashtables,
     * ordered by userid.
    */
	public static Hashtable addDataToTable (Hashtable topLevelHT, String keyName, String rank, Hashtable ht)
	{
	    int rankVectorIndex = Integer.parseInt (rank);
	    if (!topLevelHT.containsKey(keyName)) {
            Vector rankVector = new Vector();
            rankVector.setSize (rankVectorIndex + 1);
            rankVector.setElementAt (ht, rankVectorIndex);
	        topLevelHT.put(keyName, rankVector);
	    }
	    else {
	        Vector rankVector = (Vector) topLevelHT.get (keyName);
            if (rankVector.size() < rankVectorIndex + 1) {
                rankVector.setSize (rankVectorIndex + 1);
            }
	        rankVector.setElementAt (ht, rankVectorIndex);
        }
        return topLevelHT;
	}
    
    /** Write the accounts file to an output stream. This method will take each entry in the
     *  memory structure and make sure it has a correctly formed "entryKey" field, and 
     *  transform the in memory structure to a hashtable(entryKey, ht)of hashtables(name, value)
     *  before writing it to an output stream, via the SettingFileManager. 
     */
    public static void writeAccountsStream (Hashtable accountsTable, OutputStream os)
    {
        Hashtable itemTable = new Hashtable (7);
        for (Enumeration e = accountsTable.keys(); e.hasMoreElements();) {
            String name = (String) e.nextElement();
            Vector v = (Vector) accountsTable.get(name);
	        for (Enumeration en = v.elements(); en.hasMoreElements();) {
	            Hashtable ht = (Hashtable) en.nextElement();
	            String rank = (String) ht.get("entryRank");
	            String key = name.concat("_").concat(rank);
	            ht.put("entryKey", key);
	            itemTable.put(key, ht);
            }
        }
        // Writes all the changes to this output stream.
        SettingFileManager.writeStream (os, itemTable);
    }

    /** Write the accounts file out to disk. This method will take each entry in the
     *  memory structure and make sure it has a correctly formed "entryKey" field, and 
     *  transform the in memory structure to a hashtable(entryKey, ht)of hashtables(name, value)
     *  before writing it out to disk, via the SettingFileManager. 
     */
    public static void writeAccountsFile (Hashtable accountsTable, String accountsFileName)
    {
        Hashtable itemTable = new Hashtable (7);
        for (Enumeration e = accountsTable.keys(); e.hasMoreElements();) {
            String name = (String) e.nextElement();
            Vector v = (Vector) accountsTable.get(name);
	        for (Enumeration en = v.elements(); en.hasMoreElements();) {
	            Hashtable ht = (Hashtable) en.nextElement();
	            String rank = (String) ht.get("entryRank");
	            String key = name.concat("_").concat(rank);
	            ht.put("entryKey", key);
	            itemTable.put(key, ht);
            }
        }
        // Writes all the changes back into the accounts file.
        SettingFileManager.writeFile (accountsFileName, itemTable);
    }
    
    // Class Variables
    protected static Hashtable _newItemHashtable = new Hashtable (7);
}
