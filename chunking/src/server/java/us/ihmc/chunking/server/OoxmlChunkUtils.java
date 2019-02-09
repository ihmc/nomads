package us.ihmc.chunking.server;

import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.DirectoryStream;
import java.nio.file.FileSystem;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.imageio.IIOImage;
import javax.imageio.ImageIO;
import javax.imageio.ImageReader;
import javax.imageio.ImageWriter;
import javax.imageio.metadata.IIOMetadata;
import javax.imageio.stream.ImageInputStream;
import javax.imageio.stream.ImageOutputStream;

import us.ihmc.chunking.BMPUtils;
import us.ihmc.chunking.FileUtils;

public class OoxmlChunkUtils {

    private static final Logger LOGGER = Logger.getLogger(OoxmlChunkUtils.class.getName());

    // all of the root folder names within supported Ooxml documents 
    private static final String[] OOXML_MEDIA_FOLDER_ARRAY = {"/ppt/media", "/word/media", "/Pictures"};
    private static final List<String> OOXML_MEDIA_FOLDERS = Arrays.asList(OOXML_MEDIA_FOLDER_ARRAY);

    /**
     * Testing only. Breaks a given Ooxml file into the requested number of
     * chunks, then reassembles the chunks.
     *
     * @param argv
     */
    public static void main(String[] argv) {
        LOGGER.log(Level.INFO, "OoxmlUtil usage:\nSubmit pairs of space-delimited parameters, the ooxml file path followed by the number of chunks to produce.\n"
                + "example: \"resources/test.pptx\" 4");
        for (int a = 0; a < argv.length; a += 2) {
            try {
                String ooxmlFilePath = argv[a];
                int chunkCount = Integer.parseInt(argv[a + 1]);
                try {
                    List<Path> chunkPaths = OoxmlChunkUtils.chunkImages(Paths.get(ooxmlFilePath), chunkCount);
                    OoxmlChunkUtils.reassembleImages(chunkPaths.get(0), 1, chunkPaths.get(1), 2, 4, null);
                    OoxmlChunkUtils.reassembleImages(chunkPaths.get(0), 1, chunkPaths.get(2), 3, 4, new int[]{1, 2});
                    OoxmlChunkUtils.reassembleImages(chunkPaths.get(0), 1, chunkPaths.get(3), 4, 4, new int[]{1, 2, 3});
                } catch (IOException e) {
                    LOGGER.log(Level.WARNING, "error chunking image " + ooxmlFilePath + "x" + chunkCount, e);
                }
            } catch (NumberFormatException e) {
                LOGGER.log(Level.WARNING, "bad chunk count parameter: " + argv[a + 1], e);
            }
        }
    }

    /**
     * Fragment the Ooxml file into the requested number of smaller Chunk files.
     * This involves cloning the input Ooxml document to create a temp file
     * (e.g. test_1_of_4.pptx), then reducing the resolution of all supported
     * image files in the /xxx/media folder.
     *
     * @param inputOoxmlFilePath - the Path to the file to fragment
     * @param chunkCount - the number of Chunks to break the input file down
     * into (restricted to 2, 4, 8, 16).
     * @return - a List of Paths to temp files containing the resulting smaller
     * Chunk files.
     * @throws IOException
     */
    public static List<Path> chunkImages(Path inputOoxmlFilePath, int chunkCount) throws IOException {
        List<Path> resultChunkPaths = new ArrayList<Path>();

        String ooxmlRootFolderName = "";

        for (int chunkNumber = 1; chunkNumber <= chunkCount; chunkNumber++) {
            Path resultFilePath = Paths
                    .get(FileUtils.getAllExceptExtension(inputOoxmlFilePath.toString()) + "_" + chunkNumber + "_of_" + chunkCount + '.' + FileUtils.getFileExtension(inputOoxmlFilePath.toString()));

            // new copy of the original ooxml file for each 'chunk'
            Files.copy(inputOoxmlFilePath, resultFilePath, StandardCopyOption.REPLACE_EXISTING);

            // mount the zip file system
            try (FileSystem fs = FileSystems.newFileSystem(resultFilePath, null)) {

                Path mediaFolderInsideZipPath = getMediaDirectoryPath(fs);
                if (mediaFolderInsideZipPath == null) {
                    throw new IOException("failed to find the media folder (e.g. ppt/media, word/media, Pictures) that contains the images in the ooxml file: " + resultFilePath);
                }

                // list the contents of the /media/ folder
                try (DirectoryStream<Path> mediaDirectoryStream = Files.newDirectoryStream(mediaFolderInsideZipPath)) {
                    // for every file in the /media/ folder
                    // note: NOT expecting any subfolders
                    for (Path mediaFilePath : mediaDirectoryStream) {
                        if (BMPUtils.CHUNKABLE_IMAGE_FILE_EXTENSIONS.contains(FileUtils.getFileExtension(mediaFilePath.toString()))) {
                            LOGGER.fine("Chunking image: " + mediaFilePath);
                            //String resultImageFileLoc = getAllExceptExtension(mediaFilePath) + ".bmp";
                            Path resultImageFilePath = mediaFilePath; //fs.getPath(resultImageFileLoc);
                            try {
                                OoxmlChunkUtils.writeImageChunk(mediaFilePath, resultImageFilePath, FileUtils.getFileExtension(mediaFilePath.toString()), chunkNumber, chunkCount);
                            } catch (Exception e) {
                                LOGGER.log(Level.WARNING, "error writing chunk " + chunkNumber + " for image " + resultImageFilePath, e);
                            }
                        }
                        // else, any file that cannot be broken-down gets left as-is in every chunk
                    }
                } catch (IOException e) {
                    LOGGER.log(Level.WARNING, "error getting zip contents at " + mediaFolderInsideZipPath + " inside " + resultFilePath, e);
                }

            } catch (IOException e) {
                LOGGER.log(Level.WARNING, "error mounting zip filesystem " + resultFilePath, e);
            }
            resultChunkPaths.add(resultFilePath);
        }
        return resultChunkPaths;
    }

    /**
     * Not used.
     *
     * @param sourceImagePath
     * @param outputImagePath
     * @param outputFormat
     * @param chunkNumber
     * @param chunkCount
     * @throws IOException
     */
    private static void writeImageChunk_preserveMetadata(Path sourceImagePath, Path outputImagePath, String outputFormat, int chunkNumber, int chunkCount) throws IOException {

        // TRYING TO PRESERVE THE IMAGE METADATA, BUT CODE IS NOT READY YET
        try (InputStream readInputStream = Files.newInputStream(sourceImagePath, StandardOpenOption.READ)) {

            ImageInputStream input = ImageIO.createImageInputStream(readInputStream);

            Iterator<ImageReader> readers = ImageIO.getImageReaders(input);
            if (!readers.hasNext()) {
                throw new IOException("no image reader for file " + sourceImagePath);
            }
            // Read image and metadata
            ImageReader reader = readers.next();
            reader.setInput(input);
            IIOMetadata metadata = reader.getImageMetadata(0);
            BufferedImage image = reader.read(0);

            // reduce the original image to an image for the desired chunk
            BufferedImage chunkedImage = BMPFragmenter.fragmentBMP(image, chunkNumber, chunkCount);

            // Write image with metadata from original image, to maintain tRNS chunk
            try (OutputStream writeOutputStream = Files.newOutputStream(outputImagePath, StandardOpenOption.CREATE, StandardOpenOption.WRITE, StandardOpenOption.TRUNCATE_EXISTING)) {

                ImageWriter writer = ImageIO.getImageWritersBySuffix(outputFormat).next();
                ImageOutputStream output = ImageIO.createImageOutputStream(writeOutputStream);

                writer.setOutput(output);
                writer.write(new IIOImage(chunkedImage, Collections.<BufferedImage>emptyList(), metadata));
            }

        }

    }

    /**
     * Write a lower resolution version of a given image file by taking a sample
     * of the pixels based on the number of chunks. Depends on BMPFragmenter.
     *
     * @param sourceImagePath - path to the original (full resolution) image
     * file
     * @param outputImagePath - path to write out the lower resolution verison
     * of the image file
     * @param outputFormat - file extension based format designation for the
     * resulting output image file
     * @param chunkNumber - which 'chunk' to extract from the original image, >=
     * 1
     * @param chunkCount - the total number of 'chunks' that the original image
     * will be broken into
     * @throws IOException
     */
    private static void writeImageChunk(Path sourceImagePath, Path outputImagePath, String outputFormat, int chunkNumber, int chunkCount) throws IOException {
        try (InputStream readInputStream = Files.newInputStream(sourceImagePath, StandardOpenOption.READ)) {
            // read the original image
            BufferedImage image = ImageIO.read(readInputStream);

            // reduce the original image to an image for the desired chunk
            BufferedImage chunkedImage = BMPFragmenter.fragmentBMP(image, chunkNumber, chunkCount);
            // overwrite the original image with the smaller chunk
            try (OutputStream writeOutputStream = Files.newOutputStream(outputImagePath, StandardOpenOption.CREATE, StandardOpenOption.WRITE, StandardOpenOption.TRUNCATE_EXISTING)) {
                ImageIO.write(chunkedImage, outputFormat, writeOutputStream);
            }
        }
    }

    /**
     * Combine two image 'chunks' to form a higher resolution image. The result
     * replaces the first of the two given chunks. The first chunk parameter may
     * already contain the combination of two or more chunks.
     *
     * @param resultOoxmlFilePath - the file path of the first chunk to be
     * combined and into which the combined image result will be written
     * @param resultChunkNumber - the chunk id of the first chunk, >= 1
     * @param chunkOoxmlFilePath - the file path of the second chunk to be
     * combined with the first chunk
     * @param chunkNumber - the chunk id of the second chunk, >= 1
     * @param totalNumberOfChunks - the total number of chunks that the original
     * image was broken into
     * @param alreadyIncorporatedChunkIds - the chunk ids of chunks already
     * incorporated into the first (and resulting) chunk.
     */
    public static void reassembleImages(Path resultOoxmlFilePath, int resultChunkNumber, Path chunkOoxmlFilePath, int chunkNumber, int totalNumberOfChunks, int[] alreadyIncorporatedChunkIds) {

        HashMap<String, BufferedImage> chunkImageMap = new HashMap<String, BufferedImage>();

        // mount the chunk zip file system
        try (FileSystem chunkFs = FileSystems.newFileSystem(chunkOoxmlFilePath, null)) {
            List<Path> chunkFilePaths = listChunkableFiles(chunkFs);
            for (Path chunkFilePath : chunkFilePaths) {
                try (InputStream readInputStream = Files.newInputStream(chunkFilePath, StandardOpenOption.READ)) {
                    BufferedImage chunkImage = ImageIO.read(readInputStream);
                    chunkImageMap.put(chunkFilePath.toString(), chunkImage);
                }
            }
        } catch (Exception e) {
            LOGGER.log(Level.WARNING, "error mounting zip filesystem " + chunkOoxmlFilePath, e);
        }
        // mount the result zip file system
        try (FileSystem resultFs = FileSystems.newFileSystem(resultOoxmlFilePath, null)) {
            List<Path> resultFilePaths = listChunkableFiles(resultFs);

            for (int i = 0; i < resultFilePaths.size(); i++) {
                Path resultPath = resultFilePaths.get(i);

                /*
				 TRYING TO PRESERVE THE IMAGE METADATA, BUT CODE IS NOT READY YET
				 
				try (InputStream resultImageInputStream = Files.newInputStream(resultPath, StandardOpenOption.READ)) {
					ImageInputStream input = ImageIO.createImageInputStream(resultImageInputStream);

		            Iterator<ImageReader> readers = ImageIO.getImageReaders(input);
		            if (!readers.hasNext()) {
		            	throw new IOException("no image reader for file " + resultPath);
		            }
		            // Read image and metadata
		            ImageReader reader = readers.next();
		            reader.setInput(input);
		            IIOMetadata metadata = reader.getImageMetadata(0);
		            BufferedImage firstChunkImage = reader.read(0);
		            
					BMPReassembler reassembler = new BMPReassembler(totalNumberOfChunks);
					if (alreadyIncorporatedChunkIds != null && alreadyIncorporatedChunkIds.length > 0) {
					reassembler.setPartialResultImage(firstChunkImage, alreadyIncorporatedChunkIds);
					} else {
						reassembler.incorporateChunk(firstChunkImage, resultChunkNumber);						
					}						
					
					BufferedImage nextChunkImage = chunkImageMap.get(resultPath.toString());					
					reassembler.incorporateChunk(nextChunkImage, chunkNumber);
					if (alreadyIncorporatedChunkIds != null && alreadyIncorporatedChunkIds.length + 1 < totalNumberOfChunks) {
						//reassembler.interpolate();
					}
					// overwrite the original result image with the reassembled image
		            // Write image with metadata from original image, to maintain tRNS info
					try (OutputStream writeOutputStream = Files.newOutputStream(resultPath, StandardOpenOption.CREATE, StandardOpenOption.WRITE, StandardOpenOption.TRUNCATE_EXISTING) ) {
						ImageWriter writer = ImageIO.getImageWritersBySuffix(getFileExtension(resultPath)).next();
						ImageOutputStream output = ImageIO.createImageOutputStream(writeOutputStream);
		                writer.setOutput(output);
		                writer.write(new IIOImage(reassembler.getReassembledImage(), Collections.<BufferedImage>emptyList(), metadata));
		            }
		            
		        }
                 */
                // read the result image
                try (InputStream resultImageInputStream = Files.newInputStream(resultPath, StandardOpenOption.READ)) {

                    BMPReassembler reassembler = new BMPReassembler(totalNumberOfChunks);
                    BufferedImage firstChunkImage = ImageIO.read(resultImageInputStream);
                    if (alreadyIncorporatedChunkIds != null && alreadyIncorporatedChunkIds.length > 0) {
                        reassembler.setPartialResultImage(firstChunkImage, alreadyIncorporatedChunkIds);
                    } else {
                        reassembler.incorporateChunk(firstChunkImage, resultChunkNumber);
                    }

                    BufferedImage nextChunkImage = chunkImageMap.get(resultPath.toString());
                    reassembler.incorporateChunk(nextChunkImage, chunkNumber);
                    //if (alreadyIncorporatedChunkIds != null && alreadyIncorporatedChunkIds.length + 1 < totalNumberOfChunks) {
                        reassembler.interpolate();
                    //}
                    // overwrite the original result image with the reassembled image
                    try (OutputStream writeOutputStream = Files.newOutputStream(resultPath, StandardOpenOption.CREATE, StandardOpenOption.WRITE, StandardOpenOption.TRUNCATE_EXISTING)) {
                        ImageIO.write(reassembler.getReassembledImage(), FileUtils.getFileExtension(resultPath.toString()), writeOutputStream);
                    }
                }
            }
        } catch (Exception e) {
            LOGGER.log(Level.WARNING, "error mounting zip filesystem " + resultOoxmlFilePath, e);
        }

    }

    /**
     * Get the Path to the /xxx/media directory within the given Ooxml
     * document's FileSystem (representing a mounted Zip file)
     *
     * @param fs - the FileSystem into the Ooxml document
     * images for the Ooxml document
     * @return - the Path to the /xxx/media directory
     * @throws IOException
     */
    private static Path getMediaDirectoryPath(FileSystem fs) throws IOException {

        Path mediaFolderInsideZipPath = null;
        for (String mediaFolderInsideZip : OOXML_MEDIA_FOLDERS) {
            mediaFolderInsideZipPath = fs.getPath(mediaFolderInsideZip);
            if (Files.exists(mediaFolderInsideZipPath)) {
                break;
            }
        }
        return mediaFolderInsideZipPath;
    }

    /**
     * List all files within the Ooxml document's media directory for which
     * chunking is supported.
     *
     * @param fs - the FileSystem into the Ooxml document
     * images for the Ooxml document
     * @return - a List of file Paths, one for each
     * @throws IOException
     */
    private static List<Path> listChunkableFiles(FileSystem fs) throws IOException {

        List<Path> resultMediaPathList = new ArrayList<Path>();

        Path mediaFolderInsideZipPath = getMediaDirectoryPath(fs);
        // list the contents of the result /media/ folder
        try (DirectoryStream<Path> mediaDirectoryStream = Files.newDirectoryStream(mediaFolderInsideZipPath)) {
            // for every file in the result /media/ folder
            // note: NOT expecting any subfolders
            for (Path mediaFilePath : mediaDirectoryStream) {
                if (BMPUtils.CHUNKABLE_IMAGE_FILE_EXTENSIONS.contains(FileUtils.getFileExtension(mediaFilePath.toString()))) {
                    resultMediaPathList.add(mediaFilePath);
                }
            }
        }

        return resultMediaPathList;
    }


}
