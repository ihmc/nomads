package us.ihmc.aci.grpMgrOld;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.security.KeyPair;
import java.security.PublicKey;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.ByteConverter;
import us.ihmc.util.crypto.CryptoUtils;
import us.ihmc.util.crypto.SecureInputStream;

/**
 * TCP Server
 *
 * @author Niranjan Suri
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class TCPServer extends Thread
{

    public TCPServer()
    {
        _terminate = false;
        _running = false;
    }

    public void init (String uuid, int port, GroupManager gm, KeyPair keyPair, InetAddress listenAddr)
        throws NetworkException
    {
        try {
            _port = port;
            _groupManager = gm;
            _serverSock = new ServerSocket (_port, 0, listenAddr);
            _keyPair = keyPair;
        }
        catch (Exception e) {
            throw new NetworkException (e.getMessage());
        }
    }

    public void init (String uuid, int port, GroupManager gm, KeyPair keyPair)
        throws NetworkException
    {
        init (uuid, port, gm, keyPair, null);
    }

    /**
     * Returns the running state of this tcp server thread.
     *
     * @return <code>true</code> if the instance is running, false otherwise
     */
    public boolean isRunning()
    {
        return _running;
    }

    /**
     * Request to the tcp server thread to terminate and close the socket.
     */
    public void terminate()
    {
        _terminate = true;
        try {
            _serverSock.close();
        }
        catch (Exception e) {}
    }

    /**
     *
     */
    public void run()
    {
        if (_running) {
            return;
        }

        _running = true;
        try {
            while (!_terminate) {
                Socket s = _serverSock.accept();
                ConnHandler ch = new ConnHandler (s, _groupManager);
                ch.start();
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        _running = false;
    }

    /**
     * ConnHandler thread handles the connection to the group manager
     */
    protected class ConnHandler extends Thread
    {
        /**
         *
         * @param s
         * @param gm
         */
        public ConnHandler (Socket s, GroupManager gm)
        {
            _sock = s;
            _remoteAddr = s.getInetAddress();
            _remotePort = s.getPort();
            _groupManager = gm;
        }

        /**
         *
         */
        public void run()
        {
            try {
                CommHelper chOut = new CommHelper();
                chOut.init (_sock);
                while (true) {
                    String line = chOut.receiveLine();
                    if (line.equalsIgnoreCase ("EncryptedBlock") || line.equalsIgnoreCase ("UnencryptedBlock")) {
                        boolean useEncryption = line.equalsIgnoreCase ("EncryptedBlock");
                        byte[] block = chOut.receiveBlock();
                        ByteArrayInputStream bais = new ByteArrayInputStream (block);
                        CommHelper chIn = new CommHelper();
                        
                        if (useEncryption) {
                            SecureInputStream sis = CryptoUtils.decryptUsingPrivateKey (_keyPair.getPrivate(), bais);
                            chIn.init (sis, null);
                        }
                        else {
                            chIn.init (bais, null);
                        }

                        String cmd = chIn.receiveLine();
                        if (cmd.equalsIgnoreCase ("JOIN PublicManagedGroup")) {
                            handleJoinPublicManagedGroupCmd (chIn, chOut);
                        }
                        else if (cmd.equalsIgnoreCase ("JOIN PrivateManagedGroup")) {
                            handleJoinPrivateManagedGroupCmd (chIn, chOut);
                        }
                        else if (cmd.equalsIgnoreCase ("LEAVE Group")) {
                            handleLeaveGroupCmd (chIn, chOut);
                        }
                        else {
                            chOut.sendLine ("ERROR - unknown command " + cmd);
                            throw new ProtocolException ("received unknown command - <" + cmd +">");
                        }
                    }
                    else if (line.equalsIgnoreCase ("GoodBye")) {
                        break;
                    }
                    else {
                        chOut.sendLine ("ERROR - unknown command " + line);
                        throw new ProtocolException ("received unknown command - <" + line + ">");
                    }
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            try {
                _sock.close();
            }
            catch (Exception e) {}
        }

        /**
         * Handles join a public managed group.
         */
        private void handleJoinPublicManagedGroupCmd (CommHelper chInput, CommHelper chOutput)
            throws ProtocolException, CommException
        {
            try {
                String uuid = chInput.receiveRemainingLine ("UUID ");
                String groupName = chInput.receiveRemainingLine ("GroupName ");
                chInput.receiveMatch ("JoinDataLength");
                byte[] joinDataLen = chInput.receiveBlock();
                int dataLen = (int) ByteConverter.from4BytesToUnsignedInt (joinDataLen, 0);
                byte[] joinData = null;
                if (dataLen > 0) {
                    chInput.receiveMatch ("JoinData");
                    joinData = chInput.receiveBlock();
                }
                chInput.receiveMatch ("PublicKey");
                byte[] publicKeyData = chInput.receiveBlock();
                PublicKey publicKey = CryptoUtils.createPublicKeyFromDEREncodedX509Data (publicKeyData);
                if (!_groupManager.handleJoinPublicManagedGroup (_remoteAddr, _remotePort, uuid, groupName, joinData, publicKey)) {
                    chOutput.sendLine ("ERROR join group failed");
                }
                else {
                    chOutput.sendLine ("OK");
                }
            }
            catch (ProtocolException e) {
                throw e;
            }
            catch (CommException e) {
                throw e;
            }
            catch (Exception e) {
                e.printStackTrace();
                throw new ProtocolException ("failed to handle join command; nested exception - " + e);
            }
        }

        /**
         * Handles joining a private managed group.
         *
         * @param chInput
         * @param chOutput
         * @throws ProtocolException
         * @throws CommException
         */
        private void handleJoinPrivateManagedGroupCmd (CommHelper chInput, CommHelper chOutput)
            throws ProtocolException, CommException
        {
            try {
                String uuid = chInput.receiveRemainingLine ("UUID ");
                String groupName = chInput.receiveRemainingLine ("GroupName ");
                chInput.receiveMatch ("JoinDataLength");
                byte[] joinDataLen = chInput.receiveBlock();
                int dataLen = (int) ByteConverter.from4BytesToUnsignedInt (joinDataLen, 0);
                byte[] joinData = null;
                if (dataLen > 0) {
                    chInput.receiveMatch ("JoinData");
                    joinData = chInput.receiveBlock();
                }
                chInput.receiveMatch ("PublicKey");
                byte[] publicKeyData = chInput.receiveBlock();
                PublicKey publicKey = CryptoUtils.createPublicKeyFromDEREncodedX509Data (publicKeyData);

                String nonce = chInput.receiveRemainingLine ("Nonce ");
                if (!_groupManager.handleJoinPrivateManagedGroup (_remoteAddr, _remotePort, uuid, groupName, joinData, publicKey, nonce)) {
                    chOutput.sendLine ("ERROR join group failed");
                }
                else {
                    chOutput.sendLine ("OK");
                }
            }
            catch (ProtocolException e) {
                throw e;
            }
            catch (CommException e) {
                throw e;
            }
            catch (Exception e) {
                e.printStackTrace();
                throw new ProtocolException ("failed to handle join command; nested exception - " + e);
            }
        }

        /**
         * Handles leaving a group.
         *
         * @param chInput
         * @param chOutput
         * @throws ProtocolException
         * @throws CommException
         */
        private void handleLeaveGroupCmd (CommHelper chInput, CommHelper chOutput)
            throws ProtocolException, CommException
        {
            try {
                String uuid = chInput.receiveRemainingLine ("UUID ");
                String groupName = chInput.receiveRemainingLine ("GroupName ");
                if (!_groupManager.handleLeaveGroup (_remoteAddr, _remotePort, uuid, groupName)) {
                    chOutput.sendLine ("ERROR leave group failed");
                }
                else {
                    chOutput.sendLine ("OK");
                }
            }
            catch (ProtocolException e) {
                throw e;
            }
            catch (CommException e) {
                throw e;
            }
            catch (Exception e) {
                e.printStackTrace();
                throw new ProtocolException ("failed to handle join command; nested exception - " + e);
            }
        }

        private Socket _sock;
        private InetAddress _remoteAddr;
        private int _remotePort;
        private GroupManager _groupManager;
    }

    // Class variables
    private boolean _running;
    private boolean _terminate;
    private GroupManager _groupManager;
    private int _port;
    private KeyPair _keyPair;
    private ServerSocket _serverSock;
}
