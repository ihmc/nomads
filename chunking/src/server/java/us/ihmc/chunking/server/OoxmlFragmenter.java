package us.ihmc.chunking.server;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.UUID;
import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Fragmenter;
import us.ihmc.chunking.Interval;
import us.ihmc.chunking.FileUtils;

/**
 * Breaks an Ooxml (Open Office XML) document down into a set of smaller
 * 'Chunks'.
 *
 * Ooxml includes Microsoft Powerpoint pptx and Word docx as well as Open Office
 * Text sxi and Presentation sxw.
 *
 * Ooxml documents are essential zip files that contain a single folder of all
 * images (and other media) embedded in the document. This folder is /xxx/media
 * where the 'xxx' directory depends on the file type (e.g. /ppt/media for MS
 * Powerpoint and /word/media for MS Word)
 *
 * Fragmenting involves replacing most images in the /xxx/media/ folder with
 * lower resolution copies of those images (see BMPFragmenter). Each 'chunk' can
 * be used on its own and the chunks can be reassembled to reproduce the
 * original data (e.g. high resolution image).
 *
 * @author lbunch
 *
 */
public class OoxmlFragmenter implements Fragmenter {

    public enum SupportedTypes {
        OPEN_DOC ("application/vnd.openxmlformats-officedocument.wordprocessingml.document"),
        OPEN_PRES ("application/vnd.openxmlformats-officedocument.presentationml.presentation"),
        OPEN_SHEETS ("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");

        public final String _mimeType;

        SupportedTypes(String mimeType) {
            _mimeType = mimeType;
        }
    }

    private static Logger logger = Logger.getLogger(OoxmlFragmenter.class.getName());

    /**
     * This class uses temporary files to store the original data and resulting
     * Chunks.
     */
    public static final String tempFilePath = "temp";

    /**
     * Break the given data (an Ooxml document) down into a set of smaller
     * Chunks.
     *
     * @param data - the 'high resolution' Ooxml document that will be broken
     * down into smaller chunks
     * @param mimeType - the MIME type of the data
     * @param totalNumberOfChunks - how many smaller chunks to break the data
     * into (likely limited to an even or square multiple).
     * @return - the smaller chunks resulting from breaking down the given data
     */

    public List<ChunkWrapper> fragment (byte[] data, String mimeType, byte totalNumberOfChunks, byte compressionQuality) {
        
        String resource = "";
        List<ChunkWrapper> chunkResults = new ArrayList<ChunkWrapper>();
        try {
            // ensure the temp directory  exists
            File tf = null;
            Path tempDirectoryPath = Paths.get(tempFilePath);
            if (!Files.exists(tempDirectoryPath)) {
                Path tp = Files.createDirectories(tempDirectoryPath);
                tf = tp.toFile();
                tf.deleteOnExit();
            }
            else {
                tf = tempDirectoryPath.toFile();
            }

            // write the given ooxml data to a file, replacing the file if it already exists
            Path ooxmlPath = File.createTempFile("origin-" + UUID.randomUUID().toString(),
            FileUtils.getFileExtensionForMimeType(mimeType), tf).toPath();
            Files.write(ooxmlPath, data);
            resource = ooxmlPath.toString();

            // process the original file into a set of new smaller chunk files, returning the Paths to the chunk temp files
            List<Path> chunkFilePaths = OoxmlChunkUtils.chunkImages(ooxmlPath, totalNumberOfChunks);

            // create in memory chunks from the new chunk files
            for (byte i = 0; i < chunkFilePaths.size(); i++) {
                byte[] chunkedData =  Files.readAllBytes(chunkFilePaths.get(i));
                ChunkWrapper chunkResult = new ChunkWrapper (chunkedData, (byte) (i + 1), totalNumberOfChunks, mimeType);
                // chunkResult.resourceName = "";
                // chunkResult.chunkName = chunkFilePaths.get(i).getFileName().toString();
                chunkResults.add(chunkResult);
            }
        } catch (IOException e) {
            logger.log(Level.SEVERE, "Error chunking resource: " + resource, e);
        }
        // return the list of in memory chunks
        return chunkResults;
    }

    public byte[] extract (byte[] data, String inputMimeType, byte nChunks,
                           byte compressionQuality, Collection<Interval> intervals)
    {
        // TODO: implement this
        return null;
    }
}

