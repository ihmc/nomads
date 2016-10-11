/*
 * MASTPKITools.java
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

package us.ihmc.util.crypto;

import java.io.File;
import java.io.IOException;

import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.cert.X509Certificate;

import us.ihmc.comm.CommHelper;
import us.ihmc.util.ConfigLoader;
//import us.ihmc.util.ConfigHelper;

/**
 * @author Tom Cowin <tcowin@ihmc.us>
 *  MAST specific Java tools for PKI operations. These methods are primarily for use by agents on top of
 *  a Sun Java VM, such as is the case with the MASTConsole. Certain operations, like attempting to invoke
 *  the JCE Providers CertificateGenerator, will not work in the Aroma VM. This operation is important to
 *  generating a certificate from a bytestream read from disk, as in the case of attempting to extract a cert
 *  from a PEM formatted file. This is more easily done when you have access to the SSLSocket, as in the PureTLS
 *  operations, or in C++, with OpenSSL support. We use the Bouncy Castle JCE Provider as this implementation is
 *  readily available for the Classic VM (aka JDK 1.2.X). We haven't used the PureTLS implementation of some of the
 *  digital signature operations defined herein as they state that they are not complete - message digest info has not
 *  been included in their signatures. 
 *  These digital signature verification protocols are mirrored in the C++ implementation.
 * $Date: 2016/06/09 20:02:46 $
 * $Release: $
 */
public class MASTPKITools
{
	
	public static String getPathToClientKeyStore()
		throws IOException
	{
		String homeDir = null;
		if ((homeDir = System.getProperty ("mast.home")) == null) {
			if ((homeDir = System.getProperty ("nomads.home")) == null) {
				throw new IOException ("Home dir not found; set Java System Property mast.home or nomads.home on command line like eg.: -Dmast.home=c:\\Program Files\\IHMC MAST");
			}
		}
		return (homeDir + File.separator + "lib" + File.separator + "security" + File.separator + _clientKeyStoreFile);
	}
		
	
	/** Obtain the RSA PrivateKey from the PEM formatted file stored 
     * on the local disk for this particular node. We use only the 'client'
     * file in MAST as we cannot have different keys for client vs server 
     * operation as this would break our PKI Verification protocol. The private
     * key is encrypted and password protected.
     * 
     * @return PrivateKey in the form of a java.security.PrivateKey
     */
    public static PrivateKey getClientPrivateKey()
    {
        String keyPassword = null;
		PrivateKey privateKey = null;
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        try {
            //keyPassword = (String) ConfigHelper.getSystemProperties().get ("keypass");
            keyPassword = cloader.getProperty("keypass");
            if (keyPassword == null) {
                keyPassword = "foobar";
            }       
			privateKey = PKIUtils.getPrivateKeyFromPEMFile (getPathToClientKeyStore(), keyPassword, _jceProvider);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return privateKey;
    }
	
    /** Obtain the commonName from the X509 Certificate stored in the PEM formatted file stored 
     * on the local disk for this particular node. Works in Sun VM.
     * 
     * @return String containing the nodes commonName
     * C=US,ST=Florida,L=Pensacola,O=UWF,OU=IHMC,CN=prospero_sk
     */
    public static String getCommonNameFromCert (X509Certificate certificate) 
    {
		String commonName = null;
		try {
			String distinguishedName = certificate.getSubjectDN().getName();
			int index = -1;
			if ((index = distinguishedName.indexOf ("CN=")) != -1) {
				commonName = distinguishedName.substring (index + 3, (distinguishedName.length()));
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return commonName;
    }
	
	/** Obtain the commonName from the X509 Certificate stored in the PEM formatted file stored 
     * on the local disk for this particular node. Works in Sun VM.
     * 
     * @return String containing the nodes commonName
     * C=US,ST=Florida,L=Pensacola,O=UWF,OU=IHMC,CN=prospero_sk
     */
    public static String getClientCommonName() 
    {
		String commonName = null;
		try {
			commonName = getCommonNameFromCert (PKIUtils.getCertFromPEMFile (getPathToClientKeyStore(), _jceProvider));
		} catch (Exception e) {
			e.printStackTrace();
		}
		return commonName;
    }
	
	/** Obtain the RSA PublicKey from the PEM formatted file stored 
     * on the local disk for this particular node. We use only the 'client'
     * file in MAST as we cannot have different keys for client vs server 
     * operation as this would break our PKI Verification protocol.
     * 
     * @return PublicKey in the form of a java.security.PublicKey
     */
    public static PublicKey getClientPublicKey() 
    {
		PublicKey publicKey = null;
		try {
			publicKey = PKIUtils.getPublicKeyFromPEMFile (getPathToClientKeyStore(), _jceProvider);
		} catch (Exception e) {
			e.printStackTrace();
		}
        return publicKey;
    }

    /** Perform a cross validation of stored key pairs to ensure that the peer is
     * the same peer that you initially exchanged keys with during the bootstrap
     * process. The sequence here is what is expected of the client, or the initiator
     * of the SSL session.
     * 
     * Digital Signatures are a function of key size, and we are using 
     * a key size of 2048, and the signatures are 256 bytes. We allow for
     * a potential sig size of 4x that value here for future growth in key size.
     * 
     * @param commHelper pointer to instantiated helper class that will handle socket comm
     * @param peersPublicKey the RSA public key that was cached and associated with our current
     *        peer by the commonName of his X509 DistinguishedName field.
     * @return boolean that specifies whether or bidirectional PKI verification was successful. 
     *         Comm should not continue if this is not true.
     */
    public static boolean clientPKIVerification (CommHelper commHelper, PublicKey peersPublicKey) 
    {

        try {
            if (commHelper == null) {
                throw new Exception ("commHelper parameter is null");
            } 
            else if (peersPublicKey == null) {
                throw new Exception ("peersPublicKey parameter is null");
            }
            
            //given that we have to check in caller to determine if server side desires verification, this string
            //gets taken off the input stream at that level, and won't be available to this method
            //commHelper.receiveMatch ("VERIFY"); 
            
            commHelper.sendBlock (PKIUtils.generateDigitalSignature (getClientPrivateKey(), _messageText.getBytes(), "SHA1withRSA", _jceProvider));
            
            commHelper.receiveMatch ("VERIFIED");

            commHelper.sendLine ("VERIFY");

            if (!PKIUtils.verifyDigitalSignature (commHelper.receiveBlock(), peersPublicKey, _messageText.getBytes(), "SHA1withRSA", _jceProvider)) {
                commHelper.sendLine ("INVALID SIGNATURE");
                return false;
            }
            else {
                commHelper.sendLine ("VERIFIED");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return true;
    }
            
    /** Perform a cross validation of stored key pairs to ensure that the peer is
     * the same peer that you initially exchanged keys with during the bootstrap
     * process. The sequence here is what is expected of the server, or the receiver
     * of the SSL session.
     * 
     * Digital Signatures are a function of key size, and we are using 
     * a key size of 2048, and the signatures are 256 bytes. We allow for
     * a potential sig size of 4x that value here for future growth in key size.
     * 
     * @param commHelper pointer to instantiated helper class that will handle socket comm
     * @param peersPublicKey the RSA public key that was cached and associated with our current
     *        peer by the commonName of his X509 DistinguishedName field.
     * @return boolean that specifies whether or bidirectional PKI verification was successful. 
     *         Comm should not continue if this is not true.
     */
    public static boolean serverPKIVerification (CommHelper commHelper, PublicKey peersPublicKey) 
    {

        try {
            if (commHelper == null) {
                throw new Exception ("commHelper parameter is null");
            } 
            else if (peersPublicKey == null) {
                throw new Exception ("peersPublicKey parameter is null");
            }
            commHelper.sendLine ("VERIFY");
            
            if (!PKIUtils.verifyDigitalSignature (commHelper.receiveBlock(), peersPublicKey, _messageText.getBytes(), "SHA1withRSA", _jceProvider)) {
                commHelper.sendLine ("INVALID SIGNATURE");
                return false;
            }
            else {
                commHelper.sendLine ("VERIFIED");
            }

            commHelper.receiveMatch ("VERIFY");
            
            commHelper.sendBlock (PKIUtils.generateDigitalSignature (getClientPrivateKey(), _messageText.getBytes(), "SHA1withRSA", _jceProvider));
            
            commHelper.receiveMatch ("VERIFIED");

        } catch (Exception e) {
            e.printStackTrace();
        }
        return true;
    }

    private static String _clientKeyStoreFile = "client.pem";
    private static String _messageText = "this is a test!";    
    private static String _jceProvider = "BC";    
}
