package us.ihmc.media;

import java.awt.Dimension;

import javax.media.Buffer;
import javax.media.Format;
import javax.media.format.RGBFormat;

/**
 * Grabs images using a platform-specific implementation.
 */
public class NativeImageCapture
{
    public static synchronized Buffer grabImage()
    {
        byte[] data = getImageData();
        if (data == null) {
            return null;
        }
        RGBFormat format = new RGBFormat (new Dimension (320, 240),
                                          320 * 240 * 3,
                                          Format.byteArray,
                                          1.0f,
                                          24,
                                          1,
                                          2,
                                          3,
                                          3,
                                          320 * 3,
                                          RGBFormat.FALSE,
                                          RGBFormat.LITTLE_ENDIAN);
        Buffer buffer = new Buffer();
        buffer.setData (data);
        buffer.setFormat (format);
        buffer.setLength (data.length);
        buffer.setSequenceNumber (seqNum++);
        buffer.setTimeStamp (System.currentTimeMillis());
        return buffer;
    }

    private static native byte[] getImageData();
    
    private static long seqNum;

    static {
        System.loadLibrary ("imagegrab");
    }
}
