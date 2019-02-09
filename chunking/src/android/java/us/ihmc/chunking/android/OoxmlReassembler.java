package us.ihmc.chunking.android;

import android.util.Log;

import java.io.IOException;
import java.util.Collection;
import java.util.Iterator;
import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.chunking.AnnotationWrapper;
import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Reassembler;

/**
 * Reassembles a set of smaller Ooxml 'chunks' into a single 'higer resolution'
 * Ooxml document. Since the Ooxml document is a Zip file containing an
 * '/xxx/media' directory of images, this class iterates over all of the images
 * in '/xxx/media' and combines the lower-resolution image data from all of the
 * given Chunks.
 *
 * @author lbunch
 *
 */
public class OoxmlReassembler implements Reassembler {

    /**
     * This class uses temporary files to store the original data and resulting
     * Chunks.
     */
    public static final String tempFilePath = "temp";

    private static Logger logger = Logger.getLogger(OoxmlReassembler.class.getName());

    /**
     * Reassembles a set of smaller Ooxml 'chunks' into a single 'higher
     * resolution' Ooxml document.
     *
     * @param chunks - the smaller chunks to be reassembled
     * @param annotations
     * @param mimeType
     * @param totalNumberOfChunks
     * @param compressionQuality
     * @return - the result of combining all of the given chunks
     */
    public byte[] reassemble (Collection<ChunkWrapper> chunks, Collection<AnnotationWrapper> annotations,
                              String mimeType, byte totalNumberOfChunks, byte compressionQuality) {
    	// used to estimate the time taken to reassemble the chunks
        long startTime = System.currentTimeMillis();

        // init result as size 0 since we will return an empty result if reassembly is not possible.
        byte[] resultBytes = new byte[0];

        // validate parameter
        if (chunks.isEmpty()) {
            logger.log(Level.WARNING, "Cannot reassemble zero chunks");
            return resultBytes;
        }
        try {
            resultBytes = OoxmlChunkUtils.reassembleImages(chunks, compressionQuality);
        } catch (IOException e) {
            logger.log(Level.SEVERE, "", e);
        }
        // estimate and log the time taken to reassemble the chunks
        long reassemblyTime = System.currentTimeMillis() - startTime;
        Log.i("Document Reassembler", "reassembly time = " + reassemblyTime);
        // return the result byte array
        return resultBytes;
    }

}
