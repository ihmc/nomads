/*
 * DimeRecord.java
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
 * DimeRecord
 *
 * @author      Marco Arguedas    <marguedas@ihmc.us>
 *
 * @version     $Revision: 1.7 $
 *              $Date: 2014/11/07 17:58:06 $
 */

package us.ihmc.util;

import java.io.IOException;
import java.io.OutputStream;

/**
 *
 */
public class DimeRecord
{
    DimeRecord()
    {
    }

    /**
     *
     */
    public void setPayload (byte[] payload)
    {
        _payload = payload;
    }

    /**
     *
     */
    public void setType (String type)
    {
        _type = type;
    }

    /**
     *
     */
    public void setID (String id)
    {
        _id = id;
    }

    /**
     *
     */
    public byte[] getPayload()
    {
        return _payload;
    }

    /**
     *
     */
    public String getType()
    {
        return _type;
    }

    /**
     *
     */
    public String getID()
    {
        return _id;
    }

    /**
     *
     */
    public int getPayloadLenght()
    {
        int length = (_payload != null) ? _payload.length : 0;
        return length;
    }

    /**
     *
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
        aux = this.getPayloadLenght();
        recLength += (aux % 4 == 0) ? aux
                                    : ( aux + 4 - (aux % 4) ); //align to 32 bits.

        return recLength;
    }

    /**
     *
     */
    public byte[] getRecord()
    {
        int length = this.getRecordLength();
        byte[] buff = new byte[length];
        this.getRecord(buff, 0);

        return buff;
    }

    /**
     *
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
            aux = ByteArray.shortToByteArray((short)_id.length());
            System.arraycopy(aux, 0, buff, index + 0, 2);
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
    *
     */
    public void getRecord (OutputStream os)
        throws IOException
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
     *
     */
    public void print()
    {
        System.out.println ("::: Dime Record :::");
        System.out.println ("\tMB = " + _messageBeginFlag);
        System.out.println ("\tME = " + _messageEndFlag);
        System.out.println ("\tCF = " + _chunkFlag);
        System.out.println ("\tID = " + _id);
        System.out.println ("\tType = " + _type);
        System.out.println ("\tPayloadLength = " + getPayloadLenght());
        if (_payload != null) {
            String str = new String(_payload, 0, Math.min(70, getPayloadLenght()));
            System.out.println ("\tPayload = [" + str + "]");
        }
    }

    /**
     *
     */
    void setMessageBeginFlag (boolean value)
    {
        _messageBeginFlag = value;
    }

    /**
     *
     */
    void setMessageEndFlag (boolean value)
    {
        _messageEndFlag = value;
    }

    /**
     *
     */
    void setChunkFlag (boolean value)
    {
        _chunkFlag = value;
    }

    /**
     *
     */
    boolean getMessageBeginFlag()
    {
        return _messageBeginFlag;
    }

    /**
     *
     */
    boolean getMessageEndFlag()
    {
        return _messageEndFlag;
    }

    /**
     *
     */
    boolean getChunkFlag()
    {
        return _chunkFlag;
    }

    // /////////////////////////////////////////////////////////////////////////
    private boolean _messageBeginFlag   = false;
    private boolean _messageEndFlag     = false;
    private boolean _chunkFlag          = false;

    private byte[] _payload = new byte[0];
    private String _type    = "";
    private String _id      = "";
}
