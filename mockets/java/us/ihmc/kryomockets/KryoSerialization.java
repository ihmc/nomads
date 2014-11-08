/*
 * KryoSerialization.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets;

import java.nio.ByteBuffer;

import com.esotericsoftware.kryo.Kryo;
import com.esotericsoftware.kryo.io.ByteBufferInputStream;
import com.esotericsoftware.kryo.io.ByteBufferOutputStream;
import com.esotericsoftware.kryo.io.Input;
import com.esotericsoftware.kryo.io.Output;
import us.ihmc.kryomockets.FrameworkMessage.DiscoverHost;
import us.ihmc.kryomockets.FrameworkMessage.KeepAlive;
import us.ihmc.kryomockets.FrameworkMessage.RegisterMockets;
import us.ihmc.kryomockets.FrameworkMessage.Ping;

public class KryoSerialization implements Serialization {
	private final Kryo kryo;
	private final Input input;
	private final Output output;
	private final ByteBufferInputStream byteBufferInputStream = new ByteBufferInputStream();
	private final ByteBufferOutputStream byteBufferOutputStream = new ByteBufferOutputStream();

	public KryoSerialization () {
		this(new Kryo());
		kryo.setReferences(false);
		kryo.setRegistrationRequired(true);
	}

	public KryoSerialization (Kryo kryo) {
		this.kryo = kryo;

		kryo.register(RegisterMockets.class);
		kryo.register(KeepAlive.class);
		kryo.register(DiscoverHost.class);
		kryo.register(Ping.class);

		input = new Input(byteBufferInputStream, 512);
		output = new Output(byteBufferOutputStream, 512);
	}

	public Kryo getKryo () {
		return kryo;
	}

	public synchronized void write (Connection connection, ByteBuffer buffer, Object object) {
		byteBufferOutputStream.setByteBuffer(buffer);
		kryo.getContext().put("connection", connection);
		kryo.writeClassAndObject(output, object);
		output.flush();
	}

	public synchronized Object read (Connection connection, ByteBuffer buffer) {
		byteBufferInputStream.setByteBuffer(buffer);
		kryo.getContext().put("connection", connection);
		return kryo.readClassAndObject(input);
	}

	public void writeLength (ByteBuffer buffer, int length) {
		buffer.putInt(length);
	}

	public int readLength (ByteBuffer buffer) {
		return buffer.getInt();
	}

	public int getLengthLength () {
		return 4;
	}
}
