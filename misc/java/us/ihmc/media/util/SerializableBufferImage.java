package us.ihmc.media.util;

import us.ihmc.io.Streamable;

import java.awt.Dimension;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.Serializable;

import javax.media.Buffer;
import javax.media.format.RGBFormat;

public class SerializableBufferImage implements Serializable, Streamable
{
    public SerializableBufferImage()
    {
    }
    
    public SerializableBufferImage (Buffer buf)
    {
        RGBFormat rgbFormat = (RGBFormat)buf.getFormat();
        if (rgbFormat.getDataType() == rgbFormat.intArray) {
            _intData = (int[])buf.getData();
            _byteData = null;
        }
        else if (rgbFormat.getDataType() == rgbFormat.byteArray) {
            _byteData = (byte[])buf.getData();
            _intData = null;
        }
        
        _width = rgbFormat.getSize().width;
        _height = rgbFormat.getSize().height;
        _bitsPerPixel = rgbFormat.getBitsPerPixel();
        _redMask = rgbFormat.getRedMask();
        _greenMask = rgbFormat.getGreenMask();
        _blueMask = rgbFormat.getBlueMask();
        _pixelStride = rgbFormat.getPixelStride();
        _lineStride = rgbFormat.getLineStride();
        _flipped = rgbFormat.getFlipped();
        _endian = rgbFormat.getEndian();
        _frameRate = rgbFormat.getFrameRate();
     }

    public int getSize ()
    {
        return (_byteData.length);
    }

    public Buffer getBuffer()
    {
        Buffer resultBuffer = new Buffer();
        
        if (_byteData == null) {
            RGBFormat ourFormat = new RGBFormat (new Dimension(_width, _height), 
                                                 _intData.length, 
                                                 _intData.getClass(), 
                                                 _frameRate, 
                                                 _bitsPerPixel, 
                                                 _redMask, 
                                                 _greenMask, 
                                                 _blueMask,
                                                 _pixelStride,
                                                 _lineStride,
                                                 _flipped,
                                                 _endian);
            resultBuffer.setFormat (ourFormat);
            resultBuffer.setData (_intData);
        }
        else if (_intData == null) {
             RGBFormat ourFormat = new RGBFormat (new Dimension(_width, _height), 
                                                 _byteData.length, 
                                                 _byteData.getClass(), 
                                                 _frameRate, 
                                                 _bitsPerPixel, 
                                                 _redMask, 
                                                 _greenMask, 
                                                 _blueMask,
                                                 _pixelStride,
                                                 _lineStride,
                                                 _flipped,
                                                 _endian);
            resultBuffer.setFormat (ourFormat);
            resultBuffer.setData (_byteData);
        }
        return resultBuffer;
    }

    public void writeOut (DataOutputStream dos)
        throws IOException
    {
        if (_byteData != null) {
            dos.writeByte ('B');    // 'B' implies a byte array
            dos.writeInt (_byteData.length);
            dos.write (_byteData);
        }
        else {
            dos.writeByte ('I');    // 'I' implies an int array
            dos.writeInt (_intData.length);
            for (int i = 0; i < _intData.length; i++) {
                dos.writeInt (_intData[i]);
            }
        }
        dos.writeInt (_width);
        dos.writeInt (_height);
        dos.writeInt (_bitsPerPixel);
        dos.writeInt (_redMask);
        dos.writeInt (_greenMask);
        dos.writeInt (_blueMask);
        dos.writeInt (_lineStride);
        dos.writeInt (_pixelStride);
        dos.writeInt (_flipped);
        dos.writeInt (_endian);
        dos.writeFloat (_frameRate);
    }
    
    public void readIn (DataInputStream dis)
        throws IOException
    {
        byte type = dis.readByte();
        if (type == 'B') {
            // Reading a byte array
            int length = dis.readInt();
            _byteData = new byte[length];
            dis.readFully (_byteData);
        }
        else if (type == 'I') {
            // Reading an int array
            int length = dis.readInt();
            _intData = new int[length];
            for (int i = 0; i < length; i++) {
                _intData[i] = dis.readInt();
            }
        }
        else {
            throw new IOException ("unexpected data array type " + type);
        }
        _width = dis.readInt();
        _height = dis.readInt();
        _bitsPerPixel = dis.readInt();
        _redMask = dis.readInt();
        _greenMask = dis.readInt();
        _blueMask = dis.readInt();
        _lineStride = dis.readInt();
        _pixelStride = dis.readInt();
        _flipped = dis.readInt();
        _endian = dis.readInt();
        _frameRate = dis.readFloat();
    }
 
    private byte[] _byteData = null;
    private int[]  _intData = null;
    private int    _width;
    private int    _height;
    private int    _bitsPerPixel;
    private int    _redMask;
    private int    _greenMask;
    private int    _blueMask;
    private int    _lineStride;
    private int    _pixelStride;
    private int    _flipped;
    private int    _endian;
    private float  _frameRate;
}
