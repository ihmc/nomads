package us.ihmc.aci.util;

import java.util.StringTokenizer;
import java.io.IOException;
import java.io.InputStream;

/**
 * ByteArrayHandler
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 * @ Created on Apr 16, 2004 at 2:08:27 PM
 * @ $Date$
 * @ Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class ByteArrayHandler
{
    /**
     * Reads a number of bytes from the inputstream into a byteArray.
     *
     * @param is - Input stream where the read operation will be performed.
     * @param length - size of the byteArray that will be read from
     *          the input stream. It will block until this number of
     *          bytes is read, or the timeout expires.
     * @param timeout - timeout in millisencods. It breaks of the blocking
     *          read method, if expired.
     *          [TODO]- The timeout is not implemented at this point.
     * @return byte[] - the byte-array read from the inputstream
     * @throws java.io.IOException
     */
    public static byte[] readBytes (InputStream is, int length, int timeout)
        throws IOException
    {
        int bytesRead = 0;
        byte[] tempBuf = new byte[MAX_BUF_SIZE];
        while (bytesRead < length) {
            int ec = is.read(tempBuf, bytesRead, length - bytesRead);
            if (ec == -1) {
                break;
            }
            bytesRead = bytesRead + ec;
        }
        byte[] bResult = null;
        if (bytesRead > 0) {
            bResult = new byte[bytesRead];
            for (int i=0; i<bytesRead; i++) {
                bResult[i] = tempBuf[i];
            }
        }
        return (bResult);
    }

    /**
     * Reads a null terminated string from the inputstream into a byteArray.
     * The method read char by char and returns when a null character is found
     * or when the timeout is reached [TODO] - timeout not implemented.
     *
     * @param is - Input stream where the read operation will be performed.
     * @param timeout - timeout in millisencods. It breaks of the blocking
     *          read method, if expired.
     *          [TODO]- The timeout is not implemented at this point.
     * @return byte[] - the byte-array read from the inputstream
     * @throws java.io.IOException
     */
    public static byte[] readNullTermByteArray (InputStream is, int timeout)
        throws IOException
    {
        int bytesRead = 0;
        byte bnext = 0;
        byte[] tempBuf = new byte[MAX_BUF_SIZE];

        while (bnext >= 0) {
            bnext = (byte)is.read();
            if (bytesRead == MAX_BUF_SIZE || bnext == '\0') {
                break;
            }
            tempBuf [bytesRead++] = bnext;
        }

        byte[] bResult = new byte[bytesRead];
        for (int i=0; i<bytesRead; i++) {
            bResult[i] = tempBuf[i];
        }
        return (bResult);
    }


    /**
     * Converts a 4-byte long byte-array into a primitive
     * type (int).
     * @param abyteA - byte-array representing the integer.
     * @param offset - indicates the starting point (byte position)
     *      of the 4-byte sequence that will be read to obtain the
     *      integer.
     * @return int - int converted
     */
    public static int ByteArrayToInt (byte[] abyteA, int offset)
        throws Exception
    {
        int minSize = offset + 4;
        if (abyteA.length < minSize) {
            throw new Exception ("[VideoServMonitor] Invalid Argument length\n Minimum expected size is 4-bytes, found " + abyteA.length + " bytes.");
        }
        int ival = (abyteA[0 +offset]&0xff)<<24 | (abyteA[1+offset]&0xff)<<16 |
                    (abyteA[2+offset]&0xff)<<8 | (abyteA[3+offset]&0xff);
        return ival;
    }


    /**
     * Converts a primitive type (int) into a 4-byte long byte-array.
     *
     * @param n - integer to be converted
     * @return byte[] - byte-array representing the integer.
     */
    public static byte[] intToByteArray (int n)
    {
        byte[] abyte = new byte[4];
        for (int i = 3; i >= 0; i--) {
            abyte[i] = (byte) (n & 0xFF);
            n = (n >> 8);
        }
        return abyte;
    }

    /**
     * Converts an IP address in the form x.x.x.x (as a String) into
     * a byte array (32 bytes long). Note that each octed as converted into
     * a string (32-bytes long) and only the
     *
     * @param sIP
     * @return IPAddres as a 4-byte long byte-array
     */
    public static byte[] convertIPAddToByteArray (String sIP)
    {
        int cursor = 0;
        byte[] bip = new byte[4];

        try {
            StringTokenizer st = new StringTokenizer (sIP,".");
            if (st.countTokens() == 4) {
                while (st.hasMoreTokens()) {
                    int octect = Integer.parseInt(st.nextToken());
                    byte[] octectArray = ByteArrayHandler.intToByteArray (octect);
                    ByteArrayHandler.writeToByteArray(bip, cursor, octectArray[3]);
                    cursor++;
                }
            }
        }
        catch (Exception e) {
            //[TODO] - Remove stack trace for release
            e.printStackTrace();
        }
        return bip;
    }


    /**
     * Converts an 4-byte long byte-arrayt to an IP address in the form x.x.x.x (as a String)
     *
     * @param bIP , the 4-byte long byte array represeting the IP-address.
     * @return a string representing the ip address.
     */
    public static String convertByteArrayToIPAdd (byte[] bIP)
            throws Exception

    {
        String sIP = "";
        if (bIP.length != 4) {
            throw new Exception ("Byte Array MUST BE of size (4-bytes) to be converted to an IP");
        }

        byte[] ival = new byte[4];
        for (int i=0; i<4; i++) {
            ival[3] = bIP[i];
            int octect = ByteArrayHandler.byteArrayToInt(ival,0);
            if (sIP.length()>0) {
                sIP = sIP + ".";
            }
            sIP = sIP + octect + "";
        }
        return (sIP);
    }

    /**
      * Converts a primitive type (long) into a 8-byte long byte-array.
      *
      * @param n - integer to be converted
      * @return byte[] - byte-array representing the long.
      */
     public static byte[] longToByteArray (long n)
     {
         long laux;
         byte[] abyte = new byte[8];

         for (int i = 7; i >= 0; i--) {
             laux = n & 0xFF;
             if (laux > 127) {
                laux -= 256;
             }
             abyte[i] = (byte) (laux & 0xFF);
             n = (n >> 8);
         }
         return abyte;
     }

    /**
     * Converts a 4-byte long byte-array into a primitive type (int).
     *
     * @param abyteA - byte-array representing the integer.
     * @param offset - indicates the starting point (byte position) of the 4-byte sequence that will be read to obtain
     *               the integer.
     * @return int - int converted
     */
    public static int byteArrayToInt (byte[] abyteA, int offset)
            throws Exception
    {
        int minSize = offset + 4;
        if (abyteA.length < minSize) {
            throw new Exception("Minimum expected size is 4-bytes, found " + abyteA.length + " bytes.");
        }
        int ival = (abyteA[0 + offset] & 0xff) << 24 | (abyteA[1 + offset] & 0xff) << 16 |
                (abyteA[2 + offset] & 0xff) << 8 | (abyteA[3 + offset] & 0xff);
        return ival;
    }

    /**
     * Converts a 8-byte long byte-array into a primitive type (long).
     *
     * @param abyteA - byte-array representing the integer.
     * @param offset - indicates the starting point (byte position) of the 8-byte sequence that will be read to obtain
     *               the long.
     * @return long - long converted
     */
    public static long byteArrayToLong (byte[] abyteA, int offset)
            throws Exception
    {
        int minSize = offset + 8;
        if (abyteA.length < minSize) {
            throw new Exception("Minimum expected size is 8-bytes, found " + abyteA.length + " bytes.");
        }

        long lval = 0;
        for (int i=0; i<8; i++) {
            long laux = abyteA[offset + i];
            if (i > 0 & laux < 0) {
                laux = laux +=256;
            }
            lval *= 256;
            lval += laux;
        }
        return lval;
    }

    /**
     * Creates a new byteArray containing the concatenation of two byte-arrays given as arguments. The size of the
     * resultant byte-array is equal to the sum of bPart1 and bPart2.
     *
     * @param bPart1 is the first byte-array
     * @param bPart2 is the second byte-array that will be appended to bPart1.
     * @return bArray   a new byteArray containing the concatenation of bPart1 and bPart2. If both arguments are null,
     *         this method returns null. If one of the arguments is null, this method returns the other (not-null)
     *         argument.
     */
    public static byte[] appendByteArray (byte bPart1[], byte bPart2[])
    {
        if (bPart1 == null) {
            return (byte[]) bPart2.clone();
        }
        if (bPart2 == null) {
            return (byte[]) bPart1.clone();
        }

        byte[] bArray = new byte[bPart1.length + bPart2.length];
        for (int i = 0; i < bPart1.length; i++) {
            bArray[i] = bPart1[i];
        }
        for (int i = 0; i < bPart2.length; i++) {
            bArray[i + bPart1.length] = bPart2[i];
        }
        return bArray;
    }

    /**
     * Writes sequence of bytes 'src' into byteArray 'dest'. The writing will start in
     * position 'destOffset' on the array 'dest'. Only 'length' bytes will be copied
     * from 'src' to 'dest' and the copy will start in position 'srcOffset on 'src'
     *
     * @param dest  destination array, where the bytes will be copied to
     * @param destOffset stating position in the destination array
     * @param src   source array, where the bytes will be read from
     * @param srcOffset starting position in the source array
     * @param length  number of bytes to be copied from src to dest
     * @return int position of the destination cursor after the end of the write (end of array)
     * @throws ArrayIndexOutOfBoundsException if data can't fit in the dest array
     */
    public static int writeToByteArray (byte dest[], int destOffset, byte src[], int srcOffset, int length)
        throws ArrayIndexOutOfBoundsException
    {
        int minSize = destOffset + length;
        if (dest.length < minSize) {
            throw new ArrayIndexOutOfBoundsException("Minimum expected size is " + minSize + ", found " +
                                                     dest.length + " bytes.");
        }

        int k = destOffset;
        for (int i = srcOffset; i < (srcOffset + length); i++) {
            if (i < src.length) {
                dest[k++] = src[i];
            }
        }
        return k;
    }

    /**
     * Writes byte bSrc into byteArray 'dest'. The writing will start in
     * position 'destOffset' on the array 'dest'.
     *
     * @param dest  destination array, where the bytes will be copied to
     * @param destOffset stating position in the destination array
     * @param bSrc   source byte, where the bytes will be read from
     * @return int position of the destination cursor after the end of the write (end of array)
     * @throws ArrayIndexOutOfBoundsException if data can't fit in the dest array
     */
    public static int writeToByteArray (byte dest[], int destOffset, byte bSrc)
        throws ArrayIndexOutOfBoundsException
    {
        int minSize = destOffset + 1;
        if (dest.length < minSize) {
            throw new ArrayIndexOutOfBoundsException("Minimum expected size is " + minSize + ", found " + dest.length + " bytes.");
        }
        int k = destOffset;
        dest[k++] = bSrc;
        return k;
    }

    /**
     * Reads a part of a byte-array and returns the segment as a new byte-array.
     *
     * @param src  destination array, where the bytes will be copied to
     * @param start starting position in the src array
     * @param end   ending (exclusive) position in the src array, The last byte read into the
     *              returned array will be inposition (end-1).
     * @throws ArrayIndexOutOfBoundsException if bounds are invalid
     */
    public static byte[] subArray (byte src[], int start, int end)
        throws ArrayIndexOutOfBoundsException
    {
        if (start < 0 || start >= src.length) {
            throw new ArrayIndexOutOfBoundsException("Invalid 'start' argument.");
        }
        if (start >= end) {
            throw new ArrayIndexOutOfBoundsException("Invalid argument. 'start' must be < than 'end'.");
        }
        if (end < 0 || end > src.length) {
            throw new ArrayIndexOutOfBoundsException("Invalid 'end' argument.");
        }

        int rsltLength = end - start;
        byte[] result = new byte[rsltLength];
        for (int i=0; i<rsltLength; i++) {
            result[i] = src[i+start];
        }
        return result;
    }


    private static final int MAX_BUF_SIZE = 9086;
}
