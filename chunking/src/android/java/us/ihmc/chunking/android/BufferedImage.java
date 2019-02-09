package us.ihmc.chunking.android;

import java.io.OutputStream;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;

/**
 * Android only. Depends on and.awt.BufferedImage from the POI-Android library.
 * 
 * Wrapper for android.graphics.Bitmap that presents much like java.awt.BufferedImage.
 * 
 * @author lbunch
 *
 */
public class BufferedImage extends and.awt.BufferedImage {

	// Converting Bitmap Config to BufferedImage types
	public static final Config TYPE_INT_RGB = Config.RGB_565;
	public static final Config TYPE_3BYTE_BGR = Config.RGB_565;
	public static final Config TYPE_INT_ARGB = Config.ARGB_8888;
	public static final Config TYPE_4BYTE_ABGR = Config.ARGB_8888;

	/**
	 * Initialize with an existing android Bitmap
	 * @param bm an existing Bitmap that will be used to setup the size, configuration (e.g. bit depth), 
	 * and contents of this image
	 */
	public BufferedImage(Bitmap bm) {
        super(bm);
        width = bm.getWidth();
	    height = bm.getHeight();
	}

	/**
	 * Initialize a blank image with the given size and configuration (e.g. bit depth)
	 * @param width - width of the new blank image
	 * @param height - height of the new blank image
	 * @param config - specifies bit depth and byte order of the new image
	 */
	public BufferedImage(int width, int height, Config config)
    {
		super(width, height, config);
		this.width = width;
		this.height = height;
	}

	/**
	 * Write the contents of this image to the given output stream with the given compression options
	 * @param compressFormat - non null, compression type: e.g. JPEG, PNG
	 * @param quality - value between 0 (small size) and 100 (high quality)
	 * @param outputStream - the outputstream to write the compressed data
	 */
	public void write(Bitmap.CompressFormat compressFormat, int quality, OutputStream outputStream ) {
        bm.compress(compressFormat, quality, outputStream);
    }

	/**
	 * Get the color value of the pixel at the given location
	 * @param x - x coordinate of the pixel location
	 * @param y - y coordinate of the pixel location
	 * @return - int comprising the ARGB color of the pixel
	 */
	public int getRGB(int x, int y)
	{
        if (_pixels == null) {
        	_pixels = new int[width * height];
        	bm.getPixels(_pixels, 0, width, 0, 0, width, height);
		}
		return _pixels[(width * x) + y];
    }

	/**
	 * Set the color value of the pixel at the given location
	 * @param x - x coordinate of the pixel location
	 * @param y - y coordinate of the pixel location
	 * @param color - int comprising the ARGB color of the pixel
	 */
    public void setRGB(int x, int y, int color)
	{
		if (_pixels == null) {
			_pixels = new int[width * height];
			bm.getPixels(_pixels, 0,width, 0, 0, width, height);
		}
		try {
            _pixels[(width * x) + y] = color;
        } catch (IndexOutOfBoundsException e) {}
    }

    public void finalizeSetRGB()
	{
		if (_pixels == null) {
			return;
		}
		bm.setPixels(_pixels, 0, bm.getWidth(), 0, 0, bm.getWidth(), bm.getHeight());
	}

    public Config getConfig() {
        return bm.getConfig();
    }

    private int[] _pixels = null;
    private final int width;
    private final int height;
}
