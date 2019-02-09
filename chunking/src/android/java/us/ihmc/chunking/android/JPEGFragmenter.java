package us.ihmc.chunking.android;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Fragmenter;
import us.ihmc.chunking.Interval;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Created by filippo on 7/27/18.
 */
public class JPEGFragmenter implements Fragmenter {
    private static Logger logger = Logger.getLogger(JPEGFragmenter.class.getName());

    @Override
    public List<ChunkWrapper> fragment(byte[] data, String inputMimeType, byte nChunks, byte compressionQuality) {
        long  startTime = System.currentTimeMillis();

        if (inputMimeType != null && !inputMimeType.equals("image/jpeg")) {
            return null;
        }
        if (data == null) {
            return null;
        }
        //Creating a Bitmap from bytearray without coying it to memory
        BufferedImage sourceImage = ImageIO.read(data);
        List<ChunkWrapper> chunks = new ArrayList<ChunkWrapper>();
        for (int i = 1; i <= nChunks; i++) {
            BufferedImage outputChunk = BMPFragmenter.fragmentBMP(sourceImage, i, nChunks);

            ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
            outputChunk.write(Bitmap.CompressFormat.JPEG, 100, outputStream);
            ChunkWrapper chunk = new ChunkWrapper(outputStream.toByteArray(), (byte)i, (byte)nChunks, "image/jpeg");
            chunks.add(chunk);
        }
        long executionTime = System.currentTimeMillis() - startTime;
        logger.log(Level.WARNING, "fragment: execution time: " + executionTime + "ms");
        return chunks;
    }

    @Override
    public byte[] extract(byte[] data, String inputMimeType, byte nChunks, byte compressionQuality, Collection<Interval> intervals) {
        return new byte[0];
    }
}
