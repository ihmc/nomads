package us.ihmc.chunking.android;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Environment;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Logger;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.BMPUtils;
import us.ihmc.chunking.FileUtils;

import static us.ihmc.chunking.FileUtils.getFileExtension;


public class OoxmlChunkUtils {

    private static final Logger LOGGER = Logger.getLogger(OoxmlChunkUtils.class.getName());

    // all of the root folder names within supported Ooxml documents 
    private static final String[] OOXML_MEDIA_FOLDER_ARRAY = {"ppt/media/", "word/media/", "Pictures/"};
    private static final List<String> OOXML_MEDIA_FOLDERS = Arrays.asList(OOXML_MEDIA_FOLDER_ARRAY);

    // 4MB buffer
    private static final byte[] BUFFER = new byte[4096 * 1024];

    /**
     * Combine 2 or more Open XML document 'chunks'.
     * Each chunk that is added results in increasing the resolution of the images (JPEG, PNG)
     * embedded within the document. The chunks must have all originated from the same source document
     * where that source was fragmented into 
     *
     * @param chunks - a collection of Open XML document (e.g. docx, pptx) chunks to be combined.
     * @param compressionQuality - how much to compress the resulting images
     *  between 0 (small size) and 100 (high quality) 
     * @return - byte array containg the resulting Open XML document containing higher-resolution images.
     *
     */
    public static byte[] reassembleImages(Collection<ChunkWrapper> chunks, byte compressionQuality) throws IOException {


        ByteArrayOutputStream resultBytes = new ByteArrayOutputStream();

        Iterator<ChunkWrapper> chunkIter = chunks.iterator();

        ChunkWrapper firstChunk = null;
        ZipInputStream firstChunkInputStream = null;
        HashMap<ChunkWrapper, ZipInputStream> otherChunksToZipInputStreams = new HashMap<>();

        String mediaFolderInsideZipPath = null;

        boolean first = true;

        while (chunkIter.hasNext()) {
            ChunkWrapper chunk = chunkIter.next();
            ZipInputStream chunkZipInputStream = new ZipInputStream(new ByteArrayInputStream(chunk.getData()));


            if (first) {
                first = false;
                firstChunk = chunk;
                firstChunkInputStream = chunkZipInputStream;
                mediaFolderInsideZipPath = getMediaDirectoryPath(chunk);
                if (mediaFolderInsideZipPath == null) {
                    throw new IOException("failed to find the media folder (e.g. ppt/media, word/media, Pictures) that contains the images in the ooxml chunk: " + chunk);
                }
            }
            else {
                otherChunksToZipInputStreams.put(chunk, chunkZipInputStream);
            }

        }
        ZipOutputStream resultOutputStream = new ZipOutputStream(resultBytes);


        // list the contents of the /media/ folder
        ZipEntry firstChunkZipEntry = firstChunkInputStream.getNextEntry();
        while (firstChunkZipEntry != null) {

            if (firstChunkZipEntry.getName().startsWith(mediaFolderInsideZipPath) && BMPUtils.CHUNKABLE_IMAGE_FILE_EXTENSIONS.contains(getFileExtension(firstChunkZipEntry.getName()))) {

                // for every file in the /media/ folder
                // note: NOT expecting any subfolders

                LOGGER.info("Reassembling image: " + firstChunkZipEntry.getName());

                BMPReassembler reassembler = new BMPReassembler(firstChunk.getTotalNumberOfChunks());
                BufferedImage chunkImage = ImageIO.read(firstChunkInputStream);
                reassembler.incorporateChunk(chunkImage, firstChunk.getChunkId());

                    for (ChunkWrapper otherChunk : otherChunksToZipInputStreams.keySet()) {
                    ZipInputStream otherChunkInputStream = otherChunksToZipInputStreams.get(otherChunk);
                    ZipEntry otherChunkZipEntry = otherChunkInputStream.getNextEntry();
                    BufferedImage otherChunkImage = ImageIO.read(otherChunkInputStream);
                    reassembler.incorporateChunk(otherChunkImage, otherChunk.getChunkId());
                }

                if (otherChunksToZipInputStreams.size() < firstChunk.getTotalNumberOfChunks() - 1) {
                    reassembler.interpolate();
                }

                resultOutputStream.putNextEntry(new ZipEntry(firstChunkZipEntry.getName()));
                reassembler.getReassembledImage().write(getCompressFormat(firstChunkZipEntry.getName()), compressionQuality, resultOutputStream);
                resultOutputStream.closeEntry();

            } else {
                // not in the media folder or a supported image type, so just copy to the temp zip file
                resultOutputStream.putNextEntry(new ZipEntry(firstChunkZipEntry.getName()));
                int len;
                while ((len = firstChunkInputStream.read(BUFFER)) > 0) {
                    resultOutputStream.write(BUFFER, 0, len);
                }
                for (ChunkWrapper otherChunk : otherChunksToZipInputStreams.keySet()) {
                    ZipInputStream otherChunkInputStream = otherChunksToZipInputStreams.get(otherChunk);
                    ZipEntry otherChunkZipEntry = otherChunkInputStream.getNextEntry();
                }
                resultOutputStream.closeEntry();

            }
            firstChunkZipEntry = firstChunkInputStream.getNextEntry();
        }
        resultOutputStream.close();

        firstChunkInputStream.close();
        for (ChunkWrapper otherChunk : otherChunksToZipInputStreams.keySet()) {
            ZipInputStream otherChunkInputStream = otherChunksToZipInputStreams.get(otherChunk);
            otherChunkInputStream.close();
        }

        return resultBytes.toByteArray();
    }

    /**
     * Get the compression format of an image based on the file extension.
     * 
     * @param filePathStr - the path to the image, including the file name and extension.
     * @return - Bitmap.CompressFormat either JPEG or PNG, default is JPEG
     */
    private static Bitmap.CompressFormat getCompressFormat(String filePathStr) {
        String fileExtension = FileUtils.getFileExtension(filePathStr);
        if (fileExtension.toLowerCase().equals("jpg") || fileExtension.toLowerCase().equals("jpeg")) {
            return Bitmap.CompressFormat.JPEG;
        }
        if (fileExtension.toLowerCase().equals("png")) {
            return Bitmap.CompressFormat.PNG;
        }
        return Bitmap.CompressFormat.JPEG;
    }
    
    /**
     * Get the Path to the /xxx/media directory within the given Ooxml
     * document's Zip file
     *
     * @param zipFile - the ZipFile of the Ooxml document
     * @return - the Path to the /xxx/media or /Pictures directory within the given ZipFile
     * @throws IOException
     */
    private static String getMediaDirectoryPath(ZipFile zipFile) throws IOException {

        String mediaFolderInsideZipPath = null;
        for (String mediaFolderInsideZip : OOXML_MEDIA_FOLDERS) {
            ZipEntry zipEntry = zipFile.getEntry(mediaFolderInsideZip);
            if (zipEntry != null) {
                mediaFolderInsideZipPath = zipEntry.getName();
                break;
            }
        }
        return mediaFolderInsideZipPath;
    }

    /**
     * Get the Path to the /xxx/media or /Pictures directory within the Open XML
     * document's Zip file within the given ChunkWrapper's data.
     *
     * @param chunkWrapper - a chunk containing an OpenXML document (zip file) as its data
     * @return - the Path to the /xxx/media or /Pictures directory within the given ZipFile
     * @throws IOException
     */    
    private static String getMediaDirectoryPath(ChunkWrapper chunkWrapper) throws IOException {
        String mediaFolderInsideZipPath = null;
        try(ZipInputStream chunkInputStream = new ZipInputStream(new ByteArrayInputStream(chunkWrapper.getData()))) {
            ZipEntry zipEntry = chunkInputStream.getNextEntry();
            while (zipEntry != null) {
                for (String possibleMediaFolder : OOXML_MEDIA_FOLDERS) {
                    if (zipEntry.getName().startsWith(possibleMediaFolder)) {
                        mediaFolderInsideZipPath = possibleMediaFolder;
                        return mediaFolderInsideZipPath;
                    }
                }
                zipEntry = chunkInputStream.getNextEntry();
            }
        }

        return mediaFolderInsideZipPath;
    }


    /**
     * Read the contents of the file specified by the given file path string.
     * The file path may be fully specified or relative to the application 'assets' directory.
     * 
     * @param filePathString - the path to the file to read
     * @param appContext - the android application Context, used to access the assets directory
     * @return the bytes read from the given file path
     * 
     * @throws FileNotFoundException - if the file cannot be found
     * @throws IOException - if problems are encountered accessing or reading the file
     */
    public static byte[] readFileContents(String filePathString, Context appContext) throws FileNotFoundException, IOException{
        byte[] bytes = null;
        File file = new File(filePathString);
        if (file.exists()) {
            int size = (int) file.length();
            bytes = new byte[size];

            try (BufferedInputStream buf = new BufferedInputStream(new FileInputStream(file))) {
                buf.read(bytes, 0, bytes.length);
            }
        } else if (appContext != null) {
            InputStream assetInputStream = appContext.getAssets().open(filePathString);
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[1024];
            int len;
            while ((len = assetInputStream.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
            bytes = out.toByteArray();

        }
        return bytes;
    }


    /**
     * Write the given bytes to a file at the specified path.
     * The file path may be fully specified or relative to the application 'assets' directory.
     * If the file does not exist, it will be 
     * 
     * @param filePathString - the path to the file to read, either absolute or relative to the applicatio assets
     * @param fileContents - the bytes to write to the file
     * @param appContext - the android application Context, used to access the assets directory
     * @throws FileNotFoundException
     * @throws IOException
     */
    public static void writeFileContents(String filePathString, byte[] fileContents, Context appContext) throws FileNotFoundException, IOException {

        File file = null;

        if (appContext != null) {
            file = new File(appContext.getExternalFilesDir(Environment.DIRECTORY_PICTURES), filePathString);
        } else {
            file = new File(filePathString);
        }
        if (file.exists()) {
            file.delete();
        }
        try (BufferedOutputStream buf = new BufferedOutputStream(new FileOutputStream(file))) {
            buf.write(fileContents);
        }
    }
}
