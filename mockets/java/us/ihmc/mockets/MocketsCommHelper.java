/*
 * MocketsCommHelper.java
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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
 *
 * The <code>MocketsCommHelper</code> class is the implementation of the <code>CommHelper</code> based on Mockets
 * The <code>CommHelper</code> represents the lower layer of the Message Protocol.
 * It allows to send text and binary blobs.
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */

package us.ihmc.mockets;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelperInterface;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.ByteConverter;

import java.io.IOException;
import java.util.Arrays;
import java.util.StringTokenizer;

public class MocketsCommHelper implements CommHelperInterface
{
    /**
     * Constructor for <code>MocketsCommHelper</code>
     */
    public MocketsCommHelper ()
    {
        _mocket = null;
    }

    /**
     * Constructor for <code>MocketsCommHelper</code>
     *
     * @param m <code>Mocket</code> instance
     */
    public MocketsCommHelper (Mocket m)
    {
        _mocket = m;
    }

    /**
     * Initialize the <code>MocketsCommHelper</code> with the given Mocket object
     *
     * @param m <code>Mocket</code> instance
     * @return true if the no errors occurs in the <code>MocketsCommHelper</code> initialization
     */
    public boolean init (Mocket m)
    {
        if (m == null) {
            return false;
        }
        _mocket = m;

        return true;
    }

    /**
     * Returns a reference to the mocket
     *
     * @return the <code>Mocket</code> instance
     */
    public Mocket getMocket ()
    {
        return (_mocket);
    }

    /**
     * Send a line terminated terminated by \r\n
     *
     * @param line line to be sent
     * @throws CommException if problems occur in the writing process
     */
    public synchronized int sendLine (String line) throws CommException
    {
        try {
            byte[] buf = line.getBytes();
            int rc = _mocket.send(true, true, buf, 0, (short) 5, 0, 0);
            if (rc < 0) {
                throw new CommException("Unable to send data on the mocket");
            }

            return line.getBytes().length;
        }
        catch (Exception e) {
            throw new CommException("Filed to send a line", e);
        }
    }

    /**
     * Send a block of data
     *
     * @param buf <code>byte[]</code> to be sent
     * @throws CommException in case an error occurs while sending the data
     */
    public void sendBlob (byte[] buf)
            throws CommException
    {
        try {
            int rc = _mocket.send(true, true, buf, 0, (short) 5, 0, 0);
            if (rc < 0) {
                throw new CommException("Unable to send data on the mocket");
            }
        }
        catch (Exception e) {
            throw new CommException("Filed to send a line", e);
        }
    }

    public void sendBlob (byte[] buf, int off, int len)
            throws CommException
    {
        try {
            int rc = _mocket.send(true, true, buf, off, len, 0, (short) 5, 0, 0);
            if (rc < 0) {
                throw new CommException("Unable to send data on the mocket");
            }
        }
        catch (Exception e) {
            throw new CommException("Filed to send a line", e);
        }
    }

    /**
     * Receive a line of text
     *
     * @return the received line
     * @throws CommException in case an error occurs while reading the data
     */
    public String receiveLine () throws CommException
    {
        return receiveLine(-1);
    }

    /**
     * Receive a line of text waiting until the specified timeout
     *
     * @param timeout the timeout
     * @return the received line
     * @throws CommException in case an error occurs while reading the data
     */
    public String receiveLine (long timeout) throws CommException
    {
        int rc;
        try {
            byte[] buf = _mocket.receive(timeout);
            if (buf != null) {
                return new String(buf);
            }
            else {
                throw new CommException("An error occurred in the receive method call");
            }
        }
        catch (IOException e) {
            throw new CommException("Problem in receiving the line", e);
        }
    }


    /**
     * Receives a line of text after discarding the specified initial portion
     *
     * @param startsWith beginning of the line to receive
     * @return the received line
     * @throws CommException     in case an error occurs while reading the data
     * @throws ProtocolException in case the line received doesn't begin with what expected
     */
    public String receiveRemainingLine (String startsWith) throws CommException, ProtocolException
    {
        String line = receiveLine();
        if (line.regionMatches(true, 0, startsWith, 0, startsWith.length())) {
            return line.substring(startsWith.length(), line.length());
        }
        throw new ProtocolException("Substring not found");
    }

    /**
     * Receives a line of text after discarding the specified initial portion
     *
     * @param startsWith <code>String</code> portion to be ignored
     * @return an array with one word in each cell is returned
     * @throws CommException     in case an error occurs while reading the data
     * @throws ProtocolException in case no match is found
     */
    public String[] receiveRemainingParsed (String startsWith)
            throws CommException, ProtocolException
    {
        String line = receiveRemainingLine(startsWith);
        return parse(line);
    }

    /**
     * Receives a line and compares it with the specified string
     *
     * @param matchWith matching <code>String</code>
     * @throws CommException     in case an error occurs while reading the data
     * @throws ProtocolException in case the line received doesn't match with what expected
     */
    public void receiveMatch (String matchWith)
            throws CommException, ProtocolException
    {
        receiveMatch(matchWith, -1);
    }

    /**
     * Receives a line and compares it with the specified string, waiting until the timeout expires
     *
     * @param matchWith matching <code>String</code>
     * @throws CommException     in case an error occurs while reading the data
     * @throws ProtocolException in case the line received doesn't match with what expected
     */
    public void receiveMatch (String matchWith, long timeout)
            throws CommException, ProtocolException
    {
        String line = receiveLine();
        if (!(matchWith.toLowerCase().contains(line.toLowerCase()) ||
                line.toLowerCase().contains(matchWith.toLowerCase()))) {
            throw new ProtocolException("No match found: looked for " + matchWith + " in {" + line + "}");
        }
    }

    /**
     * Received a blob of size bufSize from socket
     *
     * @param bufSize size of the blob to receive
     * @return the <code>byte[]</code> containing the blob
     * @throws CommException in case an error occurs while reading the data
     */
    public byte[] receiveBlob (int bufSize)
            throws CommException
    {
        byte[] buf = new byte[bufSize];

        try {
            int rc = _mocket.receive(buf, 0, bufSize);
            if (rc == bufSize) {
                return buf;
            }
            else {
                if (rc > 0) {
                    return Arrays.copyOf(buf, rc);
                }
                else if (rc == -1) {
                    throw new CommException("An error occurred in the receive method call");
                }
                else if (rc == -10) {
                    throw new CommException("The object mocket is not initialized, mocket is null");
                }
                else if (rc == -11) {
                    throw new CommException("The parameter buffer is invalid, buffer is null");
                }
                //throw new CommException ("The length of the buffer doesn't correspond to the expected one");
                throw new CommException("Unable to process data of size " + rc);
            }
        }
        catch (IOException e) {
            throw new CommException("Unable to read from the mocket", e);
        }
    }

    /**
     * Writes a int
     *
     * @param i32Val int to write
     * @throws CommException if an error occurs while writing the object
     */
    public void write32 (int i32Val) throws CommException
    {
        try {
            byte[] buf = new byte[4];
            ByteConverter.fromUnsignedIntTo4Bytes(i32Val, buf, 0);
            int rc = _mocket.send(true, true, buf, 0, (short) 5, 0, 0);

            if (rc < 0) {
                throw new CommException("Unable to send data on the mocket");
            }
        }
        catch (IOException e) {
            throw new CommException("Unable to send on the mocket", e);
        }
    }

    /**
     * Sends a byte value through the <code>Mocket</code>
     *
     * @param val byte to write
     * @throws CommException if an error occurs while writing the object
     */
    public void write8 (byte val) throws CommException
    {
        try {
            byte[] buf = new byte[]{val};
            int rc = _mocket.send(true, true, buf, 0, (short) 5, 0, 0);
            if (rc < 0) {
                throw new CommException("Unable to send data on the mocket");
            }
        }
        catch (IOException e) {
            throw new CommException("Unable to send on the mocket", e);
        }
    }

    /**
     * Receives a byte (8-bit) val through the <code>Mocket</code>.
     *
     * @return the byte read
     * @throws CommException     if an error occurs while reading the object
     * @throws ProtocolException if the value read is bigger than a unsigned int
     */
    public byte read8 () throws CommException, ProtocolException
    {
        byte[] buf = new byte[1];
        try {
            int rc = _mocket.receive(buf, 0, 1);
            if (rc == 1) {
                short value = ByteConverter.from1ByteToUnsignedByte(buf);
                if (value > Byte.MAX_VALUE) {
                    throw new ProtocolException("Conversion from 'short' to 'byte' led to loss of data");
                }
                return (byte) value;
            }
            else {
                if (rc == -1) {
                    throw new CommException("An error occurred in the receive method call");
                }
                else if (rc == -10) {
                    throw new CommException("The object mocket is not initialized, mocket is null");
                }
                else if (rc == -11) {
                    throw new CommException("The parameter buffer is invalid, buffer is null");
                }
                throw new CommException("The length of the buffer doesn't correspond to the expected one");
            }
        }
        catch (IOException e) {
            throw new CommException("Problem in receiving the int value", e);
        }
    }


    /**
     * Reads an unsigned int
     *
     * @return the unsigned int just read
     * @throws CommException     if an error occurs while reading the object
     * @throws ProtocolException if the value read is bigger than a unsigned int
     */
    public int read32 () throws CommException, ProtocolException
    {
        byte[] buf = new byte[4];
        try {
            int rc = _mocket.receive(buf, 0, 4);
            if (rc == 4) {
                long value = ByteConverter.from4BytesToUnsignedInt(buf, 0);
                if (value > Integer.MAX_VALUE) {
                    throw new ProtocolException("Conversion from 'long' to 'int' led to loss of data");
                }
                return (int) value;
            }
            else {
                if (rc == -1) {
                    throw new CommException("An error occurred in the receive method call");
                }
                else if (rc == -10) {
                    throw new CommException("The object mocket is not initialized, mocket is null");
                }
                else if (rc == -11) {
                    throw new CommException("The parameter buffer is invalid, buffer is null");
                }
                throw new CommException("The length of the buffer doesn't correspond to the expected one");
            }
        }
        catch (IOException e) {
            throw new CommException("Problem in receiving the int value", e);
        }
    }

    /**
     * Receives a line and returns an array with one word in each cell
     *
     * @return an array with one word in each cell
     * @throws CommException in case an error occurs while reading the data
     */
    public String[] receiveParsed ()
            throws CommException
    {
        String line = receiveLine();
        return parse(line);
    }

    /**
     * Receives a line and returns an array containing words in the line.  How many words are in
     * each cell is dependent on the format string.  A format string of "1 1 2 0" will return an
     * array with 1 word in the first and second cells, 2 words in the third cell, and the rest
     * of the words in the fourth cell
     *
     * @param format format specification for the strings
     * @return the array containing the string in the specified format
     * @throws CommException     in case an error occurs while reading the data
     * @throws ProtocolException in case the format is wrong
     */
    public String[] receiveParsedSpecific (String format)
            throws CommException, ProtocolException
    {
        String line = receiveLine();
        return parseSpecific(line, format);
    }

    /**
     * Parses the <code>String</code> dividing it in single characters and following a specific format
     *
     * @param s      <code>String</code> to be parsed
     * @param format format specification for the strings
     * @return an array with the number words specified by the format in each cell
     */
    protected String[] parseSpecific (String s, String format)
    {
        StringTokenizer st = new StringTokenizer(s);
        String[] formatSpecifiers = parse(format);
        String[] res = new String[formatSpecifiers.length];
        int spec;
        StringBuffer strBuf;
        for (int i = 0; i < formatSpecifiers.length; i++) {
            spec = (new Integer(formatSpecifiers[i])).intValue();
            strBuf = new StringBuffer();
            if (spec == 0) {
                while (st.hasMoreTokens()) {
                    strBuf.append(st.nextToken() + " ");
                }
                res[i] = strBuf.toString().trim();
            }
            else {
                for (int j = 0; j < spec; j++) {
                    strBuf.append(st.nextToken());
                }
                res[i] = strBuf.toString();
            }
        }
        return res;
    }

    /**
     * Parses the <code>String</code> dividing it in single characters
     *
     * @param s <code>String</code> to be parsed
     * @return an array with one character in each cell
     */
    protected String[] parse (String s)
    {
        StringTokenizer st = new StringTokenizer(s);
        int count = st.countTokens();
        String[] strings = new String[count];
        for (int i = 0; i < count; i++) {
            strings[i] = st.nextToken();
        }
        return strings;
    }

    /**
     * Closes the <code>Mocket</code> connection
     */
    public void closeConnection ()
    {
        try {
            _mocket.close();
        }
        catch (Throwable t) {
            t.printStackTrace();
        }
    }

    protected Mocket _mocket;
}

