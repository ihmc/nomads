/*
 * KryoSerializer.java
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

import com.esotericsoftware.kryo.Kryo;
import com.esotericsoftware.kryo.io.Input;
import com.esotericsoftware.kryo.io.Output;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Implementation of <code>Serializer</code> that exploits the Kryo library
 * @author Rita Lenzi (rlenzi@ihmc.us)
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class KryoSerializer implements Serializer
{
    public KryoSerializer()
    {
        _kryo = new Kryo();
        _registrationList = new ArrayList<Class>();
    }

    @Override
    public void register (Class c)
    {
        if (c == null) {
            return;
        }

        if (_registrationList.contains (c)) {
            return;
        }
        _registrationList.add (c);
        _kryo.register(c);
    }

    @Override
    public byte[] serialize (Object obj) throws SerializationException
    {
        if (obj == null) {
            throw new SerializationException ("The argument is null");
        }

        Output output = new Output (new ByteArrayOutputStream());
        _kryo.writeObject (output, obj);
        byte[] buffer = output.toBytes();
        output.close();

        return buffer;
    }

    @Override
    public <T> T deserialize (byte[] buffer, Class<T> objClass) throws SerializationException
    {
        if (buffer == null) {
            throw new SerializationException ("The byte[] is null");
        }
        if (objClass == null) {
            throw new SerializationException ("The class argument is null");
        }

        Input input = new Input (new ByteArrayInputStream (buffer));
        Object obj = _kryo.readObject (input, objClass);
        input.close();

        try {
            return objClass.cast (obj);
        }
        catch (ClassCastException e) {
            throw new SerializationException ("Problem in the deserialization process " + e.getMessage());
        }
    }

    private final Kryo _kryo;
    private final List<Class> _registrationList;
}
