/*
 * PureTLSUtils.java
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

package us.ihmc.net.puretls;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import java.io.*;
import java.security.PublicKey;
import java.security.interfaces.RSAPublicKey;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;

import us.ihmc.util.ByteArray;
import us.ihmc.util.CertInfo;

import COM.claymoresystems.ptls.*;
import COM.claymoresystems.cert.*;

public class PureTLSUtils
{
    public PureTLSUtils ()
    {
    }
    
    public static String getPeersIPAddr (SSLSocket sslSocket)
    {
        //byte[] ba = sslSocket.getInetAddress().getAddress();
        //return new String (ba[0] + "." + ba[1] + "." + ba[2] + "." + ba[3]);
        return sslSocket.getInetAddress().getHostAddress();
    }
    
    public static X509Cert getPeersCert (SSLSocket sslSocket)
    {        
        try {
            Vector certificateChain = sslSocket.getCertificateChain();
            if (certificateChain == null) {
                System.out.println ("Certificate chain is null for cert obtained from" +  getPeersIPAddr (sslSocket));
                return null;
            }
            return (X509Cert) certificateChain.elementAt (certificateChain.size() - 1);                        
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    
    public static String getPeersCommonName (SSLSocket sslSocket) 
    {
        
        try {
            String commonName = getCommonName (getPeersCert (sslSocket));
            if (commonName == null) {
                System.out.println ("commonName is null for user cert from" +  getPeersIPAddr (sslSocket));
                return null;
            }
            else {
                return commonName;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    
	/** get public key from cached X509 cert that is indexed by the commonName that is part of the DN of the cert itself
	 *  return this as a Java PublicKey and not a PureTLS Public Key, as routines at the Java level utilizing this public key 
	 *  (digital signatures) require it.
	 */
    public static PublicKey getPeersCachedPublicKey (SSLSocket sslSocket, Hashtable certificateCache) 
    {    
        try {
            String commonName = getPeersCommonName (sslSocket);
            if (certificateCache.containsKey (commonName)) {
                CertInfo ci = (CertInfo) certificateCache.get (commonName);
				CertificateFactory f = CertificateFactory.getInstance("X.509", "BC");
				ByteArrayInputStream in = new ByteArrayInputStream (ci.getDERCert());
				X509Certificate cert = (X509Certificate) f.generateCertificate(in);
				PublicKey pk = (RSAPublicKey) cert.getPublicKey();
				if (!(pk instanceof RSAPublicKey)){
					throw new Exception ("getPeersCachedPublicKey: Unable to get proper instance of Public Key");
				}
                return pk;
            }
			else {
				throw new Exception ("Don't have cached cert for " + commonName);
			}
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    
    
    protected static String getCommonName (X509Cert cert) 
    {
        //The outer SEQUENCE is represented by a Vector, each element of which (a SET) is also a Vector. Each element 
        //of the inner Vector (the attributeValuePair) is a String[2] with the first element being the attribute and the second being the value 
        
        Vector distinguishedName = cert.getSubjectName().getName(); //Distinguished Name
        Enumeration outerEnum = distinguishedName.elements();
        while (outerEnum.hasMoreElements()) {
            Vector attributeValuePairs = (Vector) outerEnum.nextElement(); //Attribute Value Pair
            Enumeration innerEnum = attributeValuePairs.elements();
            while (innerEnum.hasMoreElements()) {
                String[] attributeValuePair = (String[]) innerEnum.nextElement();
                if (attributeValuePair[0].equalsIgnoreCase ("commonname") || attributeValuePair[0].equalsIgnoreCase ("cn")) {
                    return attributeValuePair[1];
                }
            }
        }
        return null;
    }
    
    protected static int cachePeersCert (SSLSocket sslSocket, Hashtable certificateCache) 
    {  
        try {
            byte[] DERclientCert = getPeersCert(sslSocket).getDER();
            String IPAddr = getPeersIPAddr (sslSocket);
            String commonName = getPeersCommonName (sslSocket);
            if (certificateCache.containsKey (commonName)) {
                CertInfo ci = (CertInfo) certificateCache.get (commonName);
                if (!ByteArray.areEqual (ci.getDERCert(), DERclientCert)){
                    System.out.println ("Client Cert presented for commonName: " + commonName + " does not match stored cert. Possible Security Violation.");
                }
                else {
                    System.out.println ("Already have this Certificate on file for commonName: " + commonName);
                }
                certificateCache.remove (commonName);
            }
            certificateCache.put (commonName, new CertInfo (commonName, IPAddr, DERclientCert));
            System.out.println ("Caching cert for client system at IP " + IPAddr);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return 0;
    }    
}
