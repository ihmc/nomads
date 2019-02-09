package us.ihmc.media.util;

import java.awt.Point;
import java.awt.Transparency;

import java.awt.color.ColorSpace;
import java.awt.color.ICC_ColorSpace;
import java.awt.color.ICC_Profile;

import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;
import java.awt.image.ComponentColorModel;
import java.awt.image.DataBuffer;
import java.awt.image.DataBufferByte;
import java.awt.image.Raster;
import java.awt.image.WritableRaster;

import javax.media.Buffer;
import javax.media.Format;

import javax.media.format.RGBFormat;

/**
 * BufferToBufferedImage converts objects of type javax.media.Buffer into java.awt.image.BufferedImage.
 * BufferedImage objects can be used with the ImageIO libraries or with com.sun.image.codec.jpeg.JPEGImageEncoder.
 * <p>
 * NOTE: This class currently only supports a Buffer format of RGB with the data stored in a byte array.
 * Support needs to be added for YUV, RGBA, etc. as well as data stored in a packed int array.
 */
public class BufferToBufferedImage
{
    /**
     * Create an instance of the converter for the specified format. All images subsequently converted
     * by this instance must be of the same format.
     * 
     * @param format    the format for the Buffer objects that are to be converted
     * 
     * @exception UnsupportedFormatException if the supplied format is not supported by this converter
     */
    public BufferToBufferedImage (Format format)
        throws UnsupportedFormatException
    {
        if (format instanceof RGBFormat) {
            // This is an RGB type image
            _rgbFormat = (RGBFormat) format;
            
            if (format.getDataType().getName().equals ("[B")) {
                // The data is stored as a byte array
                
                // Create the band offsets
                /*!!*/ // TODO - check if the band offsets are related to the red, green, and blue mask
                       // values in RGBFormat
                _bandOffsets = new int[3];
                _bandOffsets[0] = 2;
                _bandOffsets[1] = 1;
                _bandOffsets[2] = 0;
                
                // Create the ColorModel
                _colorModel = new ComponentColorModel (new ICC_ColorSpace (ICC_Profile.getInstance (ColorSpace.CS_sRGB)),
                                                       null, false, false, Transparency.OPAQUE, DataBuffer.TYPE_BYTE);
                //added second param to comply with java 1.2.2 for oasis classic compile
            }
            else if (format.getDataType().getName().equals ("[I")) {
                // The data is stored as a int array
                // Nothing to do in this case
            }
            else {
                throw new UnsupportedFormatException ("cannot handle data of type " + format.getDataType().getName());
            }
        }
        else {
            throw new UnsupportedFormatException ("cannot handle format of type " + format.getClass().getName());
        }
        
        // The location specifies the starting point for the BufferedImage - we always want the whole image
        _location = new Point (0,0);
    }

    /**
     * Convert a Buffer into a BufferedImage.
     * <p>
     * NOTE: The byte array from the Buffer is reused in the BufferedImage.
     * 
     * @param buffer    the original image as a Buffer object
     * 
     * @return          the converted image as a BufferedImage
     * 
     * @exception UnsupportedFormatException if the data in the buffer is of a different type
     */
    public BufferedImage createBufferedImage (Buffer buffer)
        throws UnsupportedFormatException
    {
        if (buffer.getData() instanceof byte[]) {
            byte[] data = (byte[]) buffer.getData();
            DataBufferByte dataBuffer = new DataBufferByte (data, data.length);
            WritableRaster raster = Raster.createInterleavedRaster (dataBuffer,
                                                                    _rgbFormat.getSize().width,
                                                                    _rgbFormat.getSize().height,
                                                                    _rgbFormat.getLineStride(),
                                                                    _rgbFormat.getPixelStride(),
                                                                    _bandOffsets,
                                                                    _location);
            return new BufferedImage (_colorModel, raster, false, null);
        }
        else if (buffer.getData() instanceof int[]) {
            int[] data = (int[]) buffer.getData();
            BufferedImage bImg = new BufferedImage (_rgbFormat.getSize().width,
                                                    _rgbFormat.getSize().height,
                                                    BufferedImage.TYPE_INT_RGB);
            bImg.setRGB (0, 0, _rgbFormat.getSize().width, _rgbFormat.getSize().height,
                         data, 0, _rgbFormat.getSize().width);
            return bImg;
        }
        else {
            throw new UnsupportedFormatException ("cannot handle data of type " + buffer.getData().getClass().getName());
        }
    }

    private RGBFormat _rgbFormat;
    private int[] _bandOffsets;
    private ColorModel _colorModel;
    private Point _location;
}
