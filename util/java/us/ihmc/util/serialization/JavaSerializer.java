/*
 * Serializer.java
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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

/**
 * Implementation of <code>Serializer</code> that exploits the java serialization
 * @author Rita Lenzi (rlenzi@ihmc.us)
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class JavaSerializer implements Serializer
{
    @Override
    public void register (Class c)
    {
    }

    @Override
    public byte[] serialize (Object obj) throws SerializationException
    {
        try {
            ByteArrayOutputStream b = new ByteArrayOutputStream();
            ObjectOutputStream o = new ObjectOutputStream(b);
            o.writeObject(obj);
            byte[] objAsBytes = b.toByteArray();
            o.close();
            o = null;
            b.close();
            b = null;
            return objAsBytes;
        }
        catch (IOException e) {
            throw new SerializationException ("Problem in the serialization process " + e.getMessage());
        }
    }

    @Override
    public <T> T deserialize (byte[] bytes, Class<T> objClass) throws SerializationException
    {
        try {
            ByteArrayInputStream b = new ByteArrayInputStream(bytes);
            ObjectInputStream o = new ObjectInputStream(b);
            Object desObject = o.readObject();
            o.close();
            o = null;
            b.close();
            b = null;

            return objClass.cast (desObject);
        }
        catch (ClassCastException e) {
            throw new SerializationException ("Problem in the deserialization process " + e.getMessage());
        }
        catch (ClassNotFoundException e) {
            throw new SerializationException ("Problem in the deserialization process " + e.getMessage());
        }
        catch (IOException e) {
            throw new SerializationException ("Problem in the deserialization process " + e.getMessage());
        }
    }
}
