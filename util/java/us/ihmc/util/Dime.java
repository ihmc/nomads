/*
 * Dime.java
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

/**
 * Dime
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.util;

import java.io.*;
import java.util.ArrayList;
import java.util.List;

/**
 * Manages a list of <code>DimeRecord</code> instances
 */
public class Dime
{
    /**
     * Constructor
     */
    public Dime()
    {
        _records = new ArrayList<DimeRecord>();
    }

    /**
     * Creates a dime object by reading and parsing the byte array
     * @param dimeMsg byte array representation of the dime
     */
    public Dime (byte[] dimeMsg)
    {
        this();
        parseDime (dimeMsg);
    }

    /**
     * Creates a dime object by reading it from the input stream
     * @param is input stream containing the dime object
     * @throws IOException if problems occur while reading from the input stream
     */
    public Dime (InputStream is) throws IOException
    {
        this();
        parseDime(is);
    }

    /**
     * Adds a new record to the dime object
     * @param payload byte array representation of the payload
     * @return true if the record is correctly added
     */
    public boolean addRecord (byte[] payload)
    {
        return addRecord ("", payload, "");
    }

    /**
     * Adds a new record to the dime object
     * @param payloadType payload type
     * @param payload byte array representation of the payload
     * @param payloadIdentifier payload identifier
     * @return true if the record is correctly added
     */
    public boolean addRecord (String payloadType, byte[] payload, String payloadIdentifier)
    {
        DimeRecord dr = new DimeRecord (payloadType, payload, payloadIdentifier);
        _records.add (dr);

        return true;
    }

    /**
     * Adds a new record to the dime object
     * @param payloadType payload type
     * @param payload byte array representation of the payload
     * @param payloadIdentifier payload identifier
     * @return true if the record is correctly added
     */
    public boolean addRecord (DimeRecord.PayloadType payloadType, byte[] payload, String payloadIdentifier)
    {
        DimeRecord dr = new DimeRecord (payloadType, payload, payloadIdentifier);
        _records.add (dr);

        return true;
    }

    /**
     * Writes the dime in the byte array
     * @param buff byte array where the dime will be written into
     * @return true if the dime is correctly written in the byte array
     */
    public boolean getDime (byte[] buff)
    {
        if (_records.isEmpty()) {
            return false;
        }

        int index = 0;

        //reset the MB and ME flags in all the records.
        for (DimeRecord dr :  _records) {
            dr.setMessageBeginFlag (false);
            dr.setMessageEndFlag (false);
        }

        _records.get (0).setMessageBeginFlag (true);
        _records.get (_records.size()-1).setMessageEndFlag (true);

        for (DimeRecord dr :  _records) {
            dr.getRecord (buff, index);
            index += dr.getRecordLength();
        }

        return true;
    }

    /**
     * Gets the dime as byte array
     * @return the dime as byte array
     */
    public byte[] getDime()
    {
        if (_records.isEmpty()) {
            return null;
        }

        byte[] buff = new byte[getDimeLength()];
        getDime (buff);

        return buff;
    }

    /**
     * Writes the dime in the output stream
     * @param os output stream where the time is written into
     * @throws IOException if problems occur in writing the dime
     */
    public void getDime (OutputStream os) throws IOException
    {
        if (_records.isEmpty()) {
            return;
        }

        //reset the MB and ME flags in all the records.
        for (DimeRecord dr :  _records) {
            dr.setMessageBeginFlag (false);
            dr.setMessageEndFlag (false);
        }

        _records.get (0).setMessageBeginFlag(true);
        _records.get (_records.size()-1).setMessageEndFlag(true);

        for (DimeRecord dr :  _records) {
            dr.getRecord (os);
        }
    }

    /**
     * Gets the dime size as sum of the single records length
     * @return the dime length as sum of the single records length
     */
    public int getDimeLength()
    {
        int length = 0;
        for (DimeRecord dr :  _records) {
            length += dr.getRecordLength();
        }

        return length;
    }

    /**
     * Retrieves all the <code>DimeRecord</code> instances
     * @return the list of <code>DimeRecord</code>
     */
    public List<DimeRecord> getRecords()
    {
        return _records;
    }

    /**
     * Retrieves the next <code>DimeRecord</code> and updates the records offset
     * @return the next <code>DimeRecord</code> or null if no more records are present
     */
    public DimeRecord getNextRecord()
    {
        if (_recordsOffset >= _records.size()) {
            return null;
        }

        DimeRecord dimeRecord = _records.get (_recordsOffset);
        _recordsOffset++;

        return dimeRecord;
    }

    /**
     * Resets the records offset
     */
    public void resetRecordsOffset()
    {
        _recordsOffset = 0;
    }

    /**
     * Returns the number of records in the dime
     * @return the number of records in the dime
     */
    public int getRecordCount()
    {
        return _records.size();
    }

    /**
     * Prints the dime on standard output
     */
    public void print()
    {
        System.out.println ("::: Start of DIME Message :::");
        System.out.println ("::: TotalDime size is " + getDimeLength() + " bytes :::");

        for (DimeRecord dr :  _records) {
            System.out.println ("..............................................");
            dr.print();;
        }

        System.out.println ("::: End of DIME Message :::");
    }

    /**
     * Parses the dime contained in the byte array
     * @param dimeMsg byte array containing the dime
     */
    private void parseDime (byte[] dimeMsg)
    {
        if (dimeMsg == null) {
            return;
        }

        int index = 0;

        while (true) {
            //get the MB, ME, CF flags
            boolean messageBeginFlag = (dimeMsg[index] & 0x80) == 0x80;
            boolean messageEndFlag = (dimeMsg[index] & 0x40) == 0x40;
            boolean chunkFlag = (dimeMsg[index] & 0x20) == 0x20;

            //get the ID_LENGTH field
            short idLength = ByteArray.byteArrayToShort (dimeMsg, index);
            idLength &= 0x1FFF;

            //get the TNF field

            //get the TYPE_LENGTH field
            short typeLength = ByteArray.byteArrayToShort (dimeMsg, index + 2);
            typeLength &= 0x1FFF;

            //get the DATA_LENGTH field
            int dataLength = ByteArray.byteArrayToInt (dimeMsg, index + 4);

            // ---
            index += 8;
            // ---

            //get the ID field
            String id = "";
            if (idLength > 0) {
                id = ByteArray.byteArrayToString (dimeMsg, index, idLength);
            }
            index += (idLength % 4 == 0) ? idLength : (idLength + 4 - (idLength % 4));

            //get the TYPE field
            String type = "";
            if (typeLength > 0) {
                type = ByteArray.byteArrayToString (dimeMsg, index, typeLength);
            }
            index += (typeLength % 4 == 0) ? typeLength : (typeLength + 4 - (typeLength % 4));

            //get the PAYLOAD
            byte[] payload = new byte[dataLength];
            System.arraycopy (dimeMsg, index, payload, 0, dataLength);

            index += (dataLength % 4 == 0) ? dataLength : (dataLength + 4 - (dataLength % 4));

            DimeRecord dr = new DimeRecord (type, payload, id);
            dr.setMessageBeginFlag (messageBeginFlag);
            dr.setMessageEndFlag (messageEndFlag);
            dr.setChunkFlag (chunkFlag);
            _records.add (dr);

            if (dr.getMessageEndFlag()) {
                break;
            }
        }
    }

    /**
     * Parses the dime contained in the byte array
     * @param is input stream where the dime is read from
     * @throws IOException if problems occur while reading from the input stream
     */
    private void parseDime (InputStream is) throws IOException
    {
        byte[] buffAux;

        while (true) {
            //read the header.
            byte[] header = readBytes (is, 8);

            //get the MB, ME, CF flags
            boolean messageBeginFlag = (header[0] & 0x80) == 0x80;
            boolean messageEndFlag = (header[0] & 0x40) == 0x40;
            boolean chunkFlag = (header[0] & 0x20) == 0x20;

            //get the ID_LENGTH field
            short idLength = ByteArray.byteArrayToShort (header, 0);
            idLength &= 0x1FFF;

            //get the TNF field

            //get the TYPE_LENGTH field
            short typeLength = ByteArray.byteArrayToShort (header, 2);
            typeLength &= 0x1FFF;

            //get the DATA_LENGTH field
            int dataLength = ByteArray.byteArrayToInt (header, 4);

            //get the ID field
            String id = "";
            if (idLength > 0) {
                buffAux = readBytes (is, idLength);
                id = ByteArray.byteArrayToString (buffAux, 0, idLength);
            }
            int padding = (4 - (idLength % 4)) % 4;
            skipBytes(is, padding);

            //get the TYPE field
            String type = "";
            if (typeLength > 0) {
                buffAux = readBytes (is, typeLength);
                type = ByteArray.byteArrayToString (buffAux, 0, typeLength);
            }
            padding = (4 - (typeLength % 4)) % 4;
            skipBytes (is, padding);

            //get the PAYLOAD
            byte[] payload = readBytes (is, dataLength);

            padding = (4 - (dataLength % 4)) % 4;
            skipBytes (is, padding);

            DimeRecord dr = new DimeRecord (type, payload, id);
            dr.setMessageBeginFlag (messageBeginFlag);
            dr.setMessageEndFlag (messageEndFlag);
            dr.setChunkFlag (chunkFlag);
            _records.add (dr);

            if (dr.getMessageEndFlag()) {
                break;
            }
        }
    }

    /**
     * Reads <code>nBytes</code> from the input stream and writes them inside a byte array
     * @param is input stream where to read from
     * @param nBytes number of bytes to read
     * @return the byte array containing the byte read
     * @throws IOException if problems occur in reading from the input stream
     */
    private byte[] readBytes (InputStream is, int nBytes) throws IOException
    {
        byte[] res = new byte[nBytes];
        int index = 0;
        int read;

        while (index < nBytes) {
            read = is.read (res, index, nBytes - index);
            if (read < 0) {
                throw new IOException ("Error reading Dime from InputStream");
            }
            index += read;
        }

        return res;
    }

    /**
     * Skips <code>nBytes</code> from the input stream
     * @param is input stream where to skip the bytes
     * @param nBytes number of bytes to skip
     */
    private void skipBytes (InputStream is, int nBytes)
    {
        if (nBytes <= 0) {
            return;
        }

        try {
            is.skip (nBytes);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    private final List<DimeRecord> _records;
    private int _recordsOffset = 0;



    public static void main(String[] args)
    {
        Dime dime = new Dime();
        byte[] payload = "this is the payload".getBytes();

        dime.addRecord ("payloadType", payload, "-payloadID-)");

        payload = "---2this is the payload2---".getBytes();
        dime.addRecord ("payloadType2", payload, "-payloadID2-)");

//        System.out.println("Dime Length is : " + dime.getDimeLength());
        byte[] buff = dime.getDime();
 //       ByteArray.printByteArrayAsHex(buff);

        ByteArrayInputStream bais = new ByteArrayInputStream(buff);

//        dime = new Dime(buff);
        try {
            dime = new Dime (bais);
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        dime.print();
    }
}
