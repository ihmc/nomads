package us.ihmc.chunking.android;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.provider.ContactsContract;
import us.ihmc.chunking.AnnotationWrapper;
import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Reassembler;

import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Collection;
import java.util.Iterator;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Created by filippo on 7/27/18.
 */
public class JPEGReassembler implements Reassembler {

    private static Logger logger = Logger.getLogger(JPEGReassembler.class.getName());

    @Override
    public byte[] reassemble(Collection<ChunkWrapper> chunks, Collection<AnnotationWrapper> annotations,
                             String mimeType, byte totalNumberOfChunks, byte compressionQuality) {

        byte[] resultBytes = new byte[0];
        logger.log(Level.WARNING, "JPEGReassembler: reassembler called()");

        if (chunks != null && chunks.isEmpty()) {
            logger.log(Level.WARNING, "Cannot reassemble zero chunks");
            return resultBytes;
        }

        Iterator<ChunkWrapper> chunkIter = chunks.iterator();
        BMPReassembler bmpReassembler = new BMPReassembler(totalNumberOfChunks);
        while (chunkIter.hasNext()) {
            logger.log(Level.WARNING, "JPEGReassembler: has next chunk true");
            ChunkWrapper inputChunk = chunkIter.next();
            BufferedImage sourceImage = ImageIO.read(inputChunk.getData());
            if ( !bmpReassembler._abAddedChunks[inputChunk.getChunkId()]) {
                bmpReassembler.incorporateChunk(sourceImage, inputChunk.getChunkId());
                if (inputChunk.getChunkId() != totalNumberOfChunks) {
                    bmpReassembler.interpolate();
                }
                logger.log(Level.WARNING, "JPEGReassembler: incorporated chunkId " + inputChunk.getChunkId() + " " +
                        "of len " + inputChunk.getData().length);
            }
        }
        //bmpReassembler.interpolate();
        BufferedImage reassembledImage =  bmpReassembler.getReassembledImage();
        if (reassembledImage != null) {
            ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
            reassembledImage.write(Bitmap.CompressFormat.JPEG, 100, outputStream);
            resultBytes = outputStream.toByteArray();
            logger.log(Level.WARNING, "JPEGReassembler: reassembled image of len: " + resultBytes.length);
        }
        return resultBytes;
    }
}
