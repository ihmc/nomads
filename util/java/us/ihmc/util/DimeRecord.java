/*
 * DimeRecord.java
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
 * DimeRecord
 *
 * @author      Marco Arguedas    <marguedas@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.util;

import java.io.IOException;
import java.io.OutputStream;

/**
 * Manages a dime record
 */
public class DimeRecord
{
    /**
     * Constructor
     * @param type payload type
     * @param payload byte array representing the dime record payload
     * @param id payload identifier
     */
    public DimeRecord (String type, byte[] payload, String id)
    {
        _payload = payload;
        _type = type;
        _id = id;
    }

    /**
     * Constructor
     * @param type payload type
     * @param payload byte array representing the dime record payload
     * @param id payload identifier
     */
    public DimeRecord (PayloadType type, byte[] payload, String id)
    {
        _payload = payload;
        _type = type.name();
        _id = id;
    }

    /**
     * Sets the message begin flag value
     * @param b message begin flag value
     */
    void setMessageBeginFlag (boolean b)
    {
        _messageBeginFlag = b;
    }

    /**
     * Sets the message end flag value
     * @param b message end flag value
     */
    void setMessageEndFlag (boolean b)
    {
        _messageEndFlag = b;
    }

    /**
     * Sets the chunk flag value
     * @param b chunk flag value
     */
    void setChunkFlag (boolean b)
    {
        _chunkFlag = b;
    }

    /**
     * Gets the dime record payload
     * @return the dime record payload
     */
    public byte[] getPayload()
    {
        return _payload;
    }

    /**
     * Gets the dime record payload type
     * @return the dime record payload type
     */
    public String getType()
    {
        return _type;
    }

    /**
     * Gets the dime record payload identifier
     * @return the dime record payload identifier
     */
    public String getID()
    {
        return _id;
    }

    /**
     * Gets the length of the dime record payload
     * @return the length of the dime record payload
     */
    public int getPayloadLength()
    {
        return (_payload != null) ? _payload.length : 0;
    }

    /**
     * Gets the length of the whole dime record object
     * @return the length of the whole dime record object
     */
    public int getRecordLength()
    {
        int recLength = 8; //the header size
        int aux;

        // add the length of the 'ID' field (with padding)
        if (_id != null) {
            aux = _id.length();
            recLength += (aux % 4 == 0) ? aux
                                        : ( aux + 4 - (aux % 4) ); //align to 32 bits.
        }

        //add the lenght of the 'type' field (with padding)
        if (_type != null) {
            aux = _type.length();
            recLength += (aux % 4 == 0) ? aux
                                        : ( aux + 4 - (aux % 4) ); //align to 32 bits.
        }

        // now, add the size of the payload (with padding)
        aux = this.getPayloadLength();
        recLength += (aux % 4 == 0) ? aux
                                    : ( aux + 4 - (aux % 4) ); //align to 32 bits.

        return recLength;
    }

    /**
     * Gets the whole dime record object as byte array
     * @return a byte array containing the whole dime record object
     */
    public byte[] getRecord()
    {
        int length = this.getRecordLength();
        byte[] buff = new byte[length];
        getRecord (buff, 0);

        return buff;
    }

    /**
     * Gets the dime record object as byte array starting from a given offset
     * @param buff byte array where the dime object will be written into
     * @param offset offset value
     */
    public void getRecord (byte[] buff, int offset)
    {
        int index = offset;
        int lim = index + getRecordLength();
        byte[] aux;

        for (int i = index; i < lim; i++)  {
            buff[i] = 0;
        }

        // set the ID length field.
        if (_id != null) {
            aux = ByteArray.shortToByteArray ((short)_id.length());
            System.arraycopy (aux, 0, buff, index + 0, 2);
        }

        // now set the flags
        buff[index + 0] &= 0x1F;
        if (_messageBeginFlag) {
            buff[index + 0] |= 0x80;
        }
        if (_messageEndFlag) {
            buff[index + 0] |= 0x40;
        }
        if (_chunkFlag) {
            buff[index + 0] |= 0x20;
        }

        // set the Type length field.
        if (_type != null) {
            aux = ByteArray.shortToByteArray ((short)_type.length());
            System.arraycopy(aux, 0, buff, index + 2, 2);
        }

        buff[index + 2] &= 0x1F;
        // set the TNF value here.

        // set the Payload length
        if (_payload != null) {
            aux = ByteArray.intToByteArray (_payload.length);
            System.arraycopy(aux, 0, buff, index + 4, 4);
        }

        // ---
        index += 8;
        // ---

        //pRecord[8]: ID + Padding
        if (_id != null) {
            ByteArray.stringToByteArray (_id, buff, index, _id.length());
            index += (_id.length() % 4 == 0 ? _id.length()
                                            : _id.length() + 4 - (_id.length() % 4) );
        }

        //pRecord[index]: Type + Padding
        if (_type != null) {
            ByteArray.stringToByteArray (_type, buff, index, _type.length());
            index += (_type.length() % 4 == 0 ? _type.length()
                                              : _type.length() + 4 - (_type.length() % 4) );
        }
        else {
            System.out.println ("WARNING:: TYPE field seems to be NULL.");
        }

        //copy the data (payload)
        if (_payload != null) {
            System.arraycopy (_payload, 0, buff, index, _payload.length);
        }
    }

    /**
     * Writes the whole dime record object in the output stream
     * @param os output stream where the dime record object will be written into
     * @throws IOException if problems occur in writing the dime record object
     */
    public void getRecord (OutputStream os) throws IOException
    {
        byte[] header = new byte[8];
        byte[] paddingAux = new byte[] {0, 0, 0};
        byte[] aux;
        int padding;

        // set the ID length field.
        if (_id != null) {
            aux = ByteArray.shortToByteArray ((short)_id.length());
            System.arraycopy(aux, 0, header, 0, 2);
        }

        // now set the flags
        header[0] &= 0x1F;
        if (_messageBeginFlag) {
            header[0] |= 0x80;
        }
        if (_messageEndFlag) {
            header[0] |= 0x40;
        }
        if (_chunkFlag) {
            header[0] |= 0x20;
        }

        // set the Type length field.
        if (_type != null) {
            aux = ByteArray.shortToByteArray ((short)_type.length());
            System.arraycopy(aux, 0, header, 2, 2);
        }

        header[2] &= 0x1F;
        // set the TNF value here.

        // set the Payload length
        if (_payload != null) {
            aux = ByteArray.intToByteArray (_payload.length);
            System.arraycopy(aux, 0, header, 4, 4);
        }

        // write the header.
        os.write (header);

        // write ID + Padding
        if (_id != null) {
            aux = new byte[_id.length()];
            ByteArray.stringToByteArray (_id, aux, 0, _id.length());
            os.write (aux);
            if ( (padding = (4 - (_id.length() % 4)) % 4) != 0 ) {
                os.write (paddingAux, 0, padding);
                os.flush();
            }
        }

        // write the Type + Padding
        if (_type != null) {
            aux = new byte[_type.length()];
            ByteArray.stringToByteArray (_type, aux, 0, _type.length());
            os.write (aux);
            if ( (padding = (4 - (_type.length() % 4)) % 4) != 0 ) {
                os.write (paddingAux, 0, padding);
                os.flush();
            }
        }
        else {
            System.out.println ("WARNING:: TYPE field seems to be NULL.");
        }

        // Write the Payload + Padding
        if (_payload != null) {
            os.write (_payload);
            if ( (padding = (4 - (_payload.length % 4)) % 4) != 0 ) {
                os.write (paddingAux, 0, padding);
            }
        }

        os.flush();
    }

    /**
     * Prints the dime record object on standard output
     */
    public void print()
    {
        System.out.println ("::: Dime Record :::");
        System.out.println ("\tMB = " + _messageBeginFlag);
        System.out.println ("\tME = " + _messageEndFlag);
        System.out.println ("\tCF = " + _chunkFlag);
        System.out.println ("\tID = " + _id);
        System.out.println ("\tType = " + _type);
        System.out.println ("\tPayloadLength = " + getPayloadLength());
        if (_payload != null) {
            String str = new String (_payload, 0, Math.min(70, getPayloadLength()));
            System.out.println ("\tPayload = [" + str + "]");
        }
    }

    /**
     * Gets the value of the message begin flag
     * @return the value of the message begin flag
     */
    boolean getMessageBeginFlag()
    {
        return _messageBeginFlag;
    }

    /**
     * Gets the value of the message end flag
     * @return the value of the message end flag
     */
    boolean getMessageEndFlag()
    {
        return _messageEndFlag;
    }

    /**
     * Gets the value of the chunk flag
     * @return the value of the chunk flag
     */
    boolean getChunkFlag()
    {
        return _chunkFlag;
    }



    private boolean _messageBeginFlag = false;
    private boolean _messageEndFlag = false;
    private boolean _chunkFlag = false;

    private final byte[] _payload;
    private final String _type;
    private final String _id;

    public enum PayloadType
    {
        response,
        java_lang_object,
        service_state,
        acr_file,
        binary_invoke_request
    }
}
