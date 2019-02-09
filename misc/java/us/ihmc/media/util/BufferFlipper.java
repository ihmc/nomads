package us.ihmc.media.util;

/**
 * BufferFlipper flips the image encoded in a Buffer.
 * <p>
 * NOTE: This class currently only supports a Buffer format of RGB with the data stored in a byte array.
 * Support needs to be added for YUV, RGBA, etc. as well as data stored in a packed int array.
 */

import java.awt.Dimension;

import javax.media.Buffer;
import javax.media.Format;
import javax.media.format.RGBFormat;


public class BufferFlipper
{
    /**
     * Create an instance of the flipper for the specified format. All 
     * subsequent images processed by this instance must be of the same format.
     * <p>
     * @param format    format for the Buffer objects that are to be processed.
     * 
     * @exception UnsupportedFormatException if supplied format is not supported.
     */
    public BufferFlipper (Format format) throws UnsupportedFormatException
    {
        if (format instanceof RGBFormat) {
            _rgbFormat = (RGBFormat) format;
        }
        else {
            throw new UnsupportedFormatException ("cannot handle format of type "
                + format.getClass().getName());
        }
    }

    /**
     * Flip a buffer vertically.  The buffer is modified in place.
     * <p>
     * @param buffer    image to be flipped.
     */
    public void flipVertically (Buffer buffer)
    {
        if (buffer.getData() instanceof byte[]) {
            byte[] data = (byte[]) buffer.getData();
            Dimension size = _rgbFormat.getSize();
            int lineStride = _rgbFormat.getLineStride();
            
            for (int i = 0; i < size.height / 2; i++) {
                for (int j = 0; j < lineStride; j++) {
                    int index1 = i * lineStride + j;
                    int index2 = (size.height-i-1) * lineStride + j;
                    byte b = data[index1];
                    data[index1] = data[index2];
                    data[index2] = b;
                }
            }
        }
    }
    
    
    private RGBFormat _rgbFormat;
}
