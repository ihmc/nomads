/*
 * CertsFileManager.java
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

import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

import java.util.Enumeration;
import java.util.Hashtable;

/** Creates a hashtable of entries keyed by IP Address, where each entries value is the base64 encoded certificate associated with that machine at
 * that IP. 
*/

@SuppressWarnings({ "rawtypes", "unchecked", "unused" })
public class CertsFileManager
{
    public CertsFileManager()
    {
    }
    
    /** Reads the entries from the settings file designated by 'filePath' and places
     *  them into a hashtable which is indexed by the specified 'key', and then 
     *  returned. The values of the returned hashtable are themselves hashtables which
     *  each correspond to an individual Entry.
     */
    public static Hashtable readFile (String filePath)
    {
        Hashtable dataTable = new Hashtable (10);
        try {
            PushbackBufferedReader pbr = new PushbackBufferedReader (new FileReader (filePath));
            String str;
            while ((str = pbr.readLine()) != null) {
                if (str.startsWith ("[Entry]")) {
                    String commonName = pbr.readLine();
                    String IPAddr = pbr.readLine();
                    String beginCertDemarc = pbr.readLine();
                    if (!beginCertDemarc.equalsIgnoreCase ("-----BEGIN CERTIFICATE-----")) {
                        System.out.println ("Error reading file - begin Cert Demarc doesn't match: " + beginCertDemarc);
                    }
                    String cert = "";
                    while (((str = pbr.readLine()) != null) && !str.equalsIgnoreCase("-----END CERTIFICATE-----")) {
                        cert += str;
                    }
                    byte[] DERencodedCert = Base64Transcoders.convertB64StringToByteArray (cert);
                    //ByteArray.stringToByteArray (original, DERencodedCert, 0, original.length());
                    dataTable.put (commonName, new CertInfo (commonName, IPAddr, DERencodedCert));
                    if (debug >= 1) {
                        System.out.println ("read in commonName: " + commonName);
                        System.out.println ("read in IPAddr: " + IPAddr);
                        System.out.println ("read in DERencodedCert of length: " + DERencodedCert.length);
                    }
                    else if (debug >= 3) {
                        ByteArray.printByteArrayAsHexColumns (DERencodedCert);
                    }
                    if (str == null) {
                        break;
                    }
                    else if (str.startsWith("[Entry]")) {
                        pbr.pushBackLine (str);
                    }
                }
            }
            pbr.close();
        }
        catch (FileNotFoundException e) {
            //System.out.println ("Warning; The file " + filePath + " is not found");
            //e.printStackTrace();
            return dataTable;
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return dataTable;
    }
    
    /** Writes out the provided hashtable to a text file.
     */
    public static void writeFile (String filePath, Hashtable dataTable)
    {
        try {
            PrintWriter pw = new PrintWriter (new FileWriter (filePath));
            Enumeration e = dataTable.keys();
            while (e.hasMoreElements()) {
                String commonName = (String) e.nextElement();
                if (debug >= 1) {
                    System.out.println ("writeFile, got commonName: " + commonName);
                }
                CertInfo ci = (CertInfo) dataTable.get (commonName);
                String IPAddr = ci.getIPAddress();
                String DERcert = Base64Transcoders.convertByteArrayToB64String (ci.getDERCert());
                if (debug >= 1) {
                    System.out.println ("writeFile, got cert: " + DERcert);
                }
                pw.println ("[Entry]");
                pw.println (commonName);
                pw.println (IPAddr);
                pw.println ("-----BEGIN CERTIFICATE-----");
                pw.println (DERcert);
                pw.println ("-----END CERTIFICATE-----");
            }
            pw.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }     
    }
    private final static int debug = 0;
}
