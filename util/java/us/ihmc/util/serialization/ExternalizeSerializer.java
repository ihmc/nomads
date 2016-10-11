/*
 * ExternalizeSerializer.java
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

package us.ihmc.util.serialization;


import java.io.*;

/**
 * Implementation of <code>Serializer</code> that exploits the java <code>Externalizable</code>
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class ExternalizeSerializer implements Serializer
{
    @Override
    public void register (Class c)
    {
    }

    @Override
    public byte[] serialize (Object obj) throws SerializationException
    {
        try {
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            ObjectOutputStream oos = new ObjectOutputStream (bos);
            oos.writeObject (new PersonalizedExternalizable (obj));
            byte[] buffer = bos.toByteArray();
            bos.close();
            oos.close();
            return buffer;
        }
        catch (IOException e) {
            throw new SerializationException (e);
        }
    }

    @Override
    public <T> T deserialize (byte[] buffer, Class<T> objClass) throws SerializationException
    {
        try {
            ByteArrayInputStream bis = new ByteArrayInputStream (buffer);
            ObjectInputStream ois = new ObjectInputStream (bis);
            PersonalizedExternalizable pe = (PersonalizedExternalizable) ois.readObject();
            Object obj = pe.getDeserializedObject();
            if (obj == null) {
                throw new IOException ("The deserialized object is null");
            }
            return objClass.cast (obj);
        }
        catch (ClassCastException e) {
            throw new SerializationException (e);
        }
        catch (ClassNotFoundException e) {
            throw new SerializationException (e);
        }
        catch (IOException e) {
            throw new SerializationException (e);
        }
    }
}


