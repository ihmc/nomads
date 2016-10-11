/**
 * ConnectionManager.java
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

package us.ihmc.comm;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.PublicKey;
import java.security.Security;
import java.security.interfaces.RSAPublicKey;

import us.ihmc.net.NetUtils;
import us.ihmc.net.SocketFactory;
import us.ihmc.net.puretls.PureTLSServerSocketFactory;
import us.ihmc.net.puretls.PureTLSSocketFactory;
import us.ihmc.net.puretls.PureTLSUtils;
import us.ihmc.util.crypto.MASTPKITools;
import us.ihmc.util.ConfigLoader;
import COM.claymoresystems.ptls.SSLServerSocket;
import COM.claymoresystems.ptls.SSLSocket;
import java.util.HashMap;

public class ConnectionManager
{

    /**
     *
     */
    public ConnectionManager ()
    {
    }

    public CommHelper negotiateProtocol (String protocol, String nomadsHost, String nomadsEnv)
        throws CommException, ProtocolException
    {
        _commHelper = connectToExecEnv (nomadsHost, nomadsEnv);
        _commHelper.receiveMatch ("WELCOME");
        _commHelper.sendLine ("SPEAK " + protocol);

        String srsp = _commHelper.receiveLine();
        if (!srsp.trim().equalsIgnoreCase ("USE " + protocol)) {
            throw new CommException ("Unable to establish protocol " + protocol + " with " + nomadsHost + " at " + nomadsEnv);
        }

        return _commHelper;
    }

    public CommHelper connectToExecEnv (String nomadsHost, String nomadsEnv, byte encryptionLevel)
        throws CommException
    {
        return connectToExecEnv (nomadsHost, new Integer(nomadsEnv).intValue(), encryptionLevel);
    }

    public CommHelper connectToExecEnv (String nomadsHost, int nomadsEnv, byte encryptionLevel)
        throws CommException
    {
        _encryptionLevel = encryptionLevel;
        if (_encryptionLevel == NO_ENCRYPTION && _verificationLevel == VERIFICATION_UNINITIALIZED) {
            _verificationLevel = NO_VERIFICATION;
        }
        return connectToExecEnv (nomadsHost, nomadsEnv);
    }

    public CommHelper connectToExecEnv (String nomadsHost, String nomadsEnv)
        throws CommException
    {
        if (nomadsEnv == null) {
            printDebugLog ("ConnectionManager.connectToExecEnv, nomadsEnv is null, assuming standard port");
            nomadsEnv = "3280";
        }
        return connectToExecEnv (nomadsHost, new Integer (nomadsEnv).intValue());
    }

    public CommHelper connectToExecEnv (String nomadsHost, int port)
    throws CommException
    {
        boolean localPlain = false;

        if (nomadsHost == null) {
            printDebugLog ("ConnectionManager.connectToExecEnv, nomadsHost is null, assuming localhost");
            nomadsHost = "localhost";//assume localhost for now?
        }
        _commHelper = new CommHelper();
        Socket sock;
        SocketFactory socketFactory = null;
        if (_encryptionLevel == ENCRYPTION_UNINITIALIZED) {
            _encryptionLevel = checkEncryptionLevel();
        }
        if (_verificationLevel == VERIFICATION_UNINITIALIZED) {
            _verificationLevel = checkVerificationLevel();
        }
        try {
            if (_encryptionLevel > SSL_LOCALPLAIN) {
                loadProviders();
                socketFactory = new PureTLSSocketFactory();
                sock = (SSLSocket) socketFactory.createSocket (nomadsHost, port);
            }
            else {
                sock = new Socket (nomadsHost, port);
                sock.setTcpNoDelay (true);
                _commHelper.init (sock);
                if (_encryptionLevel == SSL_LOCALPLAIN && !isLocal (sock)) {
                    _commHelper.sendLine ("STARTTLS");

                    try {
                        //   501 Syntax error (no parameters allowed)
                        //   454 TLS not available due to temporary reason
                        //   220 Ready to start TLS
                        _commHelper.receiveMatch ("220 Ready to start TLS");
                    }
                    catch (CommException e1) {
                        // TODO Auto-generated catch block
                        e1.printStackTrace();
                    }
                    catch (ProtocolException e1) {
                        // TODO Auto-generated catch block
                        e1.printStackTrace();
                    }
                    loadProviders();
                    socketFactory = new PureTLSSocketFactory();
                    Socket sslSocket = (SSLSocket) ((PureTLSSocketFactory)socketFactory).createSocket (sock, nomadsHost, port, PureTLSSocketFactory.CLIENT);
                    _commHelper.init (sslSocket);
                }
                else {
                    printDebugLog (Thread.currentThread().toString() + " :[PLAIN]socket: " + sock.toString() );
                    localPlain = true;
                }
            }

            if (!localPlain && _verificationLevel >= CPKV_VERIFICATION) {
                //Cached PKI verification
                if (!isLocal (_commHelper.getSocket())) {
                    //determine if server side wants to initiate certificate verification
                    if (_commHelper.receiveLine().equals ("VERIFY")) {
                        printDebugLog ("Client SSL Connection, Certificate Verification requested");
                        if (!MASTPKITools.clientPKIVerification (_commHelper,
                            ((PureTLSSocketFactory)socketFactory).getPeersCachedPublicKey ((SSLSocket) _commHelper.getSocket()))) {
                            throw new CommException ("problem during PKI verification");
                        }
                    }
                    else {
                        printDebugLog ("Client SSL Connection, Certificate Verification skipped ");
                    }
                }
            }
        }
        catch (UnknownHostException e) {
            throw new CommException ("unknown host " + nomadsHost + "; nested exception is " + e);
        }
        catch (IOException e) {
            throw new CommException ("unable to establish connection to " + nomadsHost + ":" + port + "; nested exception is " + e);
        }

        // Now, let's check if there's a so_timeout property set for this socket.
        // this is done through the configLoader, using property name:
        // nomads.connection.timeout = xxxx     (time in milliseconds). mcarvalho, Aug/05
        int sockTimeout = getSocketTimeout();
        if (sockTimeout > 0) {
            try {
                _commHelper.setSocketTimeout(sockTimeout);
            }
            catch (Exception e) {
                //failed to set the socket timeout - ignore exception....
                e.printStackTrace();
            }
        }
        return _commHelper;
    }


    private int getSocketTimeout ()
    {
        // this is done through the config-loader, using property name:
        // nomads.connection.timeout = xxxx     (time in milliseconds). mcarvalho, Aug/05
        try {
            ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
            int socketTimeout = cloader.getPropertyAsInt("nomads.connection.timeout");
            return socketTimeout;
        }
        catch (Exception e) {
            printDebugLog ("FAILED TO READ socket_TimeoutProperty: " + e.getMessage());
        }
        return -1;
    }



    //SERVER SIDE

    /**
     * Establishes the server socket that will be bound to the specified port and ipaddr at the
     * specified encryptionLevel. Level of verification will be determined from environment.
     *
     * @param inetAddr
     * @param portNumber
     * @param encryptionLevel
     * @return
     * @throws Exception
     * @see checkEncryptionLevel
     */

    public ServerSocket getServerSocket (InetAddress inetAddr, int portNumber, byte encryptionLevel)
        throws Exception
    {
        _encryptionLevel = encryptionLevel;
        if (_verificationLevel == VERIFICATION_UNINITIALIZED) {
            if (_encryptionLevel == NO_ENCRYPTION) {
                _verificationLevel = NO_VERIFICATION;
            }
            else {
                _verificationLevel = checkVerificationLevel();
            }
        }
        return getServerSocket (inetAddr, portNumber);
    }

    /**
     * Establishes the server socket that will be bound to the specified port and ipaddr.
     * Level of encryption and verification will be determined from environment.
     *
     * @param inetAddr
     * @param portNumber
     * @return
     * @throws Exception
     * @see checkEncryptionLevel
     * @see checkVerificationLevel
     * @see acceptSocket
     */
    public ServerSocket getServerSocket (InetAddress inetAddr, int portNumber)
        throws Exception
    {
        if (_encryptionLevel == ENCRYPTION_UNINITIALIZED) {
            _encryptionLevel = checkEncryptionLevel();
        }
        if (_verificationLevel == VERIFICATION_UNINITIALIZED) {
            _verificationLevel = checkVerificationLevel();
        }
        if (_encryptionLevel > SSL_LOCALPLAIN) {
            loadProviders();
            _serverSocketFactory = new PureTLSServerSocketFactory();
            _serverSocket = (SSLServerSocket) _serverSocketFactory
                .createServerSocket (portNumber, 0, inetAddr);
        } else {
            _serverSocket = new ServerSocket (portNumber, 0, inetAddr);
        }

        return _serverSocket;
    }

    /**
     * Establishes the server socket that will be bound to the specified port, utilizing the
     * specified level of encryption.
     * Level of verification will be determined from environment.
     *
     * @param portNumber
     * @param encryptionLevel
     * @return
     * @throws Exception
     * @see checkEncryptionLevel
     * @see checkVerificationLevel
     * @see acceptSocket
     */
    public ServerSocket getServerSocket (int portNumber, byte encryptionLevel)
        throws Exception
    {
        _encryptionLevel = encryptionLevel;
        if (_verificationLevel == VERIFICATION_UNINITIALIZED) {
            if (_encryptionLevel == NO_ENCRYPTION) {
                _verificationLevel = NO_VERIFICATION;
            }
            else {
                _verificationLevel = checkVerificationLevel();
            }
        }
        return getServerSocket (portNumber);
    }

    /**
     * Establishes the server socket that will be bound to the specified port.
     * Level of encryption and verification will be determined from environment.
     *
     * @param portNumber
     * @return
     * @throws Exception
     * @see checkEncryptionLevel
     * @see checkVerificationLevel
     * @see acceptSocket
     */
    public ServerSocket getServerSocket (int portNumber)
        throws CommException, IOException
    {
        if (_encryptionLevel == ENCRYPTION_UNINITIALIZED) {
            _encryptionLevel = checkEncryptionLevel();
        }
        if (_verificationLevel == VERIFICATION_UNINITIALIZED) {
            _verificationLevel = checkVerificationLevel();
        }
        if (_encryptionLevel > SSL_LOCALPLAIN) {
            loadProviders();
            _serverSocketFactory = new PureTLSServerSocketFactory();
            _serverSocket = (SSLServerSocket) _serverSocketFactory
                    .createServerSocket (portNumber);
        } else {
            _serverSocket = new ServerSocket (portNumber);
        }
        printDebugLog ("Established server socket with encryption: " + _encryptionLevel + " on " + Thread.currentThread().toString());

        return _serverSocket;
    }

    /**
     * Calls the acceptSocket method utilizing the class member variable _serverSocket that
     * should have been established by a previous getServerSocket() method call.
     *
     * @return
     * @throws Exception
     * @throws CommException
     */
    public Socket acceptSocket ()
        throws Exception, CommException
    {
        return acceptSocket (_serverSocket);
    }

    /**
     * Listens on the specified server socket for incoming connection requests. Depending on
     * desired level of encryption and verification, will perform required setup on socket
     * before returning it to caller.
     *
     * @param serverSocket
     * @return
     * @throws Exception
     * @throws CommException
     */
    public Socket acceptSocket (ServerSocket serverSocket)
        throws Exception, CommException
    {
        boolean localPlain = false;
        PublicKey peersPublicKey = null;

        if (serverSocket == null) {
            throw new CommException ("Attempting to listen on null Server Socket");
        }
        if (_encryptionLevel == ENCRYPTION_UNINITIALIZED) {
            _encryptionLevel = checkEncryptionLevel();
        }
        if (_verificationLevel == VERIFICATION_UNINITIALIZED) {
            _verificationLevel = checkVerificationLevel();
        }
        Socket sock;
        if (_encryptionLevel > SSL_LOCALPLAIN) {
            loadProviders();
            sock = (COM.claymoresystems.ptls.SSLSocket) _serverSocketFactory.acceptSocket (serverSocket);
            printDebugLog ("Incoming socket accepted, using SSL always " + Thread.currentThread().toString());
            if (_verificationLevel > CPKV_VERIFICATION) {
                peersPublicKey = _serverSocketFactory.getPeersCachedPublicKey ((SSLSocket)sock);
            }

        }
        else {//using plain sockets at initial connection
            sock = serverSocket.accept();

            int sockTimeout = getSocketTimeout();
            if (sockTimeout > 0) {
                sock.setSoTimeout(sockTimeout);
            }

            printDebugLog ("\nIncoming socket: " + sock.toString() + " accepted. [EncryptionLevel: " + _encryptionLevel + "] " + Thread.currentThread().toString());

            if (_encryptionLevel == SSL_LOCALPLAIN && !isLocal (sock)) {
                printDebugLog ("Remote Incoming socket accepted, using SSL " + Thread.currentThread().toString());
                CommHelper commHelper = new CommHelper();
                commHelper.init (sock);
                commHelper.receiveMatch ("STARTTLS");
                commHelper.sendLine("220 Ready to start TLS");
                loadProviders();
                PureTLSSocketFactory socketFactory = new PureTLSSocketFactory();
                Socket newSocket = (SSLSocket) socketFactory.createSocket (sock,
                                                                           sock.getInetAddress(),
                                                                           sock.getPort(),
                                                                           PureTLSSocketFactory.SERVER);
                if (_verificationLevel >= CPKV_VERIFICATION) {
                    peersPublicKey = socketFactory.getPeersCachedPublicKey ((SSLSocket)newSocket);
                }
                sock = newSocket;
            }
            else {
                printDebugLog ("\nLocal Incoming socket " + sock.toString() + " accepted, using plain sockets " + Thread.currentThread().toString());
                localPlain = true;
            }
        }
        if (!localPlain && _verificationLevel >= CPKV_VERIFICATION) {
            if (!isLocal (sock)) {
                if (!(peersPublicKey instanceof RSAPublicKey)) {
                    throw new Exception ("NomadsHelper peersPublicKey is not instance of RSAPublicKey");
                }
                else {
                    String peer = PureTLSUtils.getPeersCommonName ((SSLSocket) sock) + "." + sock.getInetAddress();
                    boolean peerEntryExists = false;
                    Long timestamp = new Long (0L);
                    if (_verifiedPeers.containsKey (peer)) {
                        printDebugLog ("Server SSL Connection, peer: " + peer + " entry previously cached");
                        peerEntryExists = true;
                        timestamp = _verifiedPeers.get (peer);
                    }
                    if (!peerEntryExists || (timestamp.longValue() + (30 * 60 * 1000)) < System.currentTimeMillis()) {
                        //docheck
                        printDebugLog ("Server SSL Connection, verifying peer's: " + peer + " cert");
                        CommHelper commHelper = new CommHelper (sock);
                        if (!MASTPKITools.serverPKIVerification (commHelper, peersPublicKey)) {
                            commHelper.closeConnection();
                            throw new CommException ("problem during PKI verification");
                        }
                        else {
                            printDebugLog ("Server SSL Connection, caching peer's: " + peer + " timestamp");
                            _verifiedPeers.put (peer, System.currentTimeMillis());
                        }
                    }
                    else {
                        sock.getOutputStream().write("NO VERIFY\r\n".getBytes());
                    }
                }
            }
        }

        int sockTimeout = getSocketTimeout();
        if (sockTimeout > 0) {
            sock.setSoTimeout(sockTimeout);
        }
        return sock;
    }

    /**
     * Cache the peers X509 Certificate that has been provided to us as part of the initial SSL
     * exchange. This is applicable in SSL connections, primarily when preparing for MAST operations.
     * This enables a similar Certificate persistance and subsequent validation as that acheived
     * by ssh.
     *
     * @param socket
     * @return
     */
    public int cachePeersCert (Socket socket)
    {
        if (_serverSocketFactory != null) {
            return _serverSocketFactory.cachePeersCert ((SSLSocket)socket);
        }
        return -1;
    }

    /**
     * This is meant to be called by applications that are using sockets that aren't
     * encapsulated by an instance of this class. This is really effective when you have
     * an SSLSocket and you need to make sure that all of the data has been cleared off
     * of the socket prior to the close. Specify with an in whether this side of the
     * connection was either a SENDER_MODE or RECEIVER_MODE.
     *
     * @param socket
     * @param mode mode of the connection - was this side last a SENDER_MODE or
     *   RECEIVER_MODE
     */

    public static void disconnect (Socket socket, int mode)
    {
            byte encryptionLevel = checkEncryptionLevel();
            if ((encryptionLevel == SSL_LOCALPLAIN && !isLocal (socket)) ||
                    encryptionLevel > SSL_LOCALPLAIN) {
                try {
                    if (!(socket instanceof SSLSocket)) {
                        printDebugLog ("socket: " + socket.toString() + " not instanceof SSLSocket: " + Thread.currentThread().toString());
                    }
                    printDebugLog ("ConnectionManager attempting to disconnect in " + (mode == SENDER_MODE ? "SENDER_MODE\n" : "RECEIVER_MODE\n"));
                    SSLSocket sslSocket = (SSLSocket) socket;
                    if (mode == SENDER_MODE) {
                        sslSocket.waitForClose (true);
                    }
                    sslSocket.close();
                    printDebugLog ("SSLSocket is closed.");
                }
                catch (Exception e) {
                    printDebugLog ("unable to disconnect: " + Thread.currentThread().toString());
                    e.printStackTrace();
                    printDebugLog ("<SSL Socket Closed>");
                }
            }
            else {
                try {
                    socket.close();
                }
                catch (IOException e) {
                    e.printStackTrace();
                }
            }
    }


    /** This is really effective when you have an SSLSocket and you need to make sure
     * that all of the data has been cleared off of the socket prior to the close.
     * Specify with an in whether this side of the connection was either a
     * SENDER_MODE or RECEIVER_MODE.
     *
     * SENDER_MODE: In the event that you were the last to send
     * something on this socket then this closure will wait the make sure that
     * all of the data has been received at the far end before attempting to close
     * the socket.
     *
     * RECEIVER_MODE: In the event that you received last on this socket, then your read
     * should be done by the time that you call this method, so you shouldn't
     * have to wait on any operations to complete on the socket.
     *
     */
    public void disconnect (int mode)
    {
        if (_commHelper != null) {
            if ((_encryptionLevel  == SSL_LOCALPLAIN && !isLocal (_commHelper.getSocket())) ||
                    _encryptionLevel > SSL_LOCALPLAIN) {
                SSLSocket sslSocket = (SSLSocket) _commHelper.getSocket();
                try {
                    if (mode == SENDER_MODE) {
                        sslSocket.waitForClose (true);
                    }
                    sslSocket.close();
                    printDebugLog ("SSLSocket is closed.");
                }
                catch (Exception e) {
                    e.printStackTrace();
                    printDebugLog ("<SSL Socket Closed>");
                }
            }
            else {
                _commHelper.closeConnection();
            }
        }
        else if (_serverSocket != null) {
            try {
                _serverSocket.close();
            }
            catch (Exception e) {
                printDebugLog ("<Server Socket Closed>");
            }
        }
    }

    public void disconnect()
    {
        if (_commHelper != null) {
            if ((_encryptionLevel  == SSL_LOCALPLAIN && !isLocal (_commHelper.getSocket())) ||
                    _encryptionLevel > SSL_LOCALPLAIN) {
                SSLSocket sslSocket = (SSLSocket)_commHelper.getSocket();
                try {
                    sslSocket.sendClose();
                    sslSocket.waitForClose (true);
                    printDebugLog ("SSLSocket is closed.");
                }
                catch (Exception e) {
                    e.printStackTrace();
                    printDebugLog ("<SSL Socket Closed>");
                }
                finally {
                    try {
                        sslSocket.close();
                    }
                    catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
            else {
                _commHelper.closeConnection();
            }
        }
        else if (_serverSocket != null) {
            try {
                _serverSocket.close();
            }
            catch (Exception e) {
                printDebugLog ("<Server Socket Closed>");
            }
        }
    }

    public void setVerification (byte verificationLevel)
    {
        _verificationLevel = verificationLevel;
    }

    protected static void loadProviders()
    {
        try {
            int ret = Security.addProvider (new cryptix.provider.Cryptix());
            printDebugLog ("Able to load Cryptix in " + ret + " position");

            ret = Security.addProvider (new org.bouncycastle.jce.provider.BouncyCastleProvider());
            printDebugLog ("Able to load BC in " + ret + " position");
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * There are extra levels of verification available above and beyond sandard SSL. CPKV is cached
     * public key verification, where precached public keys are stored by their commonName and
     * verified upon reconnection.
     * @return
     */
    protected static byte checkVerificationLevel()
    {
        String verify = ConfigLoader.getDefaultConfigLoader().getProperty ("nomads.verification");

        if (verify != null) {
            if (verify.equalsIgnoreCase ("CPKV")) {
                printDebugLog ("Setting encryption level to CPKV_VERIFICATION for " + Thread.currentThread().toString());
                return CPKV_VERIFICATION;
            }
            else if (verify.equalsIgnoreCase ("mast")) {
                printDebugLog ("Setting encryption level to MAST for " + Thread.currentThread().toString());
                return MAST_VERIFICATION;
            }
            else if (verify.equalsIgnoreCase ("no") || verify.equalsIgnoreCase ("off")) {
                printDebugLog ("Setting encryption level to NO_VERIFICATION for " + Thread.currentThread().toString());
                return NO_VERIFICATION;
            }
        }

        printDebugLog ("WARNING! No verification level set in the config file: setting verification " +
                       "level to NO_VERIFICATION for " + Thread.currentThread().toString());
        return NO_VERIFICATION;
    }

    /**
     * Encryption can be either turned off: NO_ENCRYPTION, which will result in plain sockets for
     * both client and server, or it can be on for any remote connections, but off for local
     * connections: SSL_LOCALPLAIN, or it can be on all the time: SSL_ENCRYPTION.
     * @return
     */
    protected static byte checkEncryptionLevel()
    {
        String encrypt = ConfigLoader.getDefaultConfigLoader().getProperty ("nomads.encryption");

        if (encrypt != null) {
            if (encrypt.equalsIgnoreCase ("ssl")) {
                printDebugLog ("Setting encryption level to SSL for " + Thread.currentThread().toString());
                return SSL_ENCRYPTION;
            }
            else if (encrypt.equalsIgnoreCase ("ssl_localplain")) {
                printDebugLog ("Setting encryption level to SSL_LOCALPLAIN for " + Thread.currentThread().toString());
                return SSL_LOCALPLAIN;
            }
            else if (encrypt.equalsIgnoreCase ("no") || encrypt.equalsIgnoreCase ("off")) {
                printDebugLog ("Setting encryption level to NO_ENCRYPTION for " + Thread.currentThread().toString());
                return NO_ENCRYPTION;
            }
        }

        printDebugLog ("WARNING! No encryption level set in the config file: setting encryption level to OFF for " + Thread.currentThread().toString());
        return NO_ENCRYPTION;
    }

    private static boolean isLocal (Socket socket)
    {
        if (NetUtils.isLocalAddress (socket.getInetAddress()) || socket.getInetAddress().isLoopbackAddress()) {
            printDebugLog ("Socket conn to " + socket.getInetAddress() + ":" + socket.getPort() + " is local for " + Thread.currentThread().toString());
            return true;
        }

        printDebugLog ("Socket conn to " + socket.getInetAddress() + ":" + socket.getPort() + " is NOT local for " + Thread.currentThread().toString());
        return false;
    }

    private static void printDebugLog (String logMessage)
    {
        if (_debug) {
            System.out.println ("ConnectionManager DEBUG: " + logMessage);
        }
    }

    //Class Variables
    private PureTLSServerSocketFactory _serverSocketFactory = null;
    private ServerSocket _serverSocket;
    private CommHelper _commHelper = null;
    private static final HashMap<String, Long> _verifiedPeers;
    public static final byte ENCRYPTION_UNINITIALIZED = -1;
    public static final byte NO_ENCRYPTION = 0;
    public static final byte SSL_LOCALPLAIN = 1;
    public static final byte SSL_ENCRYPTION = 2;
    public static final byte VERIFICATION_UNINITIALIZED = -1;
    public static final byte NO_VERIFICATION = 0;
    public static final byte CPKV_VERIFICATION = 1;
    public static final byte MAST_VERIFICATION = 2;
    public static final int SENDER_MODE = 0;
    public static final int RECEIVER_MODE = 1;
    private byte _encryptionLevel = ENCRYPTION_UNINITIALIZED;
    private byte _verificationLevel = VERIFICATION_UNINITIALIZED;
    private final static boolean _debug = false;

    static
    {
        _verifiedPeers = new HashMap<String, Long>();
    }
}
