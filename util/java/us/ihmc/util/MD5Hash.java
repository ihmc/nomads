/*
 * MD5Hash.java
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

/** This is using the Sun provided MD5 implementation. It may be advantageous to 
 * utilize the Cryptix impl, as we utilize more cryptography.
 */

import java.io.File; 
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class MD5Hash 
{ 
    public static String getB64HashValue (String stringToDigest) 
    {
        try {
            MessageDigest md5 = MessageDigest.getInstance("MD5"); 
            md5.update(stringToDigest.getBytes()); 
//            Base64Transcoders b64tc = new Base64Transcoders();    
//            return b64tc.convertByteArrayToB64String(md5.digest());
            return Base64Transcoders.convertByteArrayToB64String(md5.digest());
        } 
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    
    
    /**
     * Opens file, reads all bytes within the file while building an MD5 hash.
     * Returns Base64Transcoded String of final MD5 hash.
     * 
     * @param file -- File
     * @return String -- Base64Encoding of MD5 hash
     */
    public static String getB64HashValue (File file)
    {
        String funcName = "getB64HashValue: ";
          
        if (file.exists()) {
            
            try {
                MessageDigest md5 = MessageDigest.getInstance("MD5"); 
                FileInputStream fis = new FileInputStream (file);
                                       
                // offset - start offset in array b at which data is written. 
                // eof returns -1; bytesRead = -1.  Will read up to KB from 
                // file input stream (bytesRead), or if eof is reached will return
                // number of bytes read up to the eof.        
                int offset = 0;
                int bytesRead = 0;
                while(bytesRead > 0){
                    byte b[] = new byte[KB];                
                    bytesRead = fis.read(b, offset, KB);                   
                    md5.update(b, offset, bytesRead);
                }                        
                fis.close();
                
                return Base64Transcoders.convertByteArrayToB64String(md5.digest());
                
            } 
            catch (FileNotFoundException e) {
                e.printStackTrace();
            }
            catch (IOException e) {
                e.printStackTrace();
            } 
            catch (NoSuchAlgorithmException e) {
                e.printStackTrace();
            }
        }
        else {
            System.out.println(funcName + "File does not exist");
        }

        
        return null;
    }

    public static String getB64HashValue (byte b[])
    {
        try {
            MessageDigest md5 = MessageDigest.getInstance("MD5");
	    md5.update (b, 0, b.length);
	    return Base64Transcoders.convertByteArrayToB64String(md5.digest());
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }

        return null;
    }
    
    private static final int KB = 1024;
} 


