package us.ihmc.chunking.android;

import java.io.IOException;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;

import android.content.Context;
import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Reassembler;
import us.ihmc.chunking.FileUtils;

/**
 * Test for the Android open XML document reassembly.
 * 
 * @author lbunch
 *
 */
public class AndroidOoxmlTest {

	// logging (replace with android Log)
	private static Logger logger = Logger.getLogger(AndroidOoxmlTest.class.getName());
	
	// Android application context used to access test resources from the file system
    public static Context appContext = null;

    /**
     * Submit the total number of chunks the original file was fragmented into,
     * followed by pairs of space-delimited parameters specifying the chunks to reassemble:
		[total chunk count] [ooxml chunk file path] [chunk id] [ooxml chunk file path] [chunk id] ...
		example: 4 resources/test_chunk1.pptx 1 resources/test_chunk2.pptx 2
			which reassembles these 2 of 4 chunks to produce a result file "resources/test_out.pptx"
     * @param argv space delimited argument list
     */
	public static void main(String[] argv) {
		logger.log(Level.INFO, "Usage: Submit the total number of chunks the original file was fragmented into\n"
				+ "followed by space-delimited parameters specifying the chuncks to reassemble:"
				+ "[total chunk count] [ooxml chunk file path] [chunk id] [ooxml chunk file path] [chunk id] ...\n"
				+ "example: 4 \"resources/test_chunk1.pptx\" 1 \"resources/test_chunk2.pptx\" 2\n"
				+ "which combines these chunks to produce a result file \"resources/test_out.pptx\"");

		// the List of ChunkWrappers that will be reassembled
        ArrayList<ChunkWrapper> chunkList = new ArrayList<ChunkWrapper>();
        // the total number of chunks the original doc was fragmented into
        byte totalChunkCount = Byte.parseByte(argv[0]);
        // the path to the first chunk to reassemble
        String firstChunkFilePathStr = null;
        // parse the parameter pairs [ooxml chunk file path] [chunk id]
        for (int a = 1; a < argv.length; a += 2) {
			try {
				String ooxmlFilePathStr = argv[a];
                if (firstChunkFilePathStr == null) {
                	// save the path to the first chunk to use later for the resulting output file path
                    firstChunkFilePathStr =ooxmlFilePathStr;
                }
				byte chunkId = Byte.parseByte(argv[a + 1]);
				
				// read in the chunk
				byte[] originalFileData = OoxmlChunkUtils.readFileContents(ooxmlFilePathStr, appContext);
				// create a ChunkWrapper representing this parameter
                ChunkWrapper chunkWrapper = new ChunkWrapper(originalFileData, chunkId, totalChunkCount, FileUtils.getMimeTypeForFile(ooxmlFilePathStr));
                // add the ChunkWrapper to the list of chunks to reassemble
                chunkList.add(chunkWrapper);
			} catch (Exception e) {
				logger.log(Level.SEVERE, "", e);
			}
		}

        // reassembled result will be written to a new file with suffix "_out" in the same location as the first chunk
        String reassembledFilePathStr = FileUtils.getAllExceptExtension(firstChunkFilePathStr) + "_out." + FileUtils.getFileExtension(firstChunkFilePathStr);
        Reassembler reassembler = new OoxmlReassembler();
        // reassemble the list of chunks and get the bytes of the resulting document
        // note: the AnnotationsWrapper parameter is null for now (no annotation handling)
        //		 the compression quality is 100% (best/lossless)
        byte[] reassembledFileData = reassembler.reassemble(chunkList, null, FileUtils.getMimeTypeForFile(firstChunkFilePathStr), totalChunkCount, (byte) 100);
        try {
        	// write the result to the file system
            OoxmlChunkUtils.writeFileContents(reassembledFilePathStr, reassembledFileData, appContext);
        } catch (IOException e) {
            logger.log(Level.SEVERE, "", e);
        }
	}
}
