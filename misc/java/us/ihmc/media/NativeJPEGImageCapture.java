package us.ihmc.media;

/**
 * Grabs images using a platform-specific implementation.
 * Images are returned as a byte array contained JPEG data.
 */
public class NativeJPEGImageCapture
{
    public static class Image
    {
        public int width;
        public int height;
        public byte[] data;
    }

    public static native boolean grabImage (Image i);
    
    static {
        System.loadLibrary ("imagegrab");
    }
}
