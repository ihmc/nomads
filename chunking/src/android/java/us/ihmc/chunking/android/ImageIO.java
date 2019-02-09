package us.ihmc.chunking.android;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.ZipInputStream;

/**
 * Android only. Depends on and.awt.ImageIO from the POI-Android library.
 * 
 * Wrapper for android.graphics.BitmapFactory that presents much like javax.imageio.ImageIO. 
 * 
 * @author cyberlab
 *
 */
public class ImageIO extends and.awt.ImageIO {


	// 512KB buffer
	private static final byte[] BUFFER = new byte[512 * 1024];

	/**
	 * Read and decode the image bytes from the given input stream.
	 * Currently only supports JPEG and PNG.
	 * 
	 * @param byteArrayInputStream - the input stream to read
	 * @return - a BufferedImage containing the image data, or null if the image cannot be read/decoded
	 */
	public static BufferedImage read(InputStream byteArrayInputStream) {
        BitmapFactory.Options mutableOptions = new BitmapFactory.Options();
        // ensure the resulting image is mutable
        mutableOptions.inMutable=true;
		Bitmap bm = BitmapFactory.decodeStream(byteArrayInputStream, null, mutableOptions);
		return bm == null ? null : new BufferedImage(bm);
	}

	/**
	 * Read and decode the image bytes from the given array.
	 * Currently only supports JPEG and PNG.
	 * 
	 * @param imageBytes - the array of bytes for an encoded image.
	 * @return - a BufferedImage containing the image data, or null if the image cannot be read/decoded
	 */
	public static BufferedImage read(byte[] imageBytes) {
		BitmapFactory.Options mutableOptions = new BitmapFactory.Options();
		// ensure the resulting image is mutable
		mutableOptions.inMutable=true;
		Bitmap bm = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.length, mutableOptions);
		return bm == null ? null : new BufferedImage(bm);
	}

	/**
	 * Read and decode the image bytes from the given zip inputstream.
	 * Currently only supports JPEG and PNG.
	 * 
	 * @param zipInputStream - the Zip inputstream to read the image from
	 * @return - a BufferedImage containing the image data, or null if the image cannot be read/decoded
	 */
	public static BufferedImage read(ZipInputStream zipInputStream) throws IOException {
		try (ByteArrayOutputStream resultBytes = new ByteArrayOutputStream(zipInputStream.available())) {
			int len;
			while ((len = zipInputStream.read(BUFFER)) > 0) {
				resultBytes.write(BUFFER, 0, len);
			}
            BitmapFactory.Options mutableOptions = new BitmapFactory.Options();
            mutableOptions.inMutable=true;
			Bitmap bm = BitmapFactory.decodeByteArray(resultBytes.toByteArray(), 0, resultBytes.size(), mutableOptions);
			return bm == null ? null : new BufferedImage(bm);
		}
	}
}
