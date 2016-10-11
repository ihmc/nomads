/*
 * CryptoUtils.java
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

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.util.*;

import java.security.InvalidKeyException;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchProviderException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Security;

import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.Random;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;

import javax.crypto.spec.DESKeySpec;

import java.io.ByteArrayOutputStream;

/**
 * Defines utility cryptographic functions
 */
public class CryptoUtils
{
    /**
     * Sets the provider for the JCE API
     * 
     * @param provider  the provider to use
     * 
     * @exception NoSuchProviderException   if the specified provider does not exist or if
     * the provider does not provide all the required functionality
     */
    public static void setProvider (String provider)
        throws NoSuchProviderException
    {
        if (provider.equalsIgnoreCase ("BC")) {
            Security.addProvider (new org.bouncycastle.jce.provider.BouncyCastleProvider());
        }
        // Make sure that the specified provider handles the necessary functionality
        try {
            SecretKeyFactory.getInstance ("DES", provider);
            Cipher.getInstance ("DES/ECB/NoPadding", provider);
            KeyPairGenerator.getInstance ("RSA", provider);
            Cipher.getInstance ("RSA/None/PKCS1Padding", provider);
            Cipher.getInstance ("RSA/None/OAEPPadding", provider);
        }
        catch (NoSuchProviderException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }

        // If we made it this far without throwing a NoSuchProviderException, the
        // provider is satisfactory
        _provider = provider;
    }

    /**
     * Generates a new DES secret key given a password. The same key is generated on each invocation
     * if the supplied password remains the same. Only the first 8 bytes of the password are used.
     * 
     * @param password  the password used to derive the key
     * 
     * @return the generated secret key based on the supplied password
     */
    public static SecretKey generateSecretKey (String password)
    {
        try {
            byte[] keyBytes = new byte[8];
            byte[] passwordBytes = password.getBytes();
            System.arraycopy (passwordBytes, 0, keyBytes, 0, passwordBytes.length < 8 ? passwordBytes.length : 8);
            DESKeySpec keySpec = new DESKeySpec (keyBytes);
            SecretKeyFactory keyFactory;
            if (_provider == null) {
                keyFactory = SecretKeyFactory.getInstance ("DES");
            }
            else {
                keyFactory = SecretKeyFactory.getInstance ("DES", _provider);
            }
            return keyFactory.generateSecret (keySpec);
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    /**
     * Encrypt the input data using DES and the specified secret key.
     * Null bytes are added at the end to pad the input if the input buffer
     * is not an even multiple of the block size.
     *
     * @param key       the secret key to be used for encryption
     * @param input     the input data that needs to be encrypted
     *
     * @return          the encrypted data
     */
    public static byte[] encryptUsingSecretKey (SecretKey key, byte[] input)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding");
            }
            else {
                //System.out.println ("->CriptoUtils:encryptUsingSecretKey: provider not null");
                cipher = Cipher.getInstance ("DES/ECB/NoPadding", _provider);
            }
            //System.out.println ("->CriptoUtils:encryptUsingSecretKey:before cipher.init");
            cipher.init (Cipher.ENCRYPT_MODE, key);      

            int blockSize = cipher.getBlockSize();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            int count = 0;
            byte[] buf;

            while (count + blockSize <= input.length) {
                buf = cipher.doFinal (input, count, blockSize);
                baos.write (buf, 0, buf.length);
                count += blockSize;
            }
            
            if (input.length - count > 0) {
                cipher.update (input, count, input.length-count);
                int paddingLen = cipher.getBlockSize() - (input.length % cipher.getBlockSize());
                byte[] padding = new byte [paddingLen];
                buf = cipher.doFinal (padding, 0, paddingLen);
                baos.write (buf, 0, buf.length);
            }
            
//            if ((input.length % cipher.getBlockSize()) != 0) {
//                int paddingLen = cipher.getBlockSize() - (input.length % cipher.getBlockSize());
//                byte[] padding = new byte [paddingLen];
//                //System.out.println ("padding output by " + paddingLen + " null bytes");
//                cipher.update (padding);
//            }
            
            return (baos.toByteArray());
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    /**
     * Encrypt the input data using DES and the specified secret key.
     * Note that the size of the input buffer must be a multiple of 8 bytes.
     *
     * @param key       the secret key to be used for encryption
     * @param input     the input data that needs to be encrypted
     *
     * @return          the encrypted data
     */
    public static byte[] encryptUsingSecretKeyNoPadding (SecretKey key, byte[] input)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding");
            }
            else {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding", _provider);
            }
            cipher.init (Cipher.ENCRYPT_MODE, key);
            
            int blockSize = cipher.getBlockSize();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            int count = 0;
            byte[] buf;

            while (count + blockSize < input.length) {
                buf = cipher.doFinal (input, count, blockSize);
                baos.write (buf, 0, buf.length);
                count += blockSize;
            }
            
            buf = cipher.doFinal (input, count, input.length-count);
            baos.write (buf, 0, buf.length);

            return (baos.toByteArray());
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }
    
    public static byte[] encryptStringUsingSecretKey (SecretKey key, String input)
        throws InvalidKeyException
    {
        int inputLength = input.getBytes().length;
        int newInputLength = ((inputLength + 7) / 8) * 8;   // Round up to the nearest multiple of 8
        byte[] newInput = new byte[newInputLength];
        System.arraycopy (input.getBytes(), 0, newInput, 0, inputLength);
        for (int i = inputLength; i < newInputLength; i++) {
            newInput[i] = 0;
        }
        return encryptUsingSecretKeyNoPadding (key, newInput);
    }

    public static SecureOutputStream encryptUsingSecretKey (SecretKey key, OutputStream os)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding");
            }
            else {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding", _provider);
            }
            cipher.init (Cipher.ENCRYPT_MODE, key);
            return new SecureOutputStream (os, cipher);
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    
   /**
     * Decrypt the input data using DES and the specified secret key.
     * Note that the returned byte array could be padded with null bytes.
     *
     * @param key       the secret key to be used for decryption
     * @param input     the input data that needs to be decrypted
     *
     * @return          the decrypted data
     */
    public static byte[] decryptUsingSecretKey (SecretKey key, byte[] input)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding");
            }
            else {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding", _provider);
            }
            cipher.init (Cipher.DECRYPT_MODE, key);
            
            int blockSize = cipher.getBlockSize();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            int count = 0;
            byte[] buf;

            while (count + blockSize < input.length) {
                buf = cipher.doFinal (input, count, blockSize);
                baos.write (buf, 0, buf.length);
                count += blockSize;
            }
            
            buf = cipher.doFinal (input, count, input.length-count);
            baos.write (buf, 0, buf.length);

            return (baos.toByteArray());
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }
    
    public static String decryptStringUsingSecretKey (SecretKey key, byte[] input)
        throws InvalidKeyException
    {
        byte[] output = decryptUsingSecretKey (key, input);
        String result = new String (output);
        return result.trim();
    }

    public static SecureInputStream decryptUsingSecretKey (SecretKey key, InputStream is)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding");
            }
            else {
                cipher = Cipher.getInstance ("DES/ECB/NoPadding", _provider);
            }
            cipher.init (Cipher.DECRYPT_MODE, key);
            return new SecureInputStream (is, cipher);
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static KeyPair generateNewKeyPair()
    {
        try {
            KeyPairGenerator kpg;
            if (_provider == null) {
                kpg = KeyPairGenerator.getInstance ("RSA");
            }
            else {
                kpg = KeyPairGenerator.getInstance ("RSA", _provider);
            }
            return kpg.generateKeyPair();
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static PublicKey createPublicKeyFromDEREncodedX509Data (byte[] input)
        throws InvalidKeySpecException
    {
        try {
            X509EncodedKeySpec keySpec = new X509EncodedKeySpec (input);
            KeyFactory keyFac = null;
            if (_provider == null) {
                keyFac = KeyFactory.getInstance ("RSA");
            }
            else {
                keyFac = KeyFactory.getInstance ("RSA", _provider);
            }
            return keyFac.generatePublic (keySpec);
        }
        catch (InvalidKeySpecException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static PrivateKey createPrivateKeyFromDEREncodedPKCS8Data (byte[] input)
        throws InvalidKeySpecException
    {
        try {
            PKCS8EncodedKeySpec keySpec = new PKCS8EncodedKeySpec (input);
            KeyFactory keyFac = null;
            if (_provider == null) {
                keyFac = KeyFactory.getInstance ("RSA");
            }
            else {
                keyFac = KeyFactory.getInstance ("RSA", _provider);
            }
            return keyFac.generatePrivate (keySpec);
        }
        catch (InvalidKeySpecException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static byte[] encryptUsingPublicKey (PublicKey key, byte[] input)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("RSA/None/OAEPPadding");
            }
            else {
                cipher = Cipher.getInstance ("RSA/None/OAEPPadding", _provider);
            }
            cipher.init (Cipher.ENCRYPT_MODE, key);
            
            int blockSize = cipher.getBlockSize();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            int count = 0;
            byte[] buf;

            while (count + blockSize < input.length) {
                buf = cipher.doFinal (input, count, blockSize);
                baos.write (buf, 0, buf.length);
                count += blockSize;
            }
            
            buf = cipher.doFinal (input, count, input.length-count);
            baos.write (buf, 0, buf.length);

            return (baos.toByteArray());
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static SecureOutputStream encryptUsingPublicKey (PublicKey key, OutputStream os)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("RSA/None/OAEPPadding");
            }
            else {
                cipher = Cipher.getInstance ("RSA/None/OAEPPadding", _provider);
            }
            cipher.init (Cipher.ENCRYPT_MODE, key);
            return new SecureOutputStream (os, cipher);
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static byte[] encryptUsingPrivateKey (PrivateKey key, byte[] input)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("RSA/None/PKCS1Padding");
            }
            else {
                cipher = Cipher.getInstance ("RSA/None/PKCS1Padding", _provider);
            }
            cipher.init (Cipher.ENCRYPT_MODE, key);
            
            int blockSize = cipher.getBlockSize();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            int count = 0;
            byte[] buf;

            while (count + blockSize < input.length) {
                buf = cipher.doFinal (input, count, blockSize);
                baos.write (buf, 0, buf.length);
                count += blockSize;
            }
            
            buf = cipher.doFinal (input, count, input.length-count);
            baos.write (buf, 0, buf.length);

            return (baos.toByteArray());
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }
    
    public static SecureOutputStream encryptUsingPrivateKey (PrivateKey key, OutputStream os)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("RSA/None/PKCS1Paddding");
            }
            else {
                cipher = Cipher.getInstance ("RSA/None/PKCS1Padding", _provider);
            }
            cipher.init (Cipher.ENCRYPT_MODE, key);
            return new SecureOutputStream (os, cipher);
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static byte[] decryptUsingPublicKey (PublicKey key, byte[] input)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("RSA/None/PKCS1Padding");
            }
            else {
                cipher = Cipher.getInstance ("RSA/None/PKCS1Padding", _provider);
            }
            cipher.init (Cipher.DECRYPT_MODE, key);
            
            int blockSize = cipher.getBlockSize();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            int count = 0;
            byte[] buf;

            while (count + blockSize < input.length) {
                buf = cipher.doFinal (input, count, blockSize);
                baos.write (buf, 0, buf.length);
                count += blockSize;
            }
            
            buf = cipher.doFinal (input, count, input.length-count);
            baos.write (buf, 0, buf.length);

            return (baos.toByteArray());
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static SecureInputStream decryptUsingPublicKey (PublicKey key, InputStream is)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("RSA/None/PKCS1Padding");
            }
            else {
                cipher = Cipher.getInstance ("RSA/None/PKCS1Padding", _provider);
            }
            cipher.init (Cipher.DECRYPT_MODE, key);
            return new SecureInputStream (is, cipher);
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static byte[] decryptUsingPrivateKey (PrivateKey key, byte[] input)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("RSA/None/OAEPPadding");
            }
            else {
                cipher = Cipher.getInstance ("RSA/None/OAEPPadding", _provider);
            }
            cipher.init (Cipher.DECRYPT_MODE, key);
            
            int blockSize = cipher.getBlockSize();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            int count = 0;
            byte[] buf;

            while (count + blockSize < input.length) {
                buf = cipher.doFinal (input, count, blockSize);
                baos.write (buf, 0, buf.length);
                count += blockSize;
            }
            
            buf = cipher.doFinal (input, count, input.length-count);
            baos.write (buf, 0, buf.length);

            return (baos.toByteArray());

        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static SecureInputStream decryptUsingPrivateKey (PrivateKey key, InputStream is)
        throws InvalidKeyException
    {
        try {
            Cipher cipher;
            if (_provider == null) {
                cipher = Cipher.getInstance ("RSA/None/OAEPPadding");
            }
            else {
                cipher = Cipher.getInstance ("RSA/None/OAEPPadding", _provider);
            }
            cipher.init (Cipher.DECRYPT_MODE, key);
            return new SecureInputStream (is, cipher);
        }
        catch (InvalidKeyException e) {
            throw e;
        }
        catch (Exception e) {
            // Should not happen!
            throw new RuntimeException ("Unexpected exception - " + e);
        }
    }

    public static SecretKey generateAndSendSecretKey (PublicKey pk, OutputStream out) throws IOException, InvalidKeyException {
        Random r = new Random();
        SecretKey sk = CryptoUtils.generateSecretKey (""+r.nextFloat());
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        ObjectOutputStream oos = new ObjectOutputStream (baos);
        oos.writeObject (sk);
        oos.close();
        byte[] bout = CryptoUtils.encryptUsingPublicKey (pk, baos.toByteArray());
        out.write (bout);
        baos.close();
        return sk;
    }
    
    public static SecretKey readSecretKey (PrivateKey pk, InputStream in) throws IOException, ClassNotFoundException, InvalidKeyException {
        byte[] buf = new byte[256];
        in.read (buf);
        byte[] bufdec = decryptUsingPrivateKey (pk, buf);
        ObjectInputStream oin = new ObjectInputStream (new ByteArrayInputStream (bufdec));
        SecretKey sk = (SecretKey) oin.readObject();
        oin.close();
        return sk;
    }
    
    private static String _provider = null;
}
