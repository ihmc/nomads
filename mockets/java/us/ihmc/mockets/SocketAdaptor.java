/*
 * SocketAdaptor.java
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
 * Class <code>SocketAdaptor</code> provides an adaptor for easy replacement of the java.net.Socket class with the
 * Mockets framework.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */

package us.ihmc.mockets;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

public class SocketAdaptor
{
    public enum Type
    {
        TCP,
        Mockets
    }

    private SocketAdaptor (Type type, String host, int port) throws IOException
    {
        _type = type;
        switch (_type) {
            case TCP:
                _socket = new Socket(host, port);
                break;
            case Mockets:
                _streamMocket = new StreamMocket(host, port);
                break;
        }
    }

    private SocketAdaptor (Type type) throws IOException
    {
        _type = type;
        switch (_type) {
            case TCP:
                _socket = new Socket();
                break;
            case Mockets:
                _streamMocket = new StreamMocket();
                break;
        }
    }

    private SocketAdaptor (Socket socket)
    {
        _type = Type.TCP;
        _socket = socket;
    }

    private SocketAdaptor (StreamMocket streamMocket)
    {
        _type = Type.Mockets;
        _streamMocket = streamMocket;
    }

    public static SocketAdaptor newInstance (Type type) throws IOException
    {
        return new SocketAdaptor(type);
    }

    public static SocketAdaptor newInstance (Type type, String host, int port) throws IOException
    {
        return new SocketAdaptor(type, host, port);
    }

    public static SocketAdaptor newInstance (Socket socket)
    {
        return new SocketAdaptor(socket);
    }

    public static SocketAdaptor newInstance (StreamMocket streamMocket)
    {
        return new SocketAdaptor(streamMocket);
    }

    /**
     * Returns whether this <code>SocketAdaptor</code> is connected to a remote host.
     *
     * @return {@code true} if the <code>SocketAdaptor</code> is connected, {@code false} otherwise.
     */
    public boolean isConnected ()
    {
        switch (_type) {
            case TCP:
                return _socket.isConnected();
            case Mockets:
                return _streamMocket.isConnected();
            default:
                throw new UnsupportedOperationException("Unable to execute isConnected() on this type");
        }
    }

    /**
     * Returns whether this <code>SocketAdaptor</code> is bound to local host.
     *
     * @return {@code true} if the <code>SocketAdaptor</code> is bound, {@code false} otherwise.
     */
    public boolean isBound ()
    {
        switch (_type) {
            case TCP:
                return _socket.isBound();
            case Mockets:
                return _streamMocket.isBound();
            default:
                throw new UnsupportedOperationException("Unable to execute isConnected() on this type");
        }
    }

    /**
     * Returns whether this <code>SocketAdaptor</code> is closed.
     *
     * @return {@code true} if the <code>SocketAdaptor</code> is closed, {@code false} otherwise.
     */
    public boolean isClosed ()
    {
        switch (_type) {
            case TCP:
                return _socket.isClosed();
            case Mockets:
                return _streamMocket.isClosed();
            default:
                throw new UnsupportedOperationException("Unable to execute isClosed() on this type");
        }
    }

    /**
     * Closes this <code>SocketAdaptor</code>.
     * <p/>
     * <p/>
     * If this is a TCP <code>SocketAdaptor</code>, @see {@link java.net.Socket#close close}
     */
    public void close () throws IOException
    {
        switch (_type) {
            case TCP:
                _socket.close();
            case Mockets:
                _streamMocket.closeSync();
        }
    }

    /**
     * Returns an input stream for this <code>SocketAdaptor</code>.
     * <p/>
     * If this is a TCP <code>SocketAdaptor</code>, @see {@link java.net.Socket#getInputStream getInputStream}
     *
     * @return an input stream for reading bytes from this <code>SocketAdaptor</code>.
     * @throws IOException if an I/O error occurs when creating the
     *                     input stream, the <code>SocketAdaptor</code> is closed, the <code>SocketAdaptor</code> is
     *                     not connected.
     */
    public InputStream getInputStream () throws IOException
    {
        switch (_type) {
            case TCP:
                return _socket.getInputStream();
            case Mockets:
                return _streamMocket.getInputStream();
            default:
                throw new UnsupportedOperationException("Unable to execute getInputStream() on this type");
        }
    }

    /**
     * Returns an output stream for this <code>SocketAdaptor</code>.
     * <p/>
     * If this is a TCP <code>SocketAdaptor</code>, @see {@link java.net.Socket#getOutputStream getOutputStream}
     *
     * @return an output stream for writing bytes to this <code>SocketAdaptor</code>.
     * @throws IOException if an I/O error occurs when creating the
     *                     output stream or if the <code>SocketAdaptor</code> is not connected.
     */
    public OutputStream getOutputStream () throws IOException
    {
        switch (_type) {
            case TCP:
                return _socket.getOutputStream();
            case Mockets:
                return _streamMocket.getOutputStream();
            default:
                throw new UnsupportedOperationException("Unable to execute getOutputStream() on this type");
        }
    }

    private final Type _type;
    private Socket _socket;
    private StreamMocket _streamMocket;
}
