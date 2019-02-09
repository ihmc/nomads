package us.ihmc.chunking.server;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Collection;
import java.util.Iterator;
import java.util.UUID;
import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.chunking.AnnotationWrapper;
import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Reassembler;
import us.ihmc.chunking.FileUtils;

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

        // init result as size 0 since we will return an empty result if reassembly is not possible.
        byte[] resultBytes = new byte[0];

        // validate parameter
        if (chunks.isEmpty()) {
            logger.log(Level.WARNING, "Cannot reassemble zero chunks");
            return resultBytes;
        }
        // the temp file that will hold the final resulting document
        Path resultOoxmlPath = null;
        try {
            // ensure the temp directory exists
            File tf;
            Path tempDirectoryPath = Paths.get(tempFilePath);
            if (!Files.exists(tempDirectoryPath)) {
                Path tp = Files.createDirectories(tempDirectoryPath);
                tf = tp.toFile();
                tf.deleteOnExit();
            }
            else {
                tf = tempDirectoryPath.toFile();
            }

            String resourceName = "reass-" + UUID.randomUUID().toString();            
            Iterator<ChunkWrapper> chunkIter = chunks.iterator();

            // write the first chunk out as the result file
            ChunkWrapper firstChunk = chunkIter.next();
            resultOoxmlPath = File.createTempFile(resourceName,
                    FileUtils.getFileExtensionForMimeType(mimeType), tf).toPath();
            Files.write(resultOoxmlPath, firstChunk.getData());

            // write each subsequent chunk out as a file and merge that chunk into the result file
            int[] incorporatedChunks = null;
            int i = 0;

            while (chunkIter.hasNext()) {
                ChunkWrapper inputChunk = chunkIter.next();
                Path chunkPath = Paths.get(tempFilePath, resourceName + inputChunk.getChunkId());
                try {
                    // write the chunk out as a temp file
                    Files.write(chunkPath, inputChunk.getData());
                    // add the current chunk to the final resulting temp file
                    OoxmlChunkUtils.reassembleImages(resultOoxmlPath, firstChunk.getChunkId(), chunkPath, inputChunk.getChunkId(),
                            firstChunk.getTotalNumberOfChunks(), incorporatedChunks);
                    // keep track of the chunks that have already been woven into the final
                    if (incorporatedChunks == null) {
                        incorporatedChunks = new int[chunks.size()];
                        incorporatedChunks[i++] = firstChunk.getChunkId();
                    }
                    incorporatedChunks[i++] = inputChunk.getChunkId();
                } catch (Exception e) {
                    logger.log(Level.SEVERE, "Error writing file: " + chunkPath, e);
                }
            }

            // read the result temp file into the result byte array
            if (resultOoxmlPath != null) {
                resultBytes = Files.readAllBytes(resultOoxmlPath);
            }
        } catch (IOException e) {
            logger.log(Level.SEVERE, "Error reassembling resource: " + resultOoxmlPath, e);
        }

        // return the result byte array
        return resultBytes;
    }

}
