package us.ihmc.chunking.server;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Collection;
import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Fragmenter;
import us.ihmc.chunking.Reassembler;
import us.ihmc.chunking.FileUtils;

public class OoxmlTest {

	private static Logger logger = Logger.getLogger(OoxmlTest.class.getName());

	public static void main(String[] argv) {
		logger.log(Level.FINE, "OoxmlTest usage:\nSubmit pairs of space-delimited parameters specifying "
				+ "the ooxml file path followed by the number of chunks to produce.\n"
				+ "example: \"resources/test.pptx\" 4 \nwhich produces a result file \"resources/test_out.pptx\"");
		for (int a = 0; a < argv.length; a += 2) {
			try {
				String ooxmlFileStr = argv[a];
				byte chunkCount = Byte.parseByte(argv[a + 1]);
				Path ooxmlPath = Paths.get(ooxmlFileStr);
				byte[] originalFileData = Files.readAllBytes(ooxmlPath);
				String resourceName = ooxmlPath.getFileName().toString();
				
				// fragment the file into the desired number of chunks
                                final String mimeType = FileUtils.getMimeTypeForFile(resourceName);
				Fragmenter fragmenter = new OoxmlFragmenter();
				Collection<ChunkWrapper> chunks = fragmenter.fragment(originalFileData, mimeType, chunkCount, (byte)90);
			
				// reassemble the chunks into a new file with suffix "_out"
				Path reassembledFilePath = Paths.get(FileUtils.getAllExceptExtension(ooxmlPath.toString()) + "_out." + FileUtils.getFileExtension(ooxmlPath.toString()));
				Reassembler reassembler = new OoxmlReassembler();
				byte[] reassembledFileData = reassembler.reassemble(chunks, null, mimeType, chunkCount, (byte) 90);
				Files.write(reassembledFilePath, reassembledFileData);

				ChunkWrapper[] allChunksArray = chunks.toArray(new ChunkWrapper[4]);
				
				chunks.remove(allChunksArray[3]);
				reassembledFilePath = Paths.get(FileUtils.getAllExceptExtension(ooxmlPath.toString()) + "_1-2-3." + FileUtils.getFileExtension(ooxmlPath.toString()));
				reassembler = new OoxmlReassembler();
				reassembledFileData = reassembler.reassemble(chunks, null, mimeType, chunkCount, (byte) 90);
				Files.write(reassembledFilePath, reassembledFileData);

				chunks.remove(allChunksArray[2]);
				reassembledFilePath = Paths.get(FileUtils.getAllExceptExtension(ooxmlPath.toString()) + "_1-2." + FileUtils.getFileExtension(ooxmlPath.toString()));
				reassembler = new OoxmlReassembler();
				reassembledFileData = reassembler.reassemble(chunks, null, mimeType, chunkCount, (byte) 90);
				Files.write(reassembledFilePath, reassembledFileData);

				chunks.remove(allChunksArray[1]);
				reassembledFilePath = Paths.get(FileUtils.getAllExceptExtension(ooxmlPath.toString()) + "_1." + FileUtils.getFileExtension(ooxmlPath.toString()));
				reassembler = new OoxmlReassembler();
				reassembledFileData = reassembler.reassemble(chunks, null, mimeType, chunkCount, (byte) 90);
				Files.write(reassembledFilePath, reassembledFileData);

			} catch (Exception e) {
				logger.log(Level.SEVERE, "", e);
			}
		}
		// workaround for Oracle JVM bug "JDWP exit error AGENT_ERROR_NO_JNI_ENV(183):  [util.c:840]" 
		// http://stackoverflow.com/questions/2225737/error-jdwp-unable-to-get-jni-1-2-environment#2225806
		System.exit(0);
	}
}
