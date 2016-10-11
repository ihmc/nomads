/**
 * Commhelper.java
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

import java.io.*;

import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.util.StringTokenizer;

import us.ihmc.io.LineReaderInputStream;
import us.ihmc.util.ByteArray;
import us.ihmc.util.ByteConverter;

/**
 * The CommHelper class represents the lower layer of the Message Protocol.
 * It allows to send text and binary blobs.
 * <p/>
 * modified by author: Maggie Breedy 01/21/04
 *
 * @version $Revision: 1.54 $
 */
public class CommHelper implements CommHelperInterface
{
    /**
     * Constructor for <code>CommHelper</code>
     */
    public CommHelper ()
    {
        _socket = null;
        _setTCPNoDelay = false;
    }

    public CommHelper (boolean setTCPNoDelay)
    {
        _socket = null;
        _setTCPNoDelay = setTCPNoDelay;
    }

    /**
     * Constructor for <code>CommHelper</code>
     * @param s <code>Socket</code> instance
     */
    public CommHelper (Socket s)
    {
        _socket = s;
        _setTCPNoDelay = false;
        init(s);
    }

    /**
     * Initialize the <code>CommHelper</code> with the given Socket object
     * @param s <code>Socket</code> instance
     * @return true if the no errors occurs in the <code>CommHelper</code> initialization
     */
    public final boolean init (Socket s)
    {
        if (s == null) {
            return false;
        }
        _socket = s;

        try {
            if (_setTCPNoDelay) {
                _socket.setTcpNoDelay(true);
            }
            _bufferedOutputStream = new BufferedOutputStream (s.getOutputStream());
            _outputWriter = new OutputStreamWriter (_bufferedOutputStream);
            BufferedInputStream bis = new BufferedInputStream (s.getInputStream());
            _lineReaderInputStream = new LineReaderInputStream (bis);
        }
        catch (IOException e) {
            System.out.println ("CommHelper: IO Exception on " + s.getInetAddress() + ". Message: "+ e.getMessage());
            e.printStackTrace();
            return false;
        }
        catch (Exception e) {
            System.out.println ("CommHelper: Got exception");
            e.printStackTrace();
            return false;
        }

        return true;
    }

    /**
     * Initialize the <code>CommHelper</code> with the specified input and output streams
     * <p/>
     * NOTE: In order to make it easier to use the CommHelper with streams that
     * are "one-way" (such as just reading from a ByteArrayInputStream writing to
     * a ByteArrayOutputStream)
     * @param is <code>InputStream</code> instance
     * @param os <code>OutputStream</code> instance
     * @return true if the no errors occurs in the <code>CommHelper</code> initialization
     */
    public boolean init (InputStream is, OutputStream os)
    {
        _socket = null;
        try {
            if (is != null) {
                BufferedInputStream bis = new BufferedInputStream (is);
                _lineReaderInputStream = new LineReaderInputStream (bis);
            }
            if (os != null) {
                _bufferedOutputStream = new BufferedOutputStream (os);
                _outputWriter = new OutputStreamWriter (_bufferedOutputStream);
            }
        }
        catch (Exception e) {
            System.out.println ("CommHelper: Got exception");
            e.printStackTrace();
            return false;
        }
        return true;
    }

    /**
     * Returns a reference to the socket
     * @return the <code>Socket</code> instance
     */
    public Socket getSocket()
    {
        return (_socket);
    }

    /**
     * Sets class socket to specified timeout
     * @param timeout socket timeout to be set
     * @throws SocketException if a socket error occurs
     */
    public void setSocketTimeout (int timeout) throws SocketException
    {
        _socket.setSoTimeout(timeout);
    }

    /**
     * Send a line terminated terminated by \r\n
     * @param buf line to be sent
     * @throws CommException if problems occur in the writing process
     */
    public synchronized void sendLine (String buf) throws CommException
    {
        try {
            _outputWriter.write (buf);
            _outputWriter.write ("\r\n");
            _outputWriter.flush();
        }
        catch (Exception e) {
            throw new CommException ("Filed to send a line", e);
        }
    }

    /**
     * Send a block of binary data. The data block is preceded by the length of the
     * block sent as a 4 byte value. Designed to conform to behavior of the C++
     * Socket::{send|receive}Block functions.
     *
     * @param buf the data to send
     * @throws CommException in case an error occurs while sending the data
     */
    public void sendBlock (byte[] buf)
            throws CommException
    {
        try {
            byte[] len = ByteArray.intToByteArray(buf.length);
            _bufferedOutputStream.write(len);
            if (buf.length > 0) {
                _bufferedOutputStream.write (buf);
            }
            _bufferedOutputStream.flush();
        }
        catch (Exception e) {
            throw new CommException ("Filed to send a block", e);
        }
        if (_debug) {
            ByteArray.printByteArrayAsHex (buf);
        }
    }

    /**
     * Sends a block containing a <code>String</code>
     * @param string the <code>String</code> to be sent
     * @throws CommException if problems occur in the writing process
     */
    public void sendStringBlock (String string)
            throws CommException
    {
        try {
            if (string == null || string.length() == 0) {
                byte[] len = ByteArray.intToByteArray (0);
                _bufferedOutputStream.write (len);
                _bufferedOutputStream.flush();
            }
            else {
                sendBlock (string.getBytes());
            }
        }
        catch (Exception e) {
            throw new CommException ("Filed to send a sting block", e);
        }
    }

    /**
     * Receive a block of text, no termination. Preceded by length of string.
     * Designed to conform to behavior of Oasis C++ socket::{send|receive}Block
     *
     * @deprecated use the other version that does not specify maxSize
     * @param maxSize maximum size of the block to receive
     * @return the <code>byte[]</code> containing the received block
     * @throws CommException if problems occur in the writing process
     */
    public byte[] receiveBlock (int maxSize)
            throws CommException
    {
        return receiveBlock();
    }

    /**
     * Receive a block of binary data. Assumes the other end will send the length
     * of the block as a 4 byte value followed by the block data. Designed to
     * conform to behavior of the C++ Socket::{send|receive}Block functions
     *
     * @return the block of data received
     * @throws CommException in case an error occurs while sending the data
     */
    public byte[] receiveBlock()
            throws CommException
    {
        byte[] buff = null;
        byte[] sizeBuff = new byte[4];

        try {
            for (int i = 0; i < 4; i++) {
                sizeBuff[i] = (byte) _lineReaderInputStream.read();
            }
            int blockSize = ByteArray.byteArrayToInt (sizeBuff);
            if (blockSize == 0) {
                return null;
            }

            if (blockSize == -1) {
                throw new CommException ("Other end closed socket");
            }

            buff = new byte[blockSize];
            for (int i = 0; i < blockSize; i++) {
                buff[i] = (byte) _lineReaderInputStream.read();
            }
        }
        catch (IOException e) {
            throw new CommException ("Unable to read a line from socket", e);
        }
        if (buff == null) {
            throw new CommException ("Other end closed socket");
        }
        return buff;
    }

    /**
     * Send a string of text, no termination.
     *
     * @deprecated use sendBlob (byte[] buf) instead
     * @param buf <code>String</code> to be sent
     * @throws CommException in case an error occurs while sending the data
     */
    public void sendBlob (String buf)
            throws CommException
    {
        try {
            _outputWriter.write (buf);
            _outputWriter.flush();
        }
        catch (Exception e) {
            throw new CommException(e.getMessage());
        }
    }

    /**
     * Send a block of data
     * @param buf <code>byte[]</code> to be sent
     * @throws CommException in case an error occurs while sending the data
     */
    public void sendBlob (byte[] buf)
            throws CommException
    {
        sendBlob (buf, 0, buf.length);
    }

    /**
     * Send a block of data
     * @param buf <code>byte[]</code> to be sent
     * @param off offset
     * @param len <code>byte[]</code> length
     * @throws CommException in case an error occurs while sending the data
     */
    public void sendBlob (byte[] buf, int off, int len)
            throws CommException
    {
        try {
            _bufferedOutputStream.write (buf, off, len);

            /*try {
                FileOutputStream fileOutputStream = new FileOutputStream (new File ("CommHelperOutput.txt"), true);
                fileOutputStream.write (buf);
                fileOutputStream.close();
            }
            catch (FileNotFoundException e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            } */

            _bufferedOutputStream.flush();
        }
        catch (Exception e) {
            throw new CommException(e.getMessage());
        }
    }

    /**
     * Receive a line of text
     * @return the received line
     * @throws CommException in case an error occurs while reading the data
     */
    public String receiveLine()
            throws CommException
    {
        String line = null;
        try {
            line = _lineReaderInputStream.readLine();
        }
        catch (IOException e) {
            throw new CommException ("Unable to read a line from socket", e);
        }
        if (line == null) {
            throw new CommException ("Other end closed socket");
        }
        return line;
    }

    /**
     * Gets the <code>InputStream</code>
     * @return the <code>InputStream</code> object that this <code>CommHelper</code> is reading from.
     *         (i.e., return the underlying LineReaderInputStream).
     */
    public InputStream getInputStream()
    {
        return _lineReaderInputStream;
    }

    /**
     * Gets the <code>OutputStream</code>
     * @return the <code>OutputStream</code> object that this <code>CommHelper</code> is writing into
     */
    public OutputStream getOutputStream ()
    {
        try {
            return _socket.getOutputStream();
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            return null;
        }
    }

    /**
     * Receive a line of text
     * @param timeout socket timeout after which the read is aborted
     * @return the received line
     * @throws CommException in case an error occurs while reading the data
     * @throws SocketTimeoutException in case the timeout expires
     */
    public String receiveLine (int timeout)
            throws CommException, SocketTimeoutException
    {
        String line = null;
        try {
            _socket.setSoTimeout(timeout);
            line = _lineReaderInputStream.readLine();
        }
        catch (SocketTimeoutException ste) {
            throw ste;
        }
        catch (IOException e) {
            throw new CommException ("Unable to read a line from socket", e);
        }
        if (line == null) {
            throw new CommException ("Other end closed socket");
        }
        return line;
    }

    /**
     * Receives a line of text after discarding the specified initial portion
     * @param startsWith beginning of the line to receive
     * @return the received line
     * @throws CommException in case an error occurs while reading the data
     * @throws ProtocolException in case the line received doesn't begin with what expected
     */
    public String receiveRemainingLine (String startsWith)
            throws CommException, ProtocolException
    {
        String line = receiveLine();
        if (line.regionMatches (true, 0, startsWith, 0, startsWith.length())) {
            return line.substring (startsWith.length(), line.length());
        }
        throw new ProtocolException ("substring not found");
    }

    /**
     * Receives a line and compares it with the specified string
     * @param matchWith matching <code>String</code>
     * @throws CommException in case an error occurs while reading the data
     * @throws ProtocolException in case the line received doesn't match with what expected
     */
    public void receiveMatch (String matchWith)
            throws CommException, ProtocolException
    {
        String line = receiveLine();
        if (!(matchWith.toLowerCase().contains(line.toLowerCase()) || line.toLowerCase().contains(matchWith
                .toLowerCase()))) {
            throw new ProtocolException("no match found: looked for " + matchWith + " in {" + line + "}");
        }
    }

    /**
     * Received a blob of size bufSize from socket
     * @param bufSize size of the blob to receive
     * @return the <code>byte[]</code> containing the blob
     * @throws CommException in case an error occurs while reading the data
     */
    public byte[] receiveBlob (int bufSize)
            throws CommException
    {
        byte[] buf = new byte[bufSize];

        try {
            for (int i = 0; i < bufSize; i++) {
                int ch = _lineReaderInputStream.read();
                if (ch < 0) {
                    throw new CommException ("Other end closed the socket");
                }
                buf[i] = (byte) ch;
            }
        }
        catch (IOException e) {
            throw new CommException ("Unable to read from the socket", e);
        }
        return buf;
    }

    /**
     * Received a blob of size bufSize from socket
     * @param buf <code>byte[]</code> where to receive the data
     * @param off offset
     * @param len blob length
     * @throws CommException in case an error occurs while reading the data
     */
    public void receiveBlob (byte[] buf, int off, int len)
            throws CommException
    {
        try {
            for (int i = 0; i < len; i++) {
                int ch = _lineReaderInputStream.read();
                if (ch < 0) {
                    throw new CommException ("Other end closed the socket");
                }
                buf[off+i] = (byte) ch;
            }
        }
        catch (IOException e) {
            throw new CommException ("Unable to read from the socket", e);
        }
    }


    /**
     * Receives a line and compares it with up to <code>count</code> strings in the array
     * @param count number of strings to compare with
     * @param strings array containing the strings to be compared
     * @return the array index for the match
     * @throws CommException in case an error occurs while reading the data
     * @throws ProtocolException in case no match is found
     */
    public int receiveMatchIndex (int count, String[] strings)
            throws CommException, ProtocolException
    {
        String line = receiveLine();
        for (int i = 0; i < count; i++) {
            if (line.equalsIgnoreCase (strings[i])) {
                return i;
            }
        }
        String outBuff = "";
        for (int i = 0; i < count; i++) {
            outBuff += strings[i] + ", ";
        }
        throw new ProtocolException ("No match found: looked for " + line + " in {" + outBuff + "}");
    }

    /**
     * Combines receiveRemaining and receiveMatchIndex
     * @param count number of strings to compare with
     * @param strings array containing the strings to be compared
     * @param startsWith beginning of the line to receive
     * @return the array index for the match
     * @throws CommException in case an error occurs while reading the data
     * @throws ProtocolException in case no match is found
     */
    public int receiveRemainingMatchIndex (int count, String[] strings, String startsWith)
            throws CommException, ProtocolException
    {
        String line = receiveRemainingLine (startsWith);
        for (int i = 0; i < count; i++) {
            if (line.equalsIgnoreCase (strings[i])) {
                return i;
            }
        }
        throw new ProtocolException ("No match found");
    }

    /**
     * Receives a line and returns an array with one word in each cell
     * @return an array with one word in each cell
     * @throws CommException in case an error occurs while reading the data
     */
    public String[] receiveParsed()
            throws CommException
    {
        String line = receiveLine();
        return parse (line);
    }

    /**
     * Receives a line and returns an array containing words in the line.  How many words are in
     * each cell is dependent on the format string.  A format string of "1 1 2 0" will return an
     * array with 1 word in the first and second cells, 2 words in the third cell, and the rest
     * of the words in the fourth cell
     * @param format format specification for the strings
     * @return the array containing the string in the specified format
     * @throws CommException in case an error occurs while reading the data
     * @throws ProtocolException in case the format is wrong
     */
    public String[] receiveParsedSpecific (String format)
            throws CommException, ProtocolException
    {
        String line = receiveLine();
        return parseSpecific (line, format);
    }

    /**
     * Receives a line of text after discarding the specified initial portion
     * @param startsWith <code>String</code> portion to be ignored
     * @return an array with one word in each cell is returned
     * @throws CommException in case an error occurs while reading the data
     * @throws ProtocolException in case no match is found
     */
    public String[] receiveRemainingParsed (String startsWith)
            throws CommException, ProtocolException
    {
        String line = receiveRemainingLine (startsWith);
        return parse (line);
    }

    /**
     * Receives a line of text after discarding the specified initial portion.  Uses the format string to
     * determine how many words go into each cell.
     * @param format format specification for the strings
     * @param startsWith <code>String</code> portion to be ignored
     * @return an array with the number words specified by the format in each cell
     * @throws CommException in case an error occurs while reading the data
     * @throws ProtocolException in case no match is found
     */
    public String[] receiveRemainingParsedSpecific (String format, String startsWith)
	        throws CommException, ProtocolException
    {
        String line = receiveRemainingLine (startsWith);
        return parseSpecific (line, format);
    }

    /**
     * Parses the <code>String</code> dividing it in single characters
     * @param s <code>String</code> to be parsed
     * @return an array with one character in each cell
     */
    protected String[] parse (String s)
    {
        StringTokenizer st = new StringTokenizer (s);
        int count = st.countTokens();
        String[] strings = new String[count];
        for (int i = 0; i < count; i++) {
            strings[i] = st.nextToken();
        }
        return strings;
    }

    /**
     * Parses the <code>String</code> dividing it in single characters and following a specific format
     * @param s <code>String</code> to be parsed
     * @param format format specification for the strings
     * @return an array with the number words specified by the format in each cell
     */
    protected String[] parseSpecific (String s, String format)
    {
        StringTokenizer st = new StringTokenizer (s);
        String[] formatSpecifiers = parse (format);
        String[] res = new String[formatSpecifiers.length];
        int spec;
        StringBuffer strBuf;
        for (int i = 0; i < formatSpecifiers.length; i++) {
            spec = (new Integer (formatSpecifiers[i])).intValue();
            strBuf = new StringBuffer();
            if (spec == 0) {
                while (st.hasMoreTokens()) {
                    strBuf.append (st.nextToken() + " ");
                }
                res[i] = strBuf.toString().trim();
            }
            else {
                for (int j = 0; j < spec; j++) {
                    strBuf.append (st.nextToken());
                }
                res[i] = strBuf.toString();
            }
        }
        return res;
    }

    /**
     * Sends an <code>Object</code>
     * @param obj <code>Object</code> to be send
     * @throws CommException if an error occurs while sending the object
     */
    public void sendObject (Object obj)
            throws CommException
    {
        // NOTE: The ObjectOutputStream is created on demand here instead of being created
        // when the CommHelper is initialized in order to allow this CommHelper to work
        // with the C++ version
        try {
            if (_objectOutputStream == null) {
                _objectOutputStream = new ObjectOutputStream (_bufferedOutputStream);
            }
            _objectOutputStream.reset();
            _objectOutputStream.writeObject (obj);
            _objectOutputStream.flush();
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            throw new CommException ("Unable to send object", ioe);
        }
    }

    /**
     * Receives an <code>Object</code>
     * @return the <code>Object</code> just received
     * @throws CommException if an error occurs while receiving the object
     */
    public Object receiveObject () throws CommException
    {
        // NOTE: The ObjectInputStream is created on demand here instead of being created
        // when the CommHelper is initialized in order to allow this CommHelper to work
        // with the C++ version
        try {
            if (_objectInputStream == null) {
                _objectInputStream = new ObjectInputStream (_lineReaderInputStream);
            }
            return _objectInputStream.readObject();
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            throw new CommException ("Unable to receive object", ioe);
        }
        catch (ClassNotFoundException cnfe) {
            cnfe.printStackTrace();
            throw new CommException ("Unable to receive object", cnfe);
        }
    }

    /**
     * Receives an <code>Object</code>
     * @param timeout socket timeout
     * @return the <code>Object</code> just received
     * @throws CommException if an error occurs while receiving the object
     * @throws SocketTimeoutException if the timeout expires
     */
    public Object receiveObject (int timeout)
            throws CommException, SocketTimeoutException
    {
        // NOTE: The ObjectInputStream is created on demand here instead of being created
        // when the CommHelper is initialized in order to allow this CommHelper to work
        // with the C++ version
        try {
            _socket.setSoTimeout(timeout);
            if (_objectInputStream == null) {
                _objectInputStream = new ObjectInputStream (_lineReaderInputStream);
            }
            return _objectInputStream.readObject();
        }
        catch (SocketTimeoutException ste) {
            throw ste;
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            throw new CommException ("Unable to receive object", ioe);
        }
        catch (ClassNotFoundException cnfe) {
            cnfe.printStackTrace();
            throw new CommException ("Unable to receive object", cnfe);
        }
    }

    public void writeBool (boolean val)
            throws CommException
    {
        if (val) {
            write8 ((byte)1);
        }
        else {
            write8 ((byte)0);
        }
    }

    /**
     * Writes a byte
     * @param val byte to write
     * @throws CommException if an error occurs while writing the object
     * @throws ProtocolException if an error occurs while writing the object
     */
    public void write8 (byte val)
            throws CommException
    {
        try {
            byte[] buf = new byte[]{val};
            _bufferedOutputStream.write(buf, 0, 1);
            _bufferedOutputStream.flush();
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
    }

    /**
     * Writes a short
     * @param i16Val short to write
     * @throws CommException if an error occurs while writing the object
     * @throws ProtocolException if an error occurs while writing the object
     */
    public void write16 (short i16Val)
            throws CommException, ProtocolException
    {
        try {
            byte[] buf = new byte[2];
            ByteConverter.fromUnsignedShortIntTo2Bytes(i16Val, buf, 0);
            _bufferedOutputStream.write(buf, 0, 2);
            _bufferedOutputStream.flush();
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
    }

    /**
     * Writes a int
     * @param i32Val int to write
     * @throws CommException if an error occurs while writing the object
     */
    public void write32 (int i32Val)
            throws CommException
    {
        try {
            byte[] buf = new byte[4];
            ByteConverter.fromUnsignedIntTo4Bytes(i32Val, buf, 0);
            _bufferedOutputStream.write(buf, 0, 4);
            _bufferedOutputStream.flush();
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
    }

    /**
     * Writes a float
     * @param fVal float to write
     * @throws CommException if an error occurs while writing the object
     * @throws ProtocolException if an error occurs while writing the object
     */
    public void write32 (float fVal)
            throws CommException, ProtocolException
    {
        int i32Val = Float.floatToIntBits (fVal);
        write32 (i32Val);
    }

    /**
     * Writes a double
     * @param fVal double to write
     * @throws CommException if an error occurs while writing the object
     * @throws ProtocolException if an error occurs while writing the object
     */
    public void write32 (double fVal)
            throws CommException, ProtocolException
    {
        long i64Val = Double.doubleToLongBits(fVal);
        write64 (i64Val);
    }

    /**
     * Writes a int
     * @param ui32Val unsigned int to write
     * @throws CommException if an error occurs while writing the object
     */
    public void writeUI32 (long ui32Val)
            throws CommException
    {
        try {
            final long MAX_UINT32 = 0xFFFFFFFFl;
            if ((ui32Val < 0) || (ui32Val > MAX_UINT32)) {
                throw new IllegalArgumentException("The pixel value must be in the [0, " + MAX_UINT32 + "] interval");
            }
            byte[] buf = new byte[4];
            ByteConverter.fromUnsignedIntTo4Bytes(ui32Val, buf, 0);
            _bufferedOutputStream.write(buf, 0, 4);
            _bufferedOutputStream.flush();
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
    }

    /**
     * Writes a long
     * @param i64Val long to write
     * @throws CommException if an error occurs while writing the object
     * @throws ProtocolException if an error occurs while writing the object
     */
    public void write64 (long i64Val)
            throws CommException, ProtocolException
    {
        try {
            byte[] buf = new byte [8];
            ByteConverter.fromLongTo8Bytes(i64Val, buf, 0);
            _bufferedOutputStream.write (buf, 0, 8);
            _bufferedOutputStream.flush ();
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
    }

    /**
     * Reads a byte
     * @return the byte just read
     * @throws CommException if an error occurs while reading the object
     * @throws ProtocolException if the value read is bigger than a byte
     */
    public byte read8 ()
            throws CommException, ProtocolException
    {
        byte[] buf = new byte[1];
        int index = 0;
        int read;
        try {
            while (index < 1) {
                read = _lineReaderInputStream.read (buf, 0, 1 - index);
                index += read;
            }
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
        short value = ByteConverter.from1ByteToUnsignedByte(buf);
        if (value > Byte.MAX_VALUE) {
            throw new ProtocolException ("Conversion from 'short' to 'byte' led to loss of data");
        }

        return (byte) value;
    }

    /**
     * Reads a short
     * @return the short just read
     * @throws CommException if an error occurs while reading the object
     * @throws ProtocolException if the value read is bigger than a short
     */
    public short read16 ()
            throws CommException, ProtocolException
    {
        byte[] buf = new byte[2];
        int index = 0;
        int read;
        try {
            while (index < 2) {
                read = _lineReaderInputStream.read (buf, 0, 2 - index);
                index += read;
            }
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
        int value = ByteConverter.from2BytesToUnsignedShortInt (buf);
        if (value > Short.MAX_VALUE) {
            throw new ProtocolException ("Conversion from 'int' to 'short' led to loss of data");
        }

        return (short) value;
    }

    /**
     * Reads an unsigned int
     * @return the unsigned int just read
     * @throws CommException if an error occurs while reading the object
     * @throws ProtocolException if the value read is bigger than a unsigned int
     */
    public int read32()
            throws CommException, ProtocolException
    {
        byte[] buf = new byte[4];
        int index = 0;
        int read;
        try {
            while (index < 4) {
                read = _lineReaderInputStream.read (buf, 0, 4 - index);
                index += read;
            }
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException(ioe);
            }
        }
        long value = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        if (value > Integer.MAX_VALUE) {
            throw new ProtocolException ("Conversion from 'long' to 'int' led to loss of data");
        }

        return (int) value;
    }

    /**
     * Reads an signed int
     * @return the signed int just read
     * @throws CommException if an error occurs while reading the object
     * @throws ProtocolException if the value read is bigger than a signed int
     */
    public int readI32()
            throws CommException, ProtocolException
    {
        byte[] buf = new byte[4];
        int index = 0;
        int read;
        try {
            while (index < 4) {
                read = _lineReaderInputStream.read (buf, 0, 4 - index);
                index += read;
            }
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
        int value = (int) ByteConverter.from4BytesToSignedInt(buf, 0);

        return value;
    }

    /**
     * Reads an unsigned long
     * @return the unsigned long just read
     * @throws CommException if an error occurs while reading the object
     * @throws ProtocolException if the value read is bigger than a unsigned long
     */
    public long read64 ()
            throws CommException, ProtocolException
    {
        byte[] buf = new byte[8];
        int index = 0;
        int read;
        try {
            while (index < 8) {
                read = _lineReaderInputStream.read (buf, 0, 8 - index);
                index += read;
            }
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
        long value = ByteConverter.from8BytesToLong (buf, 0);

        return value;
    }

    /**
     * Reads an signed long
     * @return the signed long just read
     * @throws CommException if an error occurs while reading the object
     * @throws ProtocolException if the value read is bigger than a signed long
     */
    public long readI64 ()
            throws CommException, ProtocolException
    {
        byte[] buf = new byte[8];
        int index = 0;
        int read;
        try {
            while (index < 8) {
                read = _lineReaderInputStream.read (buf, 0, 8 - index);
                index += read;
            }
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
            if (ioe instanceof SocketException) {
                throw new CommException();
            }
        }
        long value = ByteConverter.from8BytesToSignedLong (buf, 0);

        return value;
    }

    /**
     * Closes the <code>Socket</code> connection
     */
    public void closeConnection ()
    {
        try {
            _socket.close();
        }
        catch (Throwable t) {
            t.printStackTrace();
        }
    }

    /**
     * Returns the number of bytes that can be read (or skipped over) from this input stream without blocking by the
     * next caller of a method for this input stream. The next caller might be the same thread or another thread
     * @return the number of bytes that can be read from this input stream without blocking or -1 if the stream can't be read
     */
    public int available()
    {
        int count = 0;
        try {
            count = _lineReaderInputStream.available();
        }
        catch (IOException e) {
            count = -1;
        }
        return count;
    }

    private final static boolean _debug = false;
    protected Socket _socket;
    private boolean _setTCPNoDelay;

    protected LineReaderInputStream _lineReaderInputStream;
    protected BufferedOutputStream _bufferedOutputStream;
    protected OutputStreamWriter _outputWriter;
    protected ObjectInputStream _objectInputStream;
    protected ObjectOutputStream _objectOutputStream;
}
