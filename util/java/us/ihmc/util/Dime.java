/*
 * Dime.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 * @version     $Revision: 1.11 $
 *              $Date: 2014/11/07 17:58:06 $
 */

package us.ihmc.util;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Enumeration;
import java.util.Vector;

/**
 *
 */
@SuppressWarnings({"rawtypes", "unchecked"})
public class Dime
{
    public Dime()
    {
        _records = new Vector();
    }

    public Dime (byte[] dimeMsg)
    {
        this();
        this.parseDime (dimeMsg);
    }

    public Dime (InputStream is) throws IOException
    {
        this();
        this.parseDime (is);
    }

    /**
     *
     */
    public boolean addRecord (String payloadType,
                              byte[] payload,
                              String payloadIdentifier)
    {
        DimeRecord dr = new DimeRecord();
        dr.setPayload (payload);
        dr.setID (payloadIdentifier);
        dr.setType (payloadType);

        _records.add (dr);

        return true;
    }

    /**
     *
     */
    public boolean getDime (byte[] buff)
    {
        if (_records.isEmpty()) {
            return false;
        }

        DimeRecord dr;
        int index = 0;

        //reset the MB and ME flags in all the records.
        Enumeration en = _records.elements();
        while (en.hasMoreElements()) {
            dr = (DimeRecord) en.nextElement();
            dr.setMessageBeginFlag (false);
            dr.setMessageEndFlag (false);
        }

        // ---
        dr = (DimeRecord) _records.firstElement();
        dr.setMessageBeginFlag (true);

        dr = (DimeRecord) _records.lastElement();
        dr.setMessageEndFlag (true);

        en = _records.elements();
        while (en.hasMoreElements()) {
            dr = (DimeRecord) en.nextElement();
            dr.getRecord (buff, index);
            index += dr.getRecordLength();
        }

        return true;
    }

    /**
     *
     */
    public byte[] getDime()
    {
        if (_records.isEmpty()) {
            return null;
        }

        byte[] buff = new byte[getDimeLength()];
        this.getDime(buff);
        return buff;
    }

    /**
     *
     */
    public void getDime (OutputStream os)
        throws IOException
    {
        if (_records.isEmpty()) {
            return;
        }

        DimeRecord dr;

        //reset the MB and ME flags in all the records.
        Enumeration en = _records.elements();
        while (en.hasMoreElements()) {
            dr = (DimeRecord) en.nextElement();
            dr.setMessageBeginFlag (false);
            dr.setMessageEndFlag (false);
        }

        // ---
        dr = (DimeRecord) _records.firstElement();
        dr.setMessageBeginFlag (true);

        dr = (DimeRecord) _records.lastElement();
        dr.setMessageEndFlag (true);

        en = _records.elements();
        while (en.hasMoreElements()) {
            dr = (DimeRecord) en.nextElement();
            dr.getRecord (os);
        }
    }

    /**
     *
     */
    public int getDimeLength()
    {
        int length = 0;
        Enumeration en = _records.elements();
        while (en.hasMoreElements()) {
            DimeRecord dr = (DimeRecord) en.nextElement();
            length += dr.getRecordLength();
        }
        return length;
    }

    /**
     * @return  an Enumeration of DimeRecord objects.
     */
    public Enumeration getRecords()
    {
        return _records.elements();
    }

    /**
     *
     */
    public int getRecordCount()
    {
        return _records.size();
    }

    /**
     *
     */
    public void print()
    {
        System.out.println("::: Start of DIME Message :::");
        System.out.println("::: TotalDime size is " + getDimeLength() + " bytes :::");

        Enumeration en = _records.elements();
        while (en.hasMoreElements()) {
            DimeRecord dr = (DimeRecord) en.nextElement();
            System.out.println("..............................................");
            dr.print();;
        }

        System.out.println("::: End of DIME Message :::");
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    private void parseDime (byte[] dimeMsg)
    {
        boolean flagAux;
        int index = 0;
        short typeLength, idLength;
        int dataLength;
        String strAux;


        while (true) {
            DimeRecord dr = new DimeRecord();

            //get the MB, ME, CF flags
            flagAux = (dimeMsg[index + 0] & 0x80) == 0x80;;
            dr.setMessageBeginFlag (flagAux);

            flagAux = (dimeMsg[index + 0] & 0x40) == 0x40;;
            dr.setMessageEndFlag (flagAux);

            flagAux = (dimeMsg[index + 0] & 0x20) == 0x20;;
            dr.setChunkFlag (flagAux);

            //get the ID_LENGTH field
            idLength = ByteArray.byteArrayToShort (dimeMsg, index + 0);
            idLength &= 0x1FFF;

            //get the TNF field

            //get the TYPE_LENGTH field
            typeLength = ByteArray.byteArrayToShort (dimeMsg, index + 2);
            typeLength &= 0x1FFF;

            //get the DATA_LENGTH field
            dataLength = ByteArray.byteArrayToInt (dimeMsg, index + 4);

            // ---
            index += 8;
            // ---

            //get the ID field
            if (idLength > 0) {
                strAux = ByteArray.byteArrayToString (dimeMsg, index, idLength);
                dr.setID (strAux);
            }
            index += (idLength % 4 == 0) ? idLength : (idLength + 4 - (idLength % 4));

            //get the TYPE field
            if (typeLength > 0) {
                strAux = ByteArray.byteArrayToString (dimeMsg, index, typeLength);
                dr.setType (strAux);
            }
            index += (typeLength % 4 == 0) ? typeLength : (typeLength + 4 - (typeLength % 4));

            //get the PAYLOAD
            byte[] payload = new byte[dataLength];
            System.arraycopy (dimeMsg, index, payload, 0, dataLength);
            dr.setPayload (payload);

            index += (dataLength % 4 == 0) ? dataLength : (dataLength + 4 - (dataLength % 4));

            _records.add (dr);

            if (dr.getMessageEndFlag()) {
                break;
            }
        }
    }

    /**
     *
     */
    private void parseDime(InputStream is) throws IOException
    {
        boolean flagAux;
        short typeLength, idLength;
        int dataLength;
        String strAux;
        byte[] buffAux;
        int padding;


        while (true) {
            DimeRecord dr = new DimeRecord();

            //read the header.
            byte[] header = readBytes(is, 8);

            //get the MB, ME, CF flags
            flagAux = (header[0] & 0x80) == 0x80;
            dr.setMessageBeginFlag (flagAux);

            flagAux = (header[0] & 0x40) == 0x40;
            dr.setMessageEndFlag (flagAux);

            flagAux = (header[0] & 0x20) == 0x20;
            dr.setChunkFlag (flagAux);

            //get the ID_LENGTH field
            idLength = ByteArray.byteArrayToShort (header, 0);
            idLength &= 0x1FFF;

            //get the TNF field

            //get the TYPE_LENGTH field
            typeLength = ByteArray.byteArrayToShort (header, 2);
            typeLength &= 0x1FFF;

            //get the DATA_LENGTH field
            dataLength = ByteArray.byteArrayToInt (header, 4);

            //get the ID field
            if (idLength > 0) {
                buffAux = readBytes(is, idLength);
                strAux = ByteArray.byteArrayToString (buffAux, 0, idLength);
                dr.setID (strAux);
            }
            padding = (4 - (idLength % 4)) % 4;
            skipBytes(is, padding);

            //get the TYPE field
            if (typeLength > 0) {
                buffAux = readBytes(is, typeLength);
                strAux = ByteArray.byteArrayToString (buffAux, 0, typeLength);
                dr.setType (strAux);
            }
            padding = (4 - (typeLength % 4)) % 4;
            skipBytes(is, padding);

            //get the PAYLOAD
            buffAux = readBytes(is, dataLength);
            dr.setPayload (buffAux);

            padding = (4 - (dataLength % 4)) % 4;
            skipBytes(is, padding);

            _records.add (dr);

            if (dr.getMessageEndFlag()) {
                break;
            }
        }
    }

    /**
     *
     */
    private byte[] readBytes(InputStream is, int nBytes) throws IOException
    {
        byte[] res = new byte[nBytes];
        int index = 0;
        int read;

        while (index < nBytes) {
            try {
                read = is.read(res, index, nBytes - index);
                if (read < 0) {
                    throw new IOException("Error reading Dime from InputStream");
                }
                index += read;
            }
            catch (IOException ex) {
                throw ex;
            }
        }

        return res;
    }

    /**
     *
     */
    private void skipBytes(InputStream is, int nBytes)
    {
        if (nBytes <= 0) {
            return;
        }

        try {
            is.skip(nBytes);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    private Vector _records;

    // /////////////////////////////////////////////////////////////////////////
    // MAIN METHOD - for testing purposes //////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    public static void main(String[] args)
    {
        Dime dime = new Dime();
        byte[] payload = new String("this is the payload").getBytes();

        dime.addRecord("payloadType", payload, "-payloadID-)");

        payload = new String("---2this is the payload2---").getBytes();
        dime.addRecord("payloadType2", payload, "-payloadID2-)");

//        System.out.println("Dime Length is : " + dime.getDimeLength());
        byte[] buff = dime.getDime();
 //       ByteArray.printByteArrayAsHex(buff);

        ByteArrayInputStream bais = new ByteArrayInputStream(buff);

//        dime = new Dime(buff);
        try {
            dime = new Dime(bais);
        } catch (IOException e) {
            e.printStackTrace();
        }
        dime.print();
    }
}
