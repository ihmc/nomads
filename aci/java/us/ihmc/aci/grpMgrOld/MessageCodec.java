package us.ihmc.aci.grpMgrOld;

import java.io.IOException;
import java.security.PublicKey;
import java.net.InetAddress;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import us.ihmc.net.NICInfo;
import us.ihmc.util.Base64Transcoders;
import us.ihmc.util.ByteArray;
import us.ihmc.util.ByteConverter;
import us.ihmc.util.crypto.CryptoUtils;


/**
 * Class to encode/decode messages
 *
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class MessageCodec
{
    /**
     * Checks if any of the message types is valid.
     *
     * @param message   the message byte array
     * @param msgLen    the message size
     * @param messageType
     *
     * @return <code>true</code> if the message is valid, false otherwise
     */
    protected static boolean isMessageValid (byte[] message, int msgLen, int messageType)
    {
        int pos = 0;

        switch (messageType)
        {
            case MessageCodec.PING_MESSAGE:
            {
                return (msgLen == 0);
            }
            case MessageCodec.INFO_MESSAGE:
            {
                // Checking the consistency of the Node Name Length
                pos += SHORT_FIELD_LENGTH+INT_FIELD_LENGTH;
                if ((pos + SHORT_FIELD_LENGTH) > msgLen) {
                    return false;
                }
                int nodeNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + nodeNameLen + 1;
                if (((pos + SHORT_FIELD_LENGTH) > msgLen) || (message [pos-1] != '\0')) {
                    return false;
                }

                // Checking the consistency of the EPK Field
                int encryptedKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                if (encryptedKeyLen > 0) {
                    pos += SHORT_FIELD_LENGTH + encryptedKeyLen + 1;
                    if (((pos + 1) > msgLen) || (message [pos-1] != '\0')) {
                        return false;
                    }
                }
                else {
                    pos += SHORT_FIELD_LENGTH;
                    if (pos > msgLen) {
                        return false;
                    }
                }

                // Checking the consistency of the groups
                int groupType = 0;
                while ((groupType = message[pos]) != 0) {
                    pos +=1;
                    if (pos > msgLen) {
                        return false;
                    }
                    switch (groupType)
                    {
                        case MessageCodec.PUBLIC_MANAGED_GROUP:
                            int pubGrpNameLen  = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + pubGrpNameLen + 1;
                            if (message [pos-1] != '\0') {
                                return false;
                            }
                            break;

                        case MessageCodec.PRIVATE_MANAGED_GROUP:
                            int resGrpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + resGrpNameLen + 1;
                            if (message [pos-1] != '\0') {
                                return false;
                            }
                            if ((pos + SHORT_FIELD_LENGTH) > msgLen) {
                                return false;
                            }
                            int encNounceLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + encNounceLen + 1;
                            if (message [pos-1] != '\0') {
                                return false;
                            }
                            break;

                        case MessageCodec.PUBLIC_PEER_GROUP:
                            int pubPeerGrpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + pubPeerGrpNameLen + 1;
                            if (message [pos-1] != '\0') {
                                return false;
                            }
                            if ((pos + SHORT_FIELD_LENGTH) > msgLen) {
                                return false;
                            }
                            int dataLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + dataLen;
                            break;

                        case MessageCodec.PRIVATE_PEER_GROUP:
                            int resPeerGrpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + resPeerGrpNameLen + 1;
                            if (message [pos-1] != '\0') {
                                return false;
                            }
                            if ((pos + SHORT_FIELD_LENGTH) > msgLen) {
                                return false;
                            }
                            int resEncPeerGrpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + resEncPeerGrpNameLen + 1;
                            if (message [pos-1] != '\0') {
                                return false;
                            }
                            if ((pos + SHORT_FIELD_LENGTH) > msgLen) {
                                return false;
                            }
                            int resEncNounceLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + resEncNounceLen + 1;
                            if (message [pos-1] != '\0') {
                                return false;
                            }
                            pos += SHORT_FIELD_LENGTH;
                            int encDataLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                            pos += SHORT_FIELD_LENGTH + encDataLen;
                    }
                }
                if (++pos != msgLen) {
                    return false;
                }
                return true;
            }
            case MessageCodec.PEER_SEARCH_MESSAGE:
            {
                pos += UUID_FIELD_LENGTH;
                // Get the number of network interfaces
                short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos]);
                if (pos+1+iFNum*2*INT_FIELD_LENGTH > msgLen) {
                    return false;
                }
                pos += 1+iFNum*2*INT_FIELD_LENGTH;

                // Checking the consistency of the EPK Field
                pos += INT_FIELD_LENGTH + UUID_FIELD_LENGTH;
                int encryptedKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                if (encryptedKeyLen > 0) {
                    pos += SHORT_FIELD_LENGTH + encryptedKeyLen + 1;
                    if (((pos + SHORT_FIELD_LENGTH) > msgLen) || (message [pos-1] != '\0')) {
                        return false;
                    }
                }
                else {
                    pos += SHORT_FIELD_LENGTH;
                    if (pos > msgLen) {
                        return false;
                    }
                }
                
                // Checking the consistency of the Group Name Field
                int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + grpNameLen + 1;
                if (((pos + SHORT_FIELD_LENGTH) > msgLen) || (message [pos-1] != '\0')) {
                    return false;
                }
                // Checking the consistency of the Parameter Field
                int paramLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + paramLen;
                if (pos != msgLen) {
                    return false;
                }
                return true;
            }
            case MessageCodec.PEER_SEARCH_PPG_MESSAGE:
            {
                pos += UUID_FIELD_LENGTH;
                // Get the number of network interfaces
                short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos]);
                if (pos+1+iFNum*2*INT_FIELD_LENGTH > msgLen) {
                    return false;
                }
                pos += 1+iFNum*2*INT_FIELD_LENGTH;

                // Checking the consistency of the EPK Field
                pos += INT_FIELD_LENGTH + UUID_FIELD_LENGTH;
                int encryptedKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                if (encryptedKeyLen > 0) {
                    pos += SHORT_FIELD_LENGTH + encryptedKeyLen + 1;
                    if (((pos + SHORT_FIELD_LENGTH) > msgLen) || (message [pos-1] != '\0')) {
                        return false;
                    }
                }
                else {
                    pos += SHORT_FIELD_LENGTH;
                    if (pos > msgLen) {
                        return false;
                    }
                }

                // Checking the consistency of the Group Name Field
                int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + grpNameLen + 1;
                if (((pos + SHORT_FIELD_LENGTH) > msgLen) || (message [pos-1] != '\0')) {
                    return false;
                }
                // Checking the consistency of the Encrypted Group Name Field
                int encGrpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + encGrpNameLen + 1;
                if (((pos + SHORT_FIELD_LENGTH*2) > msgLen) || (message [pos-1] != '\0')) {
                    return false;
                }
                pos += SHORT_FIELD_LENGTH;
                // Checking the consistency of the Encrypted Parameter Field
                int encParamLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + encParamLen;
                if (pos != msgLen) {
                    return false;
                }
                return true;
            }
            case MessageCodec.PEER_SEARCH_REPLY_MESSAGE:
            {
                // Checking the consistency of the Encrypted Parameter Field
                pos += UUID_FIELD_LENGTH;
                int encParamLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + encParamLen;
                if (pos != msgLen) {
                    return false;
                }
                return true;
            }
            case MessageCodec.GROUP_DATA_MESSAGE:
            {
                if ((pos + SHORT_FIELD_LENGTH) > msgLen) {
                    return false;
                }
                pos += SHORT_FIELD_LENGTH;

                // Checking the consistency of the Node Name Length
                int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + grpNameLen + 1;
                if (((pos + 2*SHORT_FIELD_LENGTH) > msgLen) || (message [pos-1] != '\0')) {
                    return false;
                }
                pos += SHORT_FIELD_LENGTH;
                // Checking the consistency of the Data Length Field
                int dataLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                pos += SHORT_FIELD_LENGTH + dataLen;
                if (pos != msgLen) {
                    return false;
                }
                return true;
            }
            default:
                // Unknown message
                return false;
        }
    }

    /**
     *
     * @param message
     * @param stateSeqNo
     * @param pingInterval
     * @param nodeName
     * @param pubKey
     * @param publicManagedGroups   Hashtable<LocalPublicManagedGroupInfo>
     * @param privateManagedGroups  Hashtable<LocalPrivateManagedGroupInfo>
     * @param publicPeerGroups      Hashtable<LocalPublicPeerGroupInfo>
     * @param privatePeerGroups     Hashtable<LocalPrivatePeerGroupInfo>
     * @return
     */
    protected static int createInfoMessage (byte[] message, int stateSeqNo, int pingInterval, String nodeName,
                                            PublicKey pubKey, Hashtable protectedManagedGroups, Hashtable privateManagedGroups,
                                            Hashtable protectedPeerGroups, Hashtable privatePeerGroups)
    {
        int msgLen = 0;

        // Write the sequence number
        ByteConverter.fromUnsignedShortIntTo2Bytes (stateSeqNo, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        // Write the ping interval
        ByteArray.intToByteArray (pingInterval, message, msgLen);
        msgLen += INT_FIELD_LENGTH;

        // Write the node name
        int nodeLen = nodeName.length();
        ByteConverter.fromUnsignedShortIntTo2Bytes (nodeLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;
        ByteArray.stringToByteArray (nodeName, message, msgLen, nodeLen+1);
        msgLen += nodeLen + 1;    // length + 1 to pad with one NULL byte

        // Write the Public Key
        if (pubKey != null) {
            try {
                String encodedPublicKey = Base64Transcoders.convertByteArrayToB64String (pubKey.getEncoded());
                int keyLen = encodedPublicKey.length();
                ByteConverter.fromUnsignedShortIntTo2Bytes (keyLen, message, msgLen);
                msgLen += SHORT_FIELD_LENGTH;
                ByteArray.stringToByteArray (encodedPublicKey, message, msgLen, keyLen+1);
                msgLen += keyLen + 1;    // length + 1 to pad with one NULL byte
            }
            catch (IOException e) {
                // Should not really happen
                e.printStackTrace();
                return -1;
            }
        }
        else { // do not write the Public Key in the message
            ByteConverter.fromUnsignedShortIntTo2Bytes (0, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
        }
        
        // Write the Local Public Managed groups
        for (Enumeration e = protectedManagedGroups.elements(); e.hasMoreElements();) {
            LocalPublicManagedGroupInfo lpmgi = (LocalPublicManagedGroupInfo) e.nextElement();
            message[msgLen++] = (byte) MessageCodec.PUBLIC_MANAGED_GROUP;

            int nameLen = lpmgi.groupName.length();
            ByteConverter.fromUnsignedShortIntTo2Bytes (nameLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            ByteArray.stringToByteArray (lpmgi.groupName, message, msgLen, nameLen+1);
            msgLen += nameLen + 1;
        }

        // Write the Private Managed Groups
        for (Enumeration e = privateManagedGroups.elements(); (e.hasMoreElements());) {
            LocalPrivateManagedGroupInfo lrmgi = (LocalPrivateManagedGroupInfo) e.nextElement();
            message[msgLen++] = (byte) MessageCodec.PRIVATE_MANAGED_GROUP;

            int nameLen = lrmgi.groupName.length();
            ByteConverter.fromUnsignedShortIntTo2Bytes (nameLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            ByteArray.stringToByteArray (lrmgi.groupName, message, msgLen, nameLen+1);
            msgLen += nameLen + 1;
            int nonceLen = lrmgi.encryptedNonce.length();
            ByteConverter.fromUnsignedShortIntTo2Bytes (nonceLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            ByteArray.stringToByteArray (lrmgi.encryptedNonce, message, msgLen, nonceLen+1);
            msgLen += nonceLen + 1;
        }

        // Write the Local Public Peer groups
        for (Enumeration e = protectedPeerGroups.elements(); e.hasMoreElements();) {
            LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) e.nextElement();
            message[msgLen++] = (byte) MessageCodec.PUBLIC_PEER_GROUP;

            int nameLen = ppgi.groupName.length();
            ByteConverter.fromUnsignedShortIntTo2Bytes (nameLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            ByteArray.stringToByteArray (ppgi.groupName, message, msgLen, nameLen+1);
            msgLen += nameLen + 1;
            int dataLen = 0;
            if (ppgi.data != null) {
                dataLen = ppgi.data.length;
            }
            ByteConverter.fromUnsignedShortIntTo2Bytes (dataLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            if (dataLen > 0) {
                System.arraycopy (ppgi.data, 0, message, msgLen, dataLen);
                msgLen += dataLen;
            }
        }

        // Write the Private Peer groups
        for (Enumeration e = privatePeerGroups.elements(); e.hasMoreElements();) {
            LocalPrivatePeerGroupInfo rpgi = (LocalPrivatePeerGroupInfo) e.nextElement();
            message[msgLen++] = (byte) MessageCodec.PRIVATE_PEER_GROUP;

            int nameLen = rpgi.groupName.length();
            ByteConverter.fromUnsignedShortIntTo2Bytes (nameLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            ByteArray.stringToByteArray (rpgi.groupName, message, msgLen, nameLen+1);
            msgLen += nameLen + 1;
            int encryptedNameLen = rpgi.encryptedGroupName.length();
            ByteConverter.fromUnsignedShortIntTo2Bytes (encryptedNameLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            ByteArray.stringToByteArray (rpgi.encryptedGroupName, message, msgLen, encryptedNameLen+1);
            msgLen += encryptedNameLen + 1;
            int nonceLen = rpgi.encryptedNonce.length();
            ByteConverter.fromUnsignedShortIntTo2Bytes (nonceLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            ByteArray.stringToByteArray (rpgi.encryptedNonce, message, msgLen, nonceLen+1);
            msgLen += nonceLen + 1;

            // Write the data lenght into the message
            ByteConverter.fromUnsignedShortIntTo2Bytes (rpgi.data.length, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;

            // Write encrypted data into message
            int encryptedDataLen = 0;
            if (rpgi.encryptedData != null) {
                encryptedDataLen = rpgi.encryptedData.length;
            }
            ByteConverter.fromUnsignedShortIntTo2Bytes (encryptedDataLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;

            if (encryptedDataLen > 0) {
                System.arraycopy (rpgi.encryptedData, 0, message, msgLen, encryptedDataLen);
                msgLen += encryptedDataLen;
            }

        }

        // Write the message terminator (= no more groups)
        message[msgLen++] = (byte) 0;

        return msgLen;
    }

    /**
     *
     * @param message
     * @param nodeUUID
     * @param nicsInfo Vector<NICInfo>
     * @param ttl
     * @param searchUUID
     * @param pubKey
     * @param groupName
     * @param param
     * @return
     */
    protected static int createPeerSearchMessage (byte[] message, String nodeUUID, Vector nicsInfo, 
                                                  int ttl, String searchUUID, PublicKey pubKey,
                                                  String groupName, byte[] param)
    {
        int msgLen = 0;

        // Write the node uuid
        ByteArray.stringToByteArray (nodeUUID, message, msgLen, UUID_FIELD_LENGTH);
        msgLen += UUID_FIELD_LENGTH;

        // Write the network interfaces information
        if (nicsInfo != null) {
            // Write the number of network interfaces
            ByteConverter.fromUnsignedByteTo1Byte ((short) nicsInfo.size(), message, msgLen++);

            // Write the network IFs information (IP and netmask)
            for (int i = 0; i < nicsInfo.size(); i++) {
                System.arraycopy (((NICInfo) nicsInfo.elementAt(i)).ip.getAddress(), 0, message, msgLen, INT_FIELD_LENGTH);
                msgLen += INT_FIELD_LENGTH;
                System.arraycopy (((NICInfo) nicsInfo.elementAt(i)).netmask.getAddress(), 0, message, msgLen, INT_FIELD_LENGTH);
                msgLen += INT_FIELD_LENGTH;
            }
        }
        else {
            message[msgLen] = (byte) 0;
            msgLen += 1;
        }

        // Write the TTL
        ByteArray.intToByteArray (ttl, message, msgLen);
        msgLen += INT_FIELD_LENGTH;

        // Write the Peer Search UUID string
        ByteArray.stringToByteArray (searchUUID, message, msgLen, UUID_FIELD_LENGTH);
        msgLen += UUID_FIELD_LENGTH;

        // Write the Public Key
        if (pubKey != null) {
            try {
                String encodedPublicKey = Base64Transcoders.convertByteArrayToB64String (pubKey.getEncoded());
                int keyLen = encodedPublicKey.length();
                ByteConverter.fromUnsignedShortIntTo2Bytes (keyLen, message, msgLen);
                msgLen += SHORT_FIELD_LENGTH;
                ByteArray.stringToByteArray (encodedPublicKey, message, msgLen, keyLen+1);
                msgLen += keyLen + 1;    // length + 1 to pad with one NULL byte
            }
            catch (IOException e) {
                // Should not really happen
                e.printStackTrace();
                return -1;
            }
        }
        else { // do not include the Public Key in the message
            ByteConverter.fromUnsignedShortIntTo2Bytes (0, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
        }

        // Write the group length into the message
        int nameLen = groupName.length();
        ByteConverter.fromUnsignedShortIntTo2Bytes (nameLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        // Write the group name into the message
        ByteArray.stringToByteArray (groupName, message, msgLen, nameLen+1);
        msgLen += nameLen + 1;

        // Write the search parameter length into the message
        int paramLen = param.length;
        ByteConverter.fromUnsignedShortIntTo2Bytes (paramLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        // Write the search parameter into the message
        if (paramLen > 0) {
            System.arraycopy (param, 0, message, msgLen, paramLen);
            msgLen += paramLen;
        }

        return msgLen;
    }

    /**
     *
     * @param message
     * @param nodeUUID
     * @param nicsInfo Vector<NICInfo>
     * @param ttl
     * @param searchUUID
     * @param pubKey
     * @param groupName
     * @param encryptedGroupName
     * @param unencryptedParamLen
     * @param encryptedParam
     * @return
     */
    protected static int createPeerSearchPPGMessage (byte[] message, String nodeUUID, Vector nicsInfo, int ttl, 
                                                     String searchUUID, PublicKey pubKey, String groupName, 
                                                     String encryptedGroupName, int unencryptedParamLen,
                                                     byte[] encryptedParam)
    {
        int msgLen = 0;

        // Write the node uuid
        ByteArray.stringToByteArray (nodeUUID, message, msgLen, UUID_FIELD_LENGTH);
        msgLen += UUID_FIELD_LENGTH;

        // Write the network interfaces information
        if (nicsInfo != null) {
            // Write the number of network interfaces
            ByteConverter.fromUnsignedByteTo1Byte ((short) nicsInfo.size(), message, msgLen++);

            // Write the network IFs information (IP and netmask)
            for (int i = 0; i < nicsInfo.size(); i++) {
                System.arraycopy (((NICInfo) nicsInfo.elementAt(i)).ip.getAddress(), 0, message, msgLen, INT_FIELD_LENGTH);
                msgLen += INT_FIELD_LENGTH;
                System.arraycopy (((NICInfo) nicsInfo.elementAt(i)).netmask.getAddress(), 0, message, msgLen, INT_FIELD_LENGTH);
                msgLen += INT_FIELD_LENGTH;
            }
        }
        else {
            message[msgLen] = (byte) 0;
            msgLen += 1;
        }

        // Write the TTL
        ByteArray.intToByteArray (ttl, message, msgLen);
        msgLen += INT_FIELD_LENGTH;

        // Write the Peer Search UUID string
        ByteArray.stringToByteArray (searchUUID, message, msgLen, UUID_FIELD_LENGTH);
        msgLen += UUID_FIELD_LENGTH;

        // Write the Public Key
        try {
            if (pubKey == null) {
                //PEER_SEARCH_PPG Messages need to include the Public Key
                return -1;
            }
            String encodedPublicKey = Base64Transcoders.convertByteArrayToB64String (pubKey.getEncoded());
            int keyLen = encodedPublicKey.length();
            ByteConverter.fromUnsignedShortIntTo2Bytes (keyLen, message, msgLen);
            msgLen += SHORT_FIELD_LENGTH;
            ByteArray.stringToByteArray (encodedPublicKey, message, msgLen, keyLen+1);
            msgLen += keyLen + 1;    // length + 1 to pad with one NULL byte
        }
        catch (IOException e) {
            // Should not really happen
            e.printStackTrace();
            return -2;
        }

        // Write the group name length
        int nameLen = groupName.length();
        ByteConverter.fromUnsignedShortIntTo2Bytes (nameLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        // Write the group name into the message
        ByteArray.stringToByteArray (groupName, message, msgLen, nameLen+1);
        msgLen += nameLen + 1;

        // Write the encrypted group name length 
        int encryptedNameLen = encryptedGroupName.length();
        ByteConverter.fromUnsignedShortIntTo2Bytes (encryptedNameLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        // Write the encrypted group name
        ByteArray.stringToByteArray (encryptedGroupName, message, msgLen, encryptedNameLen+1);
        msgLen += encryptedNameLen + 1;

        // Write search unencrypted parameter length
        ByteConverter.fromUnsignedShortIntTo2Bytes (unencryptedParamLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        // Write search encrypted parameter length
        int encryptedParamLen = encryptedParam.length;
        ByteConverter.fromUnsignedShortIntTo2Bytes (encryptedParamLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        // Write the search encrypted parameter
        if (encryptedParamLen > 0) {
            System.arraycopy (encryptedParam, 0, message, msgLen, encryptedParamLen);
            msgLen += encryptedParamLen;
        }

        return msgLen;
    }

    /**
     *
     * @param message
     * @param searchUUID
     * @param param
     * @return
     */
    protected static int createPeerSearchReplyMessage (byte[] message, String searchUUID, byte[] param)
    {
        int msgLen = 0;

        // Write the Peer Search UUID
        int searchLen = 64;
        ByteArray.stringToByteArray (searchUUID, message, msgLen, searchLen);
        msgLen += searchLen;

        // Write the search reply parameter length
        int paramLen = param.length;
        ByteConverter.fromUnsignedShortIntTo2Bytes (paramLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

         // Write the search reply parameter into the message
        if (paramLen > 0) {
            System.arraycopy (param, 0, message, msgLen, paramLen);
            msgLen += paramLen;
        }

        return msgLen;
    }

    /**
     *
     * @param message
     * @param stateSeqNo
     * @param groupName
     * @param data
     * @return
     */
    protected static int createPeerGroupDataMessage (byte[] message, int stateSeqNo, String groupName, 
                                                     int unencryptedDataLen, byte[] data)
    {
        int msgLen = 0;

        // Write the sequence number
        ByteConverter.fromUnsignedShortIntTo2Bytes (stateSeqNo, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        // Write the group name into the message
        int nameLen = groupName.length();
        ByteConverter.fromUnsignedShortIntTo2Bytes (nameLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;
        ByteArray.stringToByteArray (groupName, message, msgLen, nameLen+1);
        msgLen += nameLen + 1;

        // Write the data into the message
        int dataLen = 0;
        if (data != null) {
            dataLen = data.length;
        }
        ByteConverter.fromUnsignedShortIntTo2Bytes (unencryptedDataLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;
        
        ByteConverter.fromUnsignedShortIntTo2Bytes (dataLen, message, msgLen);
        msgLen += SHORT_FIELD_LENGTH;

        if (dataLen > 0) {
            System.arraycopy (data, 0, message, msgLen, data.length);
            msgLen += dataLen;
        }

        return msgLen;
    }


    //// INFO_MESSAGE ////

    /**
     * Get the state sequence number from the message.
     *
     * @param message    the message byte array
     * @return a number indicating the state sequence number from the message
     */
    protected static int getStateSeqNoFromInfoMessage (byte[] message)
    {
        return ByteConverter.from2BytesToUnsignedShortInt (message, 0);
    }

    /**
     * Get the frequency of the ping request from the info message
     *
     * @param message    the message byte array
     * @return a number indicating the frequency from the info message
     */
    protected static int getPingIntervalFromInfoMessage (byte[] message)
    {
        return ByteArray.byteArrayToInt (message, SHORT_FIELD_LENGTH);
    }

    /**
     * Get the node name from the info message
     *
     * @param message    the message byte array
     * @return the node name from the info message
     */
    protected static String getNodeNameFromInfoMessage (byte[] message)
    {
        int pos = 0;

        // Skip over the StateSeqNo and ping interval
        pos += SHORT_FIELD_LENGTH + INT_FIELD_LENGTH;

        int nodeNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH;

        return ByteArray.byteArrayToString (message, pos, nodeNameLen);
    }

    /**
     * Get the protected key from the info message
     *
     * @param message    the message byte array
     * @return the protected key from the info message
     */
    protected static PublicKey getPublicKeyFromInfoMessage (byte[] message)
    {
        int pos = 0;

        // Skip over the StateSeqNo, Ping Interval and node name
        pos += SHORT_FIELD_LENGTH + INT_FIELD_LENGTH;

        int nodeNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH + nodeNameLen + 1;

        // Get the length of the encoded protected key
        int keyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        if (keyLen > 0) {
            pos += SHORT_FIELD_LENGTH;
            String encodedPublicKey = ByteArray.byteArrayToString (message, pos, keyLen);
            try {
                byte[] protectedKey = Base64Transcoders.convertB64StringToByteArray (encodedPublicKey);
                return CryptoUtils.createPublicKeyFromDEREncodedX509Data (protectedKey);
            }
            catch (Exception e) {
                e.printStackTrace();
                return null;
            }
        }
        else {
            return null;
        }
    }

    /**
     * Parses a info message to get the groups defined in the message.
     * The returned hashtable maps the group name to four different types
     * of GroupInfo objects - RemotePublicManagedGroupInfo, RemotePrivateManagedGroupInfo,
     * RemotePublicPeerGroupInfo, and RemotePrivatePeerGroupInfo.
     * <p>
     * NOTE: A peer group manager should never send a message with two different
     * definitions of the same group (for example, a protected group named "XYZ" and a
     * protected peer group named "XYZ". If this does happen, the last one will overwrite
     * any previous definitions since the groups are added to the hashtable with
     * the group name as the key.
     *
     * @param uuid           the uuid of the info message
     * @param message         the message byte array
     */
    protected static Hashtable getGroupListFromInfoMessage (String uuid, byte[] message)
    {
        int pos = 0;

        // Skip over the StateSeqNo, Ping Interval
        pos += SHORT_FIELD_LENGTH + INT_FIELD_LENGTH;

        // Skip over the Node Name
        int nodeNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH + nodeNameLen + 1;

        // Skip over the public key
        int keyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        if (keyLen > 0) {
            pos += SHORT_FIELD_LENGTH + keyLen + 1;
        }
        else {
            pos += SHORT_FIELD_LENGTH;
        }

        Hashtable groupList = new Hashtable();
        int groupType = 0;
        while ((groupType = message[pos++]) != 0) {
            switch (groupType)
            {
                case MessageCodec.PUBLIC_MANAGED_GROUP:
                {
                    int pubGrpNameLen  = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    String groupName = ByteArray.byteArrayToString (message, pos, 0);
                    pos += pubGrpNameLen + 1;       // + 1 to skip over the null character
                    RemotePublicManagedGroupInfo rpgi = new RemotePublicManagedGroupInfo (groupName, uuid);
                    groupList.put (groupName, rpgi);
                    break;
                }
                case MessageCodec.PRIVATE_MANAGED_GROUP:
                {
                    int nameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    String groupName = ByteArray.byteArrayToString (message, pos, 0);
                    pos += nameLen + 1;
                    int nonceLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    String encryptedNonce = ByteArray.byteArrayToString (message, pos, 0);
                    pos += nonceLen + 1;
                    groupList.put (groupName, new RemotePrivateManagedGroupInfo (groupName, uuid, encryptedNonce));
                    break;
                }
                case MessageCodec.PUBLIC_PEER_GROUP:
                {
                    int nameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    String groupName = ByteArray.byteArrayToString (message, pos, 0);
                    pos += nameLen + 1;       // + 1 to skip over the null character
                    int dataLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    byte[] data = null;
                    if (dataLen > 0) {
                        data = new byte [dataLen];
                        System.arraycopy (message, pos, data, 0, dataLen);
                        pos += dataLen;
                    }
                    RemotePublicPeerGroupInfo rppgi = new RemotePublicPeerGroupInfo (groupName, uuid, data);
                    groupList.put (groupName, rppgi);
                    break;
                }
                case MessageCodec.PRIVATE_PEER_GROUP:
                {
                    int nameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    String groupName = ByteArray.byteArrayToString (message, pos, 0);
                    pos += nameLen + 1;
                    int encryptedNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    String encryptedGroupName = ByteArray.byteArrayToString (message, pos, 0);
                    pos += encryptedNameLen + 1;
                    int nonceLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    String encryptedNonce = ByteArray.byteArrayToString (message, pos, 0);
                    pos += nonceLen + 1;
                    // Get the unencrypted data length
                    int unencryptedDataLength = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    // Get the encrypted data
                    int encryptedDataLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
                    pos += SHORT_FIELD_LENGTH;
                    byte[] encryptedData = null;
                    if (encryptedDataLen > 0) {
                        encryptedData = new byte [encryptedDataLen];
                        System.arraycopy (message, pos, encryptedData, 0, encryptedDataLen);
                        pos += encryptedDataLen;
                    }
                    groupList.put (groupName, new RemotePrivatePeerGroupInfo (uuid, groupName, encryptedGroupName,
                                                                              encryptedNonce, unencryptedDataLength, 
                                                                              encryptedData));
                }
            }
        }
        return groupList;
    }

    //// PEER_SEARCH_MESSAGE ////
    
    /**
     * Get the nodeUUID of the peer that originated the peer search
     *
     * @param message    the message byte array
     * @return nodeUUID
     */ 
    protected static String getNodeUUIDFromPeerSearchMessage (byte[] message)
    {
        return ByteArray.byteArrayToString (message, 0, UUID_FIELD_LENGTH);
    }

    /**
     * Get the Network Interfaces of the peer that originated
     * the peer search
     *
     * @param message    the message byte array
     * @return Vector<NICInfo>
     */ 
    protected static Vector getNICsInfoFromPeerSearchMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;

        // Get the number of network interfaces
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        if (iFNum == 0) {
            return null;
        }
        Vector nicsInfo = new Vector (iFNum);

        for (int i=0; i< iFNum; i++) {
            byte[] ipAdd = new byte[4];
            byte[] netmaskAdd = new byte[4];
            System.arraycopy (message, pos+INT_FIELD_LENGTH*(i*2), ipAdd, 0, 4);
            System.arraycopy (message, pos+INT_FIELD_LENGTH*(i*2+1), netmaskAdd, 0, 4);
            try {
                nicsInfo.addElement (new NICInfo (InetAddress.getByAddress (ipAdd),
                                                  InetAddress.getByAddress (netmaskAdd)));
            }
            catch (Exception e) {
                e.printStackTrace();
            }
       }

        return nicsInfo;
    }

    /**
     * Get the time to live from the peer search message
     *
     * @param message    the message byte array
     * @return the ttl of the peer search message
     */
    protected static int getTTLFromPeerSearchMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH;
        return ByteArray.byteArrayToInt (message, pos);
    }

    /**
     * Update the time to live in the peer search message
     *
     * @param message    the message byte array
     * @param ttl in the peer search message
     */
    protected static void updateTTLInPeerSearchMessage (byte[] message, int ttl)
    {
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH;
        ByteArray.intToByteArray (ttl, message, pos);
    }

    /**
     * Get the search UUID from the peer search message
     *
     * @param message    the message byte array
     * @return the search uuid of the peer search message
     */
    protected static String getSearchUUIDFromPeerSearchMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH + INT_FIELD_LENGTH;
        return ByteArray.byteArrayToString (message, pos,  UUID_FIELD_LENGTH);
    }

     /**
     * Get the protected key from the peer search message
     *
     * @param message    the message byte array
     * @return the protected key of the peer search message
     */
    protected static PublicKey getPublicKeyFromPeerSearchMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH;

        // Skip the TTL and searchUUID
        pos += INT_FIELD_LENGTH + UUID_FIELD_LENGTH;;

        int encKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        if (encKeyLen > 0) {
            pos += SHORT_FIELD_LENGTH;
            String encodedPublicKey = ByteArray.byteArrayToString (message, pos, encKeyLen);
            try {
                byte[] protectedKey = Base64Transcoders.convertB64StringToByteArray (encodedPublicKey);
                return CryptoUtils.createPublicKeyFromDEREncodedX509Data (protectedKey);
            }
            catch (Exception e) {
                e.printStackTrace();
                return null;
            }
        }
        else {
            return null;
        }
    }

    /**
     * Get the group name from the peer search message
     *
     * @param message    the message byte array
     * @return the group name of the peer search message
     */
    protected static String getGroupNameFromPeerSearchMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH;
        // Skip the TTL and searchUUID
        pos += INT_FIELD_LENGTH + UUID_FIELD_LENGTH;;
        // Get the lenght of the Encoded Public Key
        int encKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+encKeyLen+1;
        // Get the lenght of the Group Name
        int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH;
        return ByteArray.byteArrayToString (message, pos, grpNameLen);
    }

    /**
     * Get the group param from the peer search message
     *
     * @param message    the message byte array
     * @return the group param of the peer search message
     */
    protected static byte[] getParamFromPeerSearchMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH;

        // Skip the TTL and searchUUID
        pos += INT_FIELD_LENGTH + UUID_FIELD_LENGTH;;

        // Get the lenght of the Encoded Public Key
        int encKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+encKeyLen+1;

        // Get the length of the group name in order to skip over it
        int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+grpNameLen+1;
        // Get the length of the param
        int paramLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        if (paramLen == 0) {
            return null;
        }
        byte[] param = new byte [paramLen];
        pos +=SHORT_FIELD_LENGTH;
        System.arraycopy (message, pos, param, 0, paramLen);

        return param;
    }

    //// PEER_SEARCH_PPG_MESSAGE /////

    /**
     * Get the nodeUUID of the peer that originated the peer search
     *
     * @param message    the message byte array
     * @return nodeUUID
     */
    protected static String getNodeUUIDFromPeerSearchPPGMessage (byte[] message)
    {
        return getNodeUUIDFromPeerSearchMessage (message);
    }

    /**
     * Get the Network Interfaces of the peer that originated
     * the peer search
     *
     * @param message    the message byte array
     * @return Vector<NICInfo>
     */
    protected static Vector getNICsInfoFromPeerSearchPPGMessage (byte[] message)
    {
        return getNICsInfoFromPeerSearchMessage (message);
    }

    /**
     *
     * @param message
     * @return
     */
    protected static int getTTLFromPeerSearchPPGMessage (byte[] message)
    {
        return getTTLFromPeerSearchMessage (message);
    }

    /**
     *
     * @param message
     * @return
     */
    protected static String getSearchUUIDFromPeerSearchPPGMessage (byte[] message)
    {
        return getSearchUUIDFromPeerSearchMessage (message);
    }

    /**
     *
     * @param message
     * @return
     */
    protected static PublicKey getPublicKeyFromPeerSearchPPGMessage (byte[] message)
    {
        return getPublicKeyFromPeerSearchMessage (message);
    }

    /**
     *
     * @param message
     * @return
     */
    protected static String getGroupNameFromPeerSearchPPGMessage (byte[] message)
    {
        return getGroupNameFromPeerSearchMessage (message);
    }

    /**
     * Get the encrypted group name from the peer search PPG message
     *
     * @param message    the message byte array
     * @return the encrypted group name of the peer search PPG message
     */
    protected static String getEncryptedGroupNameFromPeerSearchPPGMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH;

        // Skip the TTL and searchUUID
        pos += INT_FIELD_LENGTH + UUID_FIELD_LENGTH;;

        int encPubKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);

        // Get the lenght of the Group Name
        pos += SHORT_FIELD_LENGTH+encPubKeyLen+1;
        int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);

        // Get the length of the Encrypted Group Name
        pos += SHORT_FIELD_LENGTH+grpNameLen+1;
        int encryptedGrpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH;

        return ByteArray.byteArrayToString (message, pos, encryptedGrpNameLen);
    }

    /**
     * Get the unencrypted param length from the PEER_SEARCH_RPG message
     *
     * @param message   the message byte array
     * @return the unencrypted param length
     */ 
    protected static int getUnencryptedParamLenFromPeerSearchRPGMessage (byte[] message)
    {        
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH;

        // Skip the TTL and searchUUID
        pos += INT_FIELD_LENGTH + UUID_FIELD_LENGTH;;

        // Get the lenght of the Encoded Public Key
        int encPubKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+encPubKeyLen+1;

        // Get the length of the group name in order to skip over it
        int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+grpNameLen+1;

        // Get the length of the encrypted group name in order to skip over it
        int encryptedGrpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+encryptedGrpNameLen+1;

        return ByteConverter.from2BytesToUnsignedShortInt (message, pos);
    }

     /**
     * Get the encrypted param from the peer search PPG message
     *
     * @param message    the message byte array
     * @return the encrypted param from the peer search PPG message
     */
    protected static byte[] getEncryptedParamFromPeerSearchPPGMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;
        short iFNum = ByteConverter.from1ByteToUnsignedByte (message [pos++]);
        pos += iFNum*2*INT_FIELD_LENGTH;

        // Skip the TTL and searchUUID
        pos += INT_FIELD_LENGTH + UUID_FIELD_LENGTH;;

        // Get the lenght of the Encoded Public Key
        int encPubKeyLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+encPubKeyLen+1;

        // Get the length of the group name in order to skip over it
        int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+grpNameLen+1;

        // Get the length of the encrypted group name in order to skip over it
        int encryptedGrpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+encryptedGrpNameLen+1;

        // Skip the unencrypted param length
        pos += SHORT_FIELD_LENGTH;

        // Get the length of the param
        int encryptedParamLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);

        if (encryptedParamLen < 0) {
            return null;
        }
        pos += SHORT_FIELD_LENGTH;
        byte[] encryptedParam = new byte [encryptedParamLen];
        System.arraycopy (message, pos, encryptedParam, 0, encryptedParamLen);

        return encryptedParam;
    }


    //// PEER_SEARCH_REPLY_MESSAGE /////

    /**
     * Get the search UUID from the peer search reply message
     *
     * @param message    the message byte array
     * @return the search UUID of the peer search reply message
     */
    protected static String getSearchUUIDFromPeerSearchReplyMessage (byte[] message)
    {
        return ByteArray.byteArrayToString (message, 0, UUID_FIELD_LENGTH);
    }

    /**
     * Get the encrypted param from the peer search reply message
     *
     * @param message    the message byte array
     * @return the encrypted Param from the peer search reply message
     */
    protected static byte[] getEncryptedParamFromPeerSearchReplyMessage (byte[] message)
    {
        int pos = UUID_FIELD_LENGTH;
        // Get the length of the param
        int encryptedParamLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH;
        if (encryptedParamLen <= 0) {
            return null;
        }
        byte[] encryptedParam = new byte [encryptedParamLen];
        System.arraycopy (message, pos, encryptedParam, 0, encryptedParamLen);
        return encryptedParam;
    }

    //// GROUP_DATA_MESSAGE /////

    /**
     * Get the state sequence number from the message.
     *
     * @param message    the message byte array
     * @return a number indicating the state sequence number from the message
     */
    protected static int getStateSeqNoFromGroupDataMessage (byte[] message)
    {
        return ByteConverter.from2BytesToUnsignedShortInt (message, 0);
    }

    /**
     * Get the group name from the group data message
     *
     * @param message    the message byte array
     * @return the name of the group from the param message
     */
    protected static String getGroupNameFromGroupDataMessage (byte[] message)
    {
        int pos = SHORT_FIELD_LENGTH;
        int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH;
        return ByteArray.byteArrayToString (message, pos, grpNameLen);
    }

    /**
     * Get the data length (unencrypted) from the PEER_GROUP_DATA message
     *
     * @param message
     * @return data length (unencrypted)
     */ 
    protected static int getDataLenFromGroupDataMessage (byte[] message)
    {
        int pos = SHORT_FIELD_LENGTH;
        // Get the length of the group name in order to skip over it
        int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+grpNameLen+1;

        // Get the length of the data
        return ByteConverter.from2BytesToUnsignedShortInt (message, pos);
    }

    /**
     * Get the a data from the group data message
     *
     * @data message     the message byte array
     * @return the data byte array attached to the group
     */
    protected static byte[] getDataFromGroupDataMessage (byte[] message)
    {
        int pos = SHORT_FIELD_LENGTH;
        // Get the length of the group name in order to skip over it
        int grpNameLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH+grpNameLen+1;

        // Skip the unencrypted data message
        pos += SHORT_FIELD_LENGTH;

        // Get the length of the data
        int dataLen = ByteConverter.from2BytesToUnsignedShortInt (message, pos);
        pos += SHORT_FIELD_LENGTH;
        if (dataLen <= 0) {
            return null;
        }
        byte[] data = new byte [dataLen];
        System.arraycopy (message, pos, data, 0, dataLen);
        return data;
    }

    //// Constants ////

    // Group Types
    protected static final byte PUBLIC_MANAGED_GROUP = 0x00000001;
    protected static final byte PRIVATE_MANAGED_GROUP = 0x00000002;
    protected static final byte PUBLIC_PEER_GROUP = 0x00000003;
    protected static final byte PRIVATE_PEER_GROUP = 0x00000004;

    // Message Types
    protected static final byte PING_MESSAGE = 0x00000001;
    protected static final byte INFO_MESSAGE = 0x00000002;
    protected static final byte GROUP_DATA_MESSAGE = 0x00000004;
    protected static final byte PEER_SEARCH_MESSAGE = 0x00000050;  //80
    protected static final byte PEER_SEARCH_PPG_MESSAGE = 0x00000051; //81
    protected static final byte PEER_SEARCH_REPLY_MESSAGE = 0x00000052;  //82

    protected static final int MAX_MESSAGE_SIZE = 65535;

    private static final byte UUID_FIELD_LENGTH = 64;
    private static final byte INT_FIELD_LENGTH = 4;
    private static final byte SHORT_FIELD_LENGTH = 2;
}

