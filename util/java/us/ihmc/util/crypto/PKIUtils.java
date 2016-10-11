/*
 * PKIUtils.java
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

// The functions get(Secret)Key() and Read(Private?)SecretKey() were taken from the file
// PEMReader.java from the bouncycastle.org JCE provider. These two functions
// are governed by the following copyright:
// Copyright (c) 2000 The Legion Of The Bouncy Castle (http://www.bouncycastle.org)

// Permission is hereby granted, free of charge, to any person obtaining a copy of 
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to 
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to 
// do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
// SOFTWARE. 
// *** End of BouncyCastle copyright

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.StringReader;
import java.security.Key;
import java.security.KeyFactory;
import java.security.MessageDigest;
import java.security.PrivateKey;
import java.security.Provider;
import java.security.PublicKey;
import java.security.Security;
import java.security.Signature;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.security.spec.DSAPrivateKeySpec;
import java.security.spec.KeySpec;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.RSAPrivateCrtKeySpec;
import java.util.StringTokenizer;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;

import org.bouncycastle.util.encoders.Hex;
import org.bouncycastle.util.encoders.Base64;
import org.bouncycastle.asn1.DERInteger;
import org.bouncycastle.asn1.DERInputStream;
import org.bouncycastle.asn1.ASN1Sequence;
public class PKIUtils
{   
    /** Generate a digital signature of the provided data, given the RSA Private Key 
     *  provided, and utilizing the algorithm so specified. 
     * 
     * In MAST operation, this is called from within the MAST package tree, and we 
     * use the same static string of text for our data in both client and server as 
     * well as Java and C++ implementations. We use JCE and the Bouncy Castle provider here
     * as the PureTLS signature generation functions did not include the MD data. The algorithm 
     * utilized in this context is SHA1withRSA. This is sent using the clients own privateKey,
     * and then verified on the server end using the publicKey that the server had previously cached
     * for this client. I believe the MD chosen here should be the same as the one used to create
     * the keys initially.
     * 
     * @param privateKey RSA privateKey from java.security.PrivateKey/java.security.spec.RSAPrivateKeySpec
     * @param byte{} data containing the data to be signed.
     * @param String algorithm the algorithm to be utilized in signing the data.
     * @param String jceProvider the crypto provider to use to obtain the algorithm and actual crypto impl
     * @return byte array containing the digital signature 
     */
    public static byte[] generateDigitalSignature (PrivateKey privateKey, byte[] data, String algorithm, String jceProvider)
    {
        try {
            Signature signature = Signature.getInstance (algorithm, jceProvider);
            signature.initSign (privateKey);
            signature.update (data);
            return signature.sign();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
	
    /** Verify a digital signature given the RSA Public Key, original subject data, 
     * algorithm and jceProvider provided. 
     * 
     * In MAST operation, this is called from within the MAST package tree, and we 
     * use the same static string of text for our data in both client and server as 
     * well as Java and C++ implementations. We use JCE and the Bouncy Castle provider here
     * as the PureTLS signature verification functions did not include the MD data. The algorithm 
     * utilized in this context is SHA1withRSA. This is sent using the clients own privateKey,
     * and then verified on the server end using the publicKey that the server had previously cached
     * for this client. I believe the MD chosen here should be the same as the one used to create
     * the keys initially.
     * 
     * @param publicKey RSA publicKey from java.security.PublicKey/java.security.spec.RSAPublicKeySpec
     * @param byte[] signature contains the digital signature to be verified
     * @param byte[] data containing the data that was originally signed.
     * @param String algorithm the algorithm to be utilized in verifying the signature
     * @param String jceProvider the crypto provider to use to obtain the algorithm and actual crypto impl
     * @return boolean to indicate whether or not the signature was actually verified (true) or not (false)
     */
    public static boolean verifyDigitalSignature (byte[] digitalSignature, PublicKey publicKey, byte[] data, String algorithm, String jceProvider)
    {
        try {
            Signature signature = Signature.getInstance (algorithm, jceProvider);
            signature.initVerify (publicKey);
            signature.update (data);
            return signature.verify (digitalSignature);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }
    /**
     * Reads the user certificate originally generated by OpenSSL from a PEM formatted file
     * and transform into a java.security.cert.X509Certificate.
     * 
     * These methods will not work in Oasis - Oasis fails with a msg to the affect that it cannot invoke a 
     * method with a return type of 20 - VirtualMachine error stemming from java.security.cert.CertificateGenerator
     * 
     * These were really implemented to assist in the use of a Sun VM underlying the MASTConsole, and to ensure that
     * we could use SSL, and accomplish our cached certificate validation with a MASTKernel on the other end.
     * These will also be useful in getting away from PureTLS as our SSL implementation, and replacing it with
     * Sun's JSSE, as that appears to be the one that is currently being maintained.
     * 
     * @param String fileName PEM formatted textfile that contains the certificate
     * @return X509Certificate java.security.cert.X509Certificate extracted from the file
     */
    public static X509Certificate getCertFromPEMFile (String fileName, String jceProvider)
    {
        if (_debug) {
            System.out.println ("getCertFromPEMFile: Reading cert from " + fileName);
        }
        try {
            File inputFile = new File (fileName);
            BufferedReader inputReader = new BufferedReader (new FileReader (inputFile));
            String inputString = extractPEMDelimitedBlock (inputReader, "CERTIFICATE", 0);
			CertificateFactory certificateFactory = null;
			if (jceProvider.startsWith ("Sun") || jceProvider.startsWith ("SUN")) {
				certificateFactory = CertificateFactory.getInstance ("X.509");
			}
			else {
				certificateFactory = CertificateFactory.getInstance ("X.509", jceProvider);
			}
            X509Certificate clientCert = (java.security.cert.X509Certificate) certificateFactory.generateCertificate (new ByteArrayInputStream (inputString.getBytes()));
			if (clientCert == null) {
				throw new Exception ("PKIUils.getCertFromPEMFile: Generated X509 Certificate is null");
			}
            return clientCert;
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        return null;
    }
	
	
    public static X509Certificate[] getCertChainFromPEMFile (String fileName, String jceProvider)
	{
        if (_debug) {
            System.out.println ("getCertChainFromPEMFile: Reading cert from " + fileName);
        }
        try {
            File inputFile = new File (fileName);
            BufferedReader inputReader = new BufferedReader (new FileReader (inputFile));
			int numCerts = countPEMDelimitedBlocks (inputReader, "CERTIFICATE");
			X509Certificate[] clientCert = new X509Certificate[numCerts];
			CertificateFactory certificateFactory = null;
			if (jceProvider.startsWith ("Sun") || jceProvider.startsWith ("SUN")) {
				certificateFactory = CertificateFactory.getInstance ("X.509");
			}
			else {
				certificateFactory = CertificateFactory.getInstance ("X.509", jceProvider);
			}
			for (int i = 0; i < numCerts; i++) {
				String inputString = extractPEMDelimitedBlock (inputReader, "CERTIFICATE", i);
				if (inputString == null) {
					throw new Exception ("Unable to extract PEM delimited block with index " + i + " from " + fileName);
				}
				clientCert[i] = (java.security.cert.X509Certificate) certificateFactory.generateCertificate (new ByteArrayInputStream (inputString.getBytes()));
				if (clientCert[i] == null) {
					throw new Exception ("PKIUils.getCertChainFromPEMFile: Generated X509 Certificate is null");
				}
                if (_debug) {
                    System.out.println ("getCertChainFromPEMFile: Adding " + i + "th cert: " + clientCert[i].getSubjectDN().getName() + " to array");
                }
			}
            return clientCert;
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        return null;
	}

	/**
	 * Reads the public key from a PEM formatted file (via the cert)
	 * @see getCertFromPEMFile (String)
	 * 
     * @param String fileName PEM formatted textfile that contains the certificate
     * @param String that designates the JCE Provider
     * @return PublicKey java.security.PublicKey extracted from the file
	 */
	public static PublicKey getPublicKeyFromPEMFile (String fileName, String jceProvider)
	{
        _jceProvider = jceProvider;
		if (_debug) {
			System.out.println ("getPublicKeyFromPEMFile: Reading public key from " + fileName);
		}
		return getCertFromPEMFile (fileName, _jceProvider).getPublicKey();
	}
    
    /**
     * Reads the user private key from a file
     */
    public static PrivateKey getPrivateKeyFromPEMFile (String fileName, String secretKey, String jceProvider)
    {
        _secretKey = secretKey;
        _jceProvider = jceProvider;
        try {
            BufferedReader inputReader = new BufferedReader (new FileReader (new File (fileName)));
            inputReader.mark (10240); //twice the size of std client.pem file.
            String inputString = null;
            
            //text header of stored private key block indicates type of stored format
            if ((inputString = extractPEMDelimitedBlock (inputReader, "RSA PRIVATE KEY", 0)) != null) {
                return readPrivateKey ("RSA", inputString);
            }
            inputReader.reset();
            if ((inputString = extractPEMDelimitedBlock (inputReader, "PRIVATE KEY", 0)) != null) {
                return readPrivateKey ("RSA-PKCS8", inputString);
            }
            inputReader.reset();
            if ((inputString = extractPEMDelimitedBlock (inputReader, "DSA PRIVATE KEY", 0)) != null) {
                return readPrivateKey ("DSA", inputString);
            }
        } catch (Exception e) {
        	System.out.println ("got exception, message: " + e.getMessage());
            e.printStackTrace ();
        }
        return null;
    }

	private static int countPEMDelimitedBlocks (BufferedReader inputReader, String delimiter) throws Exception
	{
		String startDelimiter = "-----BEGIN " + delimiter + "-----";
		String stopDelimiter = "-----END " + delimiter + "-----";
		
		inputReader.mark (500000);
		int numBlocks = 0;
		String currentLine = null;
		do {
			while (((currentLine = inputReader.readLine()) != null) && !currentLine.equalsIgnoreCase (startDelimiter)) {
			}
			if (currentLine == null) {
				break;
			}
			while (((currentLine = inputReader.readLine()) != null) && !currentLine.equalsIgnoreCase (stopDelimiter)) {
			}
			if (currentLine.equalsIgnoreCase (stopDelimiter)) {
				numBlocks++;
			}
		} while (currentLine != null);
		inputReader.reset();
		return numBlocks;
    }    

	private static String extractPEMDelimitedBlock (BufferedReader inputReader, String delimiter, int desiredIndex) throws Exception
	{
        String startDelimiter = "-----BEGIN " + delimiter + "-----";
        String stopDelimiter = "-----END " + delimiter + "-----";
		
		inputReader.mark (500000);
        String currentLine = null;
		int blockIndex = 0;
		while (blockIndex < desiredIndex) {
			while (((currentLine = inputReader.readLine()) != null) && !currentLine.equalsIgnoreCase (startDelimiter) ) {
			}
			if (currentLine == null) {//no such block exists in file
				inputReader.reset();
				return null;
			}
			while (((currentLine = inputReader.readLine()) != null) && !currentLine.equalsIgnoreCase (stopDelimiter)) {
			}
			if (currentLine == null) {//no such block exists in file
				inputReader.reset();
				return null;
			}
			blockIndex++;
		}
			
        while (((currentLine = inputReader.readLine()) != null) && !currentLine.equalsIgnoreCase (startDelimiter) ) {
        }
        if (currentLine == null) {//no such block exists in file
			inputReader.reset();
            return null;
        }
        StringBuffer block = new StringBuffer();
		if (delimiter.equalsIgnoreCase ("CERTIFICATE")) {
			block.append (currentLine + "\n");
		}
        while (((currentLine = inputReader.readLine()) != null) && !currentLine.equalsIgnoreCase (stopDelimiter)) {
			if (currentLine.startsWith ("Proc-Type") || currentLine.startsWith ("DEK-Info")) {
				block.append (currentLine + "\n");
			}
			else
				block.append (currentLine);
        }
		if (currentLine.equalsIgnoreCase (stopDelimiter) && delimiter.equalsIgnoreCase ("CERTIFICATE")) {
				block.append ("\n" + currentLine + "\n");
		}
		inputReader.reset();
        return block.toString();
    }

    /**
	 * create the secret key needed for this object, fetching the password
	 */
	private static SecretKey getSecretKey (String algorithm, int keyLength, byte[] salt)
		throws IOException
	{
		byte[] key = new byte[keyLength];
		int offset = 0;
		int bytesNeeded = keyLength;

		if (_secretKey == null) {
			throw new IOException ("No password specified, but a password is required");
		}

		char[] password = _secretKey.toCharArray();

		MessageDigest md5;
		
		try {
		if (_jceProvider.startsWith ("Sun") || _jceProvider.startsWith ("SUN")) {
				md5 = MessageDigest.getInstance ("MD5");
			}
			else {
				md5 = MessageDigest.getInstance ("MD5", _jceProvider);
			}
		}
		catch (Exception e)
		{
			throw new IOException ("can't create digest: " + e.toString());
		}

		for (;;)
		{
			for (int i = 0; i != password.length; i++)
			{
				md5.update ((byte) password[i]);
			}
			md5.update (salt);

			byte[] digest = md5.digest();
			int len = (bytesNeeded > digest.length) ? digest.length : bytesNeeded;
			System.arraycopy (digest, 0, key, offset, len);
			offset += len;

			// check if we need any more
			bytesNeeded = key.length - offset;
			if (bytesNeeded == 0)
			{
				break;
			}

			// do another round
			md5.reset();
			md5.update (digest);
		}

		return new javax.crypto.spec.SecretKeySpec (key, algorithm);
	}

    /**
    * Removes line breaks from a String
    */
	private static byte[] readStream (String inputString) throws Exception
	{
		String line = null;
		String outputString = new String();
		StringReader inputStringReader= new StringReader (inputString);
		BufferedReader inputReader = new BufferedReader (inputStringReader);

		line = inputReader.readLine();
		while (line != null)
		{
			outputString += line;
			line = inputReader.readLine();
		}
		return outputString.getBytes();
	}

	private static PrivateKey readPrivateKey (String keyType, String keyString) throws Exception
	{
        if (_debug) {
            Provider[] providers = Security.getProviders();
            for (int i = 0; i < providers.length; i++) {
                System.out.println("readPrivateKey; providers = " + providers[i].getInfo());
            }
			System.out.println ("readPrivateKey, got keyType: " + keyType + " keyString: \n" + keyString);
        }

		boolean isEncrypted = false;
		String line = null;
		String dekInfo = null;
		StringBuffer keyData = new StringBuffer();
		String endline = "-----END";
		int endlen = endline.length();
		Base64 base64 = new org.bouncycastle.util.encoders.Base64();

		StringReader inputStringReader = new StringReader (keyString);
		BufferedReader inputReader = new BufferedReader (inputStringReader);

		//line = inputReader.readLine(); // Skip first line
		while ((line = inputReader.readLine()) != null) {
			if (line.startsWith ("Proc-Type: 4,ENCRYPTED")) {
				isEncrypted = true;
			}
			else if (line.startsWith ("DEK-Info:")) {
				dekInfo = line.substring (10);
			}
			else if (line.indexOf (endline) != -1) {
				break;
			}
			else {
				keyData.append (line);
			}
		}

		//
		// extract the key
		//
		byte[] keyBytes = null;

		if (isEncrypted) {
			StringTokenizer tokenizer = new StringTokenizer (dekInfo, ",");
			String encoding = tokenizer.nextToken();

			if (encoding.equals ("DES-EDE3-CBC")) {
				if (_debug) {
					System.out.println ("readPrivateKey, DEKInfo, string encoding is DES-EDE3-CBC");
				}
				String algorithm = "DESede";
				byte[] initializationVector = Hex.decode (tokenizer.nextToken());
				Key secretKey = getSecretKey (algorithm, 24, initializationVector);
				Cipher cipher = Cipher.getInstance ("DESede/CBC/PKCS5Padding", _jceProvider);
				cipher.init (Cipher.DECRYPT_MODE, secretKey, new IvParameterSpec (initializationVector));
				keyBytes = cipher.doFinal (base64.decode (readStream (keyData.toString())));
			}
			else if (encoding.equals("DES-CBC")) {
				String algorithm = "DES";
				byte[] initializationVector = Hex.decode(tokenizer.nextToken());
				Key secretKey = getSecretKey (algorithm, 8, initializationVector);
				Cipher cipher = Cipher.getInstance("DES/CBC/PKCS5Padding", _jceProvider);
				cipher.init(Cipher.DECRYPT_MODE, secretKey, new IvParameterSpec(initializationVector));
				String bString = new String (cipher.doFinal (base64.decode (readStream (keyData.toString()))));
				keyBytes = bString.getBytes();
			}
			else {   
				throw new IOException ("unknown encryption with private key");
			}
		}
		else { 
			keyBytes = base64.decode (readStream (keyData.toString()));
		}

		if (keyType.equals ("RSA-PKCS8")) {
			PKCS8EncodedKeySpec private_key_spec = new PKCS8EncodedKeySpec (keyBytes);
			KeyFactory kfac = KeyFactory.getInstance ("RSA", _jceProvider);
			PrivateKey key = kfac.generatePrivate (private_key_spec);
			return key;
		}

		KeySpec pubSpec, privSpec;
		ByteArrayInputStream bIn = new ByteArrayInputStream (keyBytes);
		DERInputStream  dIn = new DERInputStream (bIn);
		ASN1Sequence seq = (ASN1Sequence) dIn.readObject();
		dIn.close();

		if (keyType.equals ("RSA")) {
			DERInteger v = (DERInteger)seq.getObjectAt(0);
			DERInteger modulus = (DERInteger)seq.getObjectAt(1);
			DERInteger publicExponent = (DERInteger)seq.getObjectAt(2);
			DERInteger privateExponent = (DERInteger)seq.getObjectAt(3);
			DERInteger primeP = (DERInteger)seq.getObjectAt(4);
			DERInteger primeQ = (DERInteger)seq.getObjectAt(5);
			DERInteger primeExponentP = (DERInteger)seq.getObjectAt(6);
			DERInteger primeExponentQ = (DERInteger)seq.getObjectAt(7);
			DERInteger crtCoefficient = (DERInteger)seq.getObjectAt(8);

            privSpec = new RSAPrivateCrtKeySpec (modulus.getValue(),
                    publicExponent.getValue(),
                    privateExponent.getValue(),
                    primeP.getValue(),
                    primeQ.getValue(),
                    primeExponentP.getValue(),
                    primeExponentQ.getValue(),
                    crtCoefficient.getValue());
			
		}
        else {   // "DSA"
			DERInteger v = (DERInteger)seq.getObjectAt(0);
			DERInteger p = (DERInteger)seq.getObjectAt(1);
			DERInteger q = (DERInteger)seq.getObjectAt(2);
			DERInteger g = (DERInteger)seq.getObjectAt(3);
			DERInteger y = (DERInteger)seq.getObjectAt(4);
			DERInteger x = (DERInteger)seq.getObjectAt(5);

			privSpec = new DSAPrivateKeySpec (x.getValue(), p.getValue(), q.getValue(), g.getValue());
		}
		
		KeyFactory keyFactory = null;
		if (_jceProvider.startsWith ("Sun") || _jceProvider.startsWith ("SUN")) {
			keyFactory = KeyFactory.getInstance (keyType);
		}
		else {
			keyFactory = KeyFactory.getInstance (keyType, _jceProvider);
		}
		
		return keyFactory.generatePrivate (privSpec); 
	}


  private static String _secretKey = "foobar";
  private static String _jceProvider = "puretls";
  //private static String _jceProvider = "SunJCE";
  private static final boolean _debug = false;

}