package us.ihmc.aci.grpMgrOld;

import java.net.InetAddress;
import java.util.Vector;

import us.ihmc.net.NICInfo;
import us.ihmc.util.ByteArray;
import us.ihmc.util.ByteConverter;


/**
 * Class to encode/decode packets
 *
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class PacketCodec
{

    /**
     *
     * @param packet
     * @param packetType
     * @param sequenceNum
     * @param msgType
     * @param uuid
     * @param hopCount
     * @param floodProb
     * @param message
     * @param messageLen
     * @param nicsInfo
     * @return
     */
    protected static int createPacket (byte[] packet, int packetType, int sequenceNum, int msgType, String uuid,
                                       int hopCount, int floodProb, byte[] message, int messageLen, Vector nicsInfo)
    {
        int headerSize = 0;
        // Write the default signature
        ByteConverter.fromUnsignedIntTo4Bytes (DEFAULT_SIGNATURE, packet, headerSize);
        headerSize += INT_FIELD_LENGTH;

        // Write the packet type
        ByteConverter.fromUnsignedByteTo1Byte ((short) packetType, packet, headerSize);
        headerSize += 1;

        // Write the packet sequence number
        ByteConverter.fromUnsignedShortIntTo2Bytes (sequenceNum, packet, headerSize);
        headerSize += 2;

        // Write the message type
        ByteConverter.fromUnsignedByteTo1Byte ((short) msgType, packet, headerSize);
        headerSize += 1;

        // Write the node uuid
        ByteArray.stringToByteArray (uuid, packet, headerSize, UUID_FIELD_LENGTH);
        headerSize += UUID_FIELD_LENGTH;

        // Write the original hop count (will not be modified by relayer nodes)
        ByteConverter.fromUnsignedByteTo1Byte ((short) hopCount, packet, headerSize);
        headerSize += 1;

        // Write the Hop count
        ByteConverter.fromUnsignedByteTo1Byte ((short) hopCount, packet, headerSize);
        headerSize += 1;

        // Write the Flood probability
        ByteConverter.fromUnsignedByteTo1Byte ((short) floodProb, packet, headerSize);
        headerSize += 1;

        // If needed append to the header the network interfaces information
        if (nicsInfo != null) {
            // Write the number of network interfaces
            ByteConverter.fromUnsignedByteTo1Byte ((short) nicsInfo.size(), packet, headerSize);
            headerSize += 1;

            // Write the network IFs information (IP and netmask)
            for (int i = 0; i < nicsInfo.size(); i++) {
                System.arraycopy (((NICInfo) nicsInfo.elementAt(i)).ip.getAddress(), 0, packet, headerSize, 4);
                headerSize += INT_FIELD_LENGTH;
                System.arraycopy (((NICInfo) nicsInfo.elementAt(i)).netmask.getAddress(), 0, packet, headerSize, 4);
                headerSize += INT_FIELD_LENGTH;
            }
        }
        else {
            packet[headerSize] = (byte) 0;
            headerSize += 1;
        }

        // Append the Message Body (if any)
        if (messageLen > 0) {
            System.arraycopy (message, 0, packet, headerSize, messageLen);
            headerSize += messageLen;
        }

        return headerSize;
    }

    /**
     *
     * @param packet
     * @param packetSize
     * @return
     */
    protected static boolean isPacketValid (byte[] packet, int packetSize)
    {
        return (ByteConverter.from4BytesToUnsignedInt (packet, 0) == DEFAULT_SIGNATURE);
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static int getPacketType (byte[] packet)
    {
        return ByteConverter.from1ByteToUnsignedByte (packet[INT_FIELD_LENGTH]);
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static int getSequenceNumber (byte[] packet)
    {
        return ByteConverter.from2BytesToUnsignedShortInt (packet, INT_FIELD_LENGTH+1);
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static int getMessageType(byte[] packet)
    {
        return ByteConverter.from1ByteToUnsignedByte (packet[INT_FIELD_LENGTH+3]);
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static int getHeaderSize (byte[] packet)
    {
        int pos = 2*INT_FIELD_LENGTH+UUID_FIELD_LENGTH+3;
        short ifNum = ByteConverter.from1ByteToUnsignedByte (packet[pos]);
        return pos+1+INT_FIELD_LENGTH*2*ifNum;
    }

    /**
     *
     * @param packet
     * @param packetLen
     * @return
     */
    protected static byte[] getMessage (byte[] packet, int packetLen)
    {
        int headerLen = getHeaderSize (packet);
        int messageLen = packetLen - headerLen;
        byte[] message = new byte [messageLen];
        System.arraycopy (packet, headerLen , message, 0, messageLen);
        return message;
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static String getNodeUUID (byte[] packet)
    {
        return ByteArray.byteArrayToString (packet, 2*INT_FIELD_LENGTH, UUID_FIELD_LENGTH);
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static int getOrigHopCount (byte[] packet)
    {
        return ByteConverter.from1ByteToUnsignedByte (packet[2*INT_FIELD_LENGTH+UUID_FIELD_LENGTH]);
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static int getHopCount (byte[] packet)
    {
        return ByteConverter.from1ByteToUnsignedByte (packet[2*INT_FIELD_LENGTH+UUID_FIELD_LENGTH+1]);
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static int getFloodProbability (byte[] packet)
    {
        return ByteConverter.from1ByteToUnsignedByte (packet[2*INT_FIELD_LENGTH+UUID_FIELD_LENGTH+2]);
    }

    /**
     *
     * @param packet
     * @param hopCount
     * @return
     */
    protected static int setHopCount (byte[] packet, int hopCount)
    {
        int pos = 2*INT_FIELD_LENGTH+UUID_FIELD_LENGTH+2;
        ByteConverter.fromUnsignedByteTo1Byte ((short) hopCount, packet, pos);
        return 0;
    }

    /**
     *
     * @param packet
     * @return
     */
    protected static Vector getNICsInfo (byte[] packet)
    {
        int pos = 2*INT_FIELD_LENGTH+UUID_FIELD_LENGTH+3;

        // Get the number of network interfaces
        short iFNum = ByteConverter.from1ByteToUnsignedByte (packet [pos++]);
        if (iFNum == 0) {
            return null;
        }
        Vector nicsInfo = new Vector (iFNum);

        for (int i=0; i< iFNum; i++) {
            byte[] ipAdd = new byte[4];
            byte[] netmaskAdd = new byte[4];
            System.arraycopy (packet, pos+INT_FIELD_LENGTH*(i*2), ipAdd, 0, 4);
            System.arraycopy (packet, pos+INT_FIELD_LENGTH*(i*2+1), netmaskAdd, 0, 4);
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


    ////// CONSTANTS ///////

    protected static final long DEFAULT_SIGNATURE = 0xAC00AC00L;

    // Packet Types
    protected static final byte BROADCAST_PACKET = 0x01;
    protected static final byte UNICAST_RELIABLE_PACKET = 0x02;
    protected static final byte UNICAST_UNRELIABLE_PACKET = 0x03;
    protected static final byte ACK_PACKET = 0x04;

    // Sizes
    protected static final int MAX_PACKET_SIZE = 65535;
    private static final byte UUID_FIELD_LENGTH = 64;
    private static final byte INT_FIELD_LENGTH = 4;
}
