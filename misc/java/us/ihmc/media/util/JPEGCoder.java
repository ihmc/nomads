package us.ihmc.media.util;

import com.sun.image.codec.jpeg.JPEGCodec;
import com.sun.image.codec.jpeg.JPEGEncodeParam;
import com.sun.image.codec.jpeg.JPEGImageDecoder;
import com.sun.image.codec.jpeg.JPEGImageEncoder;

import java.awt.image.BufferedImage;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

import javax.media.Buffer;
import javax.media.Format;

/**
 * JPEGCoder is used to encode and decode to JPEG images. Either a javax.media.Buffer
 * or a java.awt.image.BufferedImage may be used as input to create a JPEG image. Decoding
 * a JPEG image results in a java.awt.image.BufferedImage.
 * <p>
 * Note that when encoding a java.media.Buffer, the method first internally converts
 * the Buffer to a BufferedImage.
 * 
 */
public class JPEGCoder
{
    /**
     * Encodes the image in a Buffer into a JPEG image with the specified quality setting.
     * The encoded image is returned in a byte array.
     * 
     * @param b         the image to be encoded
     * @param quality   the JPEG quality setting - from 0.0 to 1.0
     * 
     * @return the image encoded in JPEG format
     * 
     * @exception UnsupportedFormatException     if the format of the buffer cannot be handled
     */
    public synchronized static byte[] encode (Buffer b, float quality)
        throws UnsupportedFormatException
    {
        if (_btobi == null) {
            _format = b.getFormat();
            _btobi = new BufferToBufferedImage (_format);
        }
        else if (!(_format.equals (b.getFormat()))) {
            _format = b.getFormat();
            _btobi = new BufferToBufferedImage (_format);
        }
        BufferedImage bufImg = _btobi.createBufferedImage (b);

        return encode (bufImg, quality);
    }
    
    /**
     * Encodes the image in a BufferedImage into a JPEG image with the specified quality setting.
     * The encoded image is returned in a byte array.
     * 
     * @param bi        the image to be encoded
     * @param quality   the JPEG quality setting - from 0.0 to 1.0
     * 
     * @return the image encoded in JPEG format
     * 
     * @exception UnsupportedFormatException     if the format of the buffer cannot be handled
     */
    public synchronized static byte[] encode (BufferedImage bi, float quality)
        throws UnsupportedFormatException
    {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        JPEGImageEncoder encoder = JPEGCodec.createJPEGEncoder (baos);
        JPEGEncodeParam param = encoder.getDefaultJPEGEncodeParam (bi);
        param.setQuality (quality, false);
        encoder.setJPEGEncodeParam (param);
        try {
            encoder.encode (bi);
        }
        catch (IOException e) {
            throw new UnsupportedFormatException ("failed to encode image; nested exception: " + e);
        }
        return baos.toByteArray();
    }

    public synchronized static BufferedImage decode (byte[] jpegImage)
        throws UnsupportedFormatException
    {
        return decode (jpegImage, 0, jpegImage.length);
    }

    public synchronized static BufferedImage decode (byte[] jpegImage, int offset)
        throws UnsupportedFormatException
    {
        return decode (jpegImage, offset, jpegImage.length - offset);
    }

    /**
     * Decodes the JPEG image into a BufferedImage.
     * 
     * @param jpegImage the image to be decoded
     * @param offset the offset in the buffer of the first byte of the image
     * @param length the number of bytes in the buffer that make up the image
     * 
     * @return the decoded image as a BufferedImage
     * 
     * @exception UnsupportedFormatException     if the format of the image cannot be handled
     */
    public synchronized static BufferedImage decode (byte[] jpegImage, int offset, int length)
        throws UnsupportedFormatException
    {
        ByteArrayInputStream bais = new ByteArrayInputStream (jpegImage, offset, length);
        JPEGImageDecoder decoder = JPEGCodec.createJPEGDecoder (bais);
        BufferedImage bufImg = null;
        try {
            bufImg = decoder.decodeAsBufferedImage();
        }
        catch (Exception e) {
            throw new UnsupportedFormatException ("failed to decode image; nested exception: " + e);
        }
        return bufImg;
    }

    private static Format _format;
    private static BufferToBufferedImage _btobi;
}
