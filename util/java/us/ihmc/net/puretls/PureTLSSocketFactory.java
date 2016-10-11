/*
 * PureTLSSocketFactory.java
 *
 *This file is part of the IHMC Util Library
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

import java.io.File;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.PublicKey;
import java.util.Hashtable;
import java.util.Vector;

import us.ihmc.net.SocketFactory;
import us.ihmc.util.ByteArray;
import us.ihmc.util.CertInfo;
import us.ihmc.util.CertsFileManager;
import us.ihmc.util.ConfigLoader;
import COM.claymoresystems.cert.X509Cert;
import COM.claymoresystems.ptls.SSLContext;
import COM.claymoresystems.ptls.SSLDebug;
import COM.claymoresystems.ptls.SSLSocket;
import COM.claymoresystems.sslg.SSLPolicyInt;

/**
 * SSL socket factory--wraps PureTLS
 *
 * @author Tom Cowin <tcowin@ihmc.us>
 * @author Matteo Rebeschini <mrebeschini@ihmc.us>
 *  - moved part of the init() to the static initializer
 */
public class PureTLSSocketFactory implements SocketFactory
{
    public PureTLSSocketFactory()
    {
    }

    public Socket createSocket (String hostName, int port) throws IOException, UnknownHostException
    {
        SSLContext sslContext = init();
        SSLSocket sslSocket = new SSLSocket (sslContext, hostName, port);
        postConnectionCheck (sslSocket);
        return sslSocket;
    }

    public Socket createSocket (Socket socket, InetAddress hostAddr, int port, int how) throws IOException, UnknownHostException
    {
        //return createSocket (socket, hostAddr.getHostName(), port, how); Avoid the use of DNS when not necessary
        return createSocket (socket, hostAddr.getHostAddress(), port, how);
    }

    public Socket createSocket (Socket socket, String hostName, int port, int how) throws IOException, UnknownHostException
    {
        SSLContext sslContext = init();
        SSLSocket sslSocket = new SSLSocket (sslContext, socket, hostName, port, how);
        sslSocket.connect (socket.getRemoteSocketAddress());

        postConnectionCheck (sslSocket);

        return sslSocket;
    }

    public Socket createSocket (InetAddress hostAddr, int port) throws IOException, UnknownHostException
    {
        SSLContext sslContext = init();
        SSLSocket sslSocket = new SSLSocket (sslContext, hostAddr, port);
        postConnectionCheck (sslSocket);
        return sslSocket;
    }

    public Socket createSocket (String hostName, int port, InetAddress clientHostAddr, int clientPort) throws IOException, UnknownHostException
    {
        SSLContext sslContext = init();
        SSLSocket sslSocket = new SSLSocket (sslContext, hostName, port, clientHostAddr, clientPort);
        postConnectionCheck (sslSocket);
        return sslSocket;
    }

    public Socket createSocket (InetAddress hostAddr, int port, InetAddress clientHostAddr, int clientPort) throws IOException, UnknownHostException
    {
        SSLContext sslContext = init();
        SSLSocket sslSocket = new SSLSocket (sslContext, hostAddr, port, clientHostAddr, clientPort);
        postConnectionCheck (sslSocket);
        return sslSocket;
    }

    public void handshake (Socket sock) throws IOException
    {
        long now = System.currentTimeMillis();
        ((SSLSocket)sock).handshake();
        System.out.println ("PureTLSSocketFactory.handshake(): " + (System.currentTimeMillis() - now));
    }

    public static void setMode (int newMode)
    {
        _mode = newMode;
    }

    public PublicKey getPeersCachedPublicKey (SSLSocket sslSocket) {
        return PureTLSUtils.getPeersCachedPublicKey (sslSocket, _X509Certificates);
    }

    private void postConnectionCheck (SSLSocket sslSocket)
    {
        try {
            //the certificate chain as a Vector of X509Certs, null if unavailable
            //The root is at 0 and the user cert is at n-1
            Vector certificateChain = sslSocket.getCertificateChain();
            InetAddress remoteIPAddr = sslSocket.getInetAddress();
            byte[] ba = remoteIPAddr.getAddress();
            String IPAddr = new String (ba[0] + "." + ba[1] + "." + ba[2] + "." + ba[3]);
            if (_debug >= 1) {
                System.out.println ("Remote IP address is : " + IPAddr);
            }

            if (certificateChain != null) {
                if (_debug >= 1) {
                    System.out.println ("Cert chain");
                }
                int certChainLength = certificateChain.size();
                for (int i = 0; i < certChainLength; i++) {
                    X509Cert cert = (X509Cert) certificateChain.elementAt (i);
                    if (_debug >= 1) {
                        System.out.println ("Cert at position " + i + ":");
                    }
                    //added to check if valid cert (i.e., mapping from IP Addr to Cert)
                    String commonName = PureTLSUtils.getCommonName (cert);
                    if (_debug >= 1) {
                        System.out.println ("Common Name is : " + commonName);
                    }
                    byte[] serverCert = cert.getDER();
                    if (i == certChainLength - 1) {
                        if (_mode == CERTUIDCROSSVALIDATING) {
                            if (_X509Certificates.containsKey (commonName)) {
                                CertInfo ci = (CertInfo)_X509Certificates.get (commonName);
                                if (!ByteArray.areEqual (ci.getDERCert(), serverCert)){
                                    System.out.println ("Certs for commonName " + commonName + " do not match! exiting....");
                                    System.exit(1);
                                }
                                if (_debug >= 1) {
                                    System.out.println("Certs do match! continuing....");
                                }
                            }
                            else {
                                System.out.println ("commonName " + commonName + " has no stored cert...exiting.");
                                System.exit(1);
                            }
                        }
                        else if (_mode == CERTCACHING) {
                            if (_X509Certificates.containsKey (commonName)) {
                                CertInfo ci = (CertInfo)_X509Certificates.get (commonName);
                                if (!ByteArray.areEqual (ci.getDERCert(), serverCert)){
                                    System.out.println ("Server Cert presented for IP " + IPAddr + " does not match stored cert. Possible Security Violation.");
                                }
                                else {
                                    System.out.println ("Already have this Certificate on file for IP " + IPAddr);
                                }
                            }
                            else {
                                _X509Certificates.put (commonName, new CertInfo (commonName, IPAddr, serverCert));
                                CertsFileManager.writeFile (_cachedCertsPath, _X509Certificates);
                            }
                        }
                    }
                    if (_debug >= 2) {
                        ByteArray.printByteArrayAsHexColumns (serverCert);
                    }
                    if (_debug >= 1) {
                        System.out.println ("Subject " + cert.getSubjectName().getNameString());
                    }
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private SSLContext init() throws IOException
    {
        String keyStorePath =  _securityDir + File.separator + _keyStoreFile;
        String rootCertPath = _securityDir + File.separator + _rootFile;
        //String randomPath = _securityDir + File.separator + _randomFile;

        SSLContext sslContext = null;

        sslContext = new SSLContext();

        SSLPolicyInt policy = new SSLPolicyInt();

        if (_cipherSuites != null) {
            policy.setCipherSuites (_cipherSuites);
        }

        policy.acceptUnverifiableCertificates (_acceptUnverified);
        policy.negotiateTLS (_protocol.equalsIgnoreCase ("TLS"));
        policy.requireClientAuth (true);
        policy.waitOnClose (false);
        sslContext.setPolicy (policy);

        if (_debug >= 3) {
            SSLDebug.setDebug (SSLDebug.DEBUG_INIT|SSLDebug.DEBUG_CERT);
            //SSL_debug.set_debug (SSL_debug._debug_ALL);
        }
        sslContext.loadRootCertificates (rootCertPath);
        sslContext.loadEAYKeyFile (keyStorePath, _keyPass);
        //sslContext.useRandomnessFile (randomPath, _keyPass);

        _cachedCertsPath = _securityDir + File.separator + _cachedCertsFile;
        _X509Certificates = CertsFileManager.readFile (_cachedCertsPath);

        return sslContext;
    }

    //Class Variables
    public static final int NORMAL = 0;
    public static final int CERTCACHING = 1;
    public static final int CERTUIDCROSSVALIDATING = 2;
    public static final int CLIENT = 1;
    public static final int SERVER = 2;

    private static int _mode = NORMAL;
    private static Hashtable _X509Certificates;
    private static String _keyStoreFile = "client.pem";
    private static String _rootFile = "rootcert.pem";
    private static String _securityDir = null;
    private static String _baseDir = null;
    //private static String _randomFile = "random-" + new Long (System.currentTimeMillis()).toString() + ".pem";
    private static String _cachedCertsFile = "cached_certs.txt";
    private static String _cachedCertsPath = null;
    private static String _keyPass = "foobar";
    private static boolean _acceptUnverified = false;
    private static short[] _cipherSuites = null;
    private static String _protocol = "TLS";
    private static final int _debug = 0;

    //Static Initializer
    static
    {
           ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();

           try {
            String tmpString = null;

            if ((tmpString = cloader.getProperty ("keypass")) != null) {
                _keyPass = tmpString;
            }

            if ((tmpString = cloader.getProperty ("secureprotocol")) != null) {
                _protocol = tmpString;
            }

            if ((tmpString = cloader.getProperty ("keystore")) != null) {
                _keyStoreFile = tmpString;
            }

            if ((tmpString = cloader.getProperty ("rootfile")) != null) {
                _rootFile = tmpString;
            }

            /*
            if ((tmpString = cloader.getProperty ("randomfile")) != null) {
                _randomFile = tmpString;
            }
            */

            if ((_baseDir = cloader.getHomeDirectory()) == null) {

                    throw new IOException ("Unable to get value of HomeDir from systemProperties");
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }

        _securityDir = _baseDir + File.separator + "lib" + File.separator + "security";
    }
}





