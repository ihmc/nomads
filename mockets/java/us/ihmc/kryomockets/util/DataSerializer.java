/*
 * DataSerializer.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets.util;

import com.esotericsoftware.kryo.Kryo;
import com.esotericsoftware.kryo.io.Input;
import com.esotericsoftware.kryo.io.Output;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;

/**
 * DataSerializer.java
 * <p/>
 * Class <code>DataSerializer</code> handles serialization for data defined in <code>Data</code> and
 * <code>DataType</code> using <code>Kryo</code>.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DataSerializer
{
    public DataSerializer ()
    {
        _kryo = new Kryo();
        _kryo.register(Data.class);
    }

    public byte[] serialize (Data data)
    {
        Output output = new Output(new ByteArrayOutputStream());
        _kryo.writeObject(output, data);
        byte[] buffer = output.toBytes();
        output.close();

        return buffer;
    }

    public Data deserialize (byte[] buffer)
    {
        Input input = new Input(new ByteArrayInputStream(buffer));
        Data data = _kryo.readObject(input, Data.class);
        input.close();

        return data;
    }

    private final Kryo _kryo;
}
