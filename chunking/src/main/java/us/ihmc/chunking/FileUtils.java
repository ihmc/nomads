package us.ihmc.chunking;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by lbunch on 4/19/17.
 */

public class FileUtils {

    public static final String MIME_TYPE_PPTX = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    public static final String MIME_TYPE_DOCX = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    public static final String MIME_TYPE_XLSX = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    /*	Ext 	MIME Type
	.doc	application/msword
	.dot	application/msword
	.docx	application/vnd.openxmlformats-officedocument.wordprocessingml.document
	.dotx	application/vnd.openxmlformats-officedocument.wordprocessingml.template
	.docm	application/vnd.ms-word.document.macroEnabled.12
	.dotm	application/vnd.ms-word.template.macroEnabled.12
	.xls	application/vnd.ms-excel
	.xlt	application/vnd.ms-excel
	.xla	application/vnd.ms-excel
	.xlsx	application/vnd.openxmlformats-officedocument.spreadsheetml.sheet
	.xltx	application/vnd.openxmlformats-officedocument.spreadsheetml.template
	.xlsm	application/vnd.ms-excel.sheet.macroEnabled.12
	.xltm	application/vnd.ms-excel.template.macroEnabled.12
	.xlam	application/vnd.ms-excel.addin.macroEnabled.12
	.xlsb	application/vnd.ms-excel.sheet.binary.macroEnabled.12
	.ppt	application/vnd.ms-powerpoint
	.pot	application/vnd.ms-powerpoint
	.pps	application/vnd.ms-powerpoint
	.ppa	application/vnd.ms-powerpoint
	.pptx	application/vnd.openxmlformats-officedocument.presentationml.presentation
	.potx	application/vnd.openxmlformats-officedocument.presentationml.template
	.ppsx	application/vnd.openxmlformats-officedocument.presentationml.slideshow
	.ppam	application/vnd.ms-powerpoint.addin.macroEnabled.12
	.pptm	application/vnd.ms-powerpoint.presentation.macroEnabled.12
	.potm	application/vnd.ms-powerpoint.template.macroEnabled.12
	.ppsm	application/vnd.ms-powerpoint.slideshow.macroEnabled.12
     */
    /**
     * Map of file extension to MIME types for the supported Ooxml types
     */
    public static final Map<String, String> FILEEXT_MIMETYPE_MAP = new HashMap<>();
    public static final Map<String, String> MIMETYPE_FILEEXT_MAP = new HashMap<>();

    static {
        FILEEXT_MIMETYPE_MAP.put("pptx", MIME_TYPE_PPTX);
        FILEEXT_MIMETYPE_MAP.put("docx", MIME_TYPE_DOCX);
        FILEEXT_MIMETYPE_MAP.put("xlsx", MIME_TYPE_XLSX);
        FILEEXT_MIMETYPE_MAP.put("odp", MIME_TYPE_PPTX);
        FILEEXT_MIMETYPE_MAP.put("odt", MIME_TYPE_DOCX);

        for (Map.Entry<String, String> e : FILEEXT_MIMETYPE_MAP.entrySet()) {
            MIMETYPE_FILEEXT_MAP.put(e.getValue(), e.getKey());
        }
    }



    public static void copy(String existingFilePathStr, String newFilePathStr) throws FileNotFoundException, IOException {
        InputStream in = new FileInputStream(existingFilePathStr);
        OutputStream out = new FileOutputStream(newFilePathStr);

        // Transfer bytes from in to out
        byte[] buf = new byte[1024];
        int len;
        while ((len = in.read(buf)) > 0) {
            out.write(buf, 0, len);
        }
        in.close();
        out.close();
    }

    /**
     * Get the text excluding the last '.' and everything after it
     *
     * @param filePathStr
     * @return
     */
    public static String getAllExceptExtension(String filePathStr) {
        int dotIdx = filePathStr.lastIndexOf('.');
        if (dotIdx == -1) {
            return filePathStr;
        }
        return filePathStr.substring(0, dotIdx);
    }


    /**
     * Get the text after the last '.' in the file name
     *
     * @param filePathStr
     * @return
     */
    public static String getFileName(String filePathStr) {
        int lastSepartorIdx = filePathStr.lastIndexOf('/');
        if (lastSepartorIdx == -1) {
            lastSepartorIdx = filePathStr.lastIndexOf('\\');
            if (lastSepartorIdx == -1) {
                lastSepartorIdx = filePathStr.lastIndexOf(File.pathSeparatorChar);
                if (lastSepartorIdx == -1) {
                    return filePathStr;
                }
            }
        }
        return filePathStr.substring(lastSepartorIdx + 1);
    }



    /**
     * Get the text after the last '.' in the file name
     *
     * @param filePathStr
     * @return
     */
    public static String getFileExtension(String filePathStr) {
        int dotIdx = filePathStr.lastIndexOf('.');
        if (dotIdx == -1) {
            return "";
        }
        return filePathStr.substring(dotIdx + 1);
    }


    /**
     * Get the MIME type bases on the file extension at the end of the file
     * name.
     *
     * @param fileName - the full file name (including extension, e.g.
     * test.pptx)
     * @return - the MIME type as a String
     */
    public static String getMimeTypeForFile(String fileName) {
        String fileExt = getFileExtension(fileName);
        if (fileExt.length() == 0) {
            return "";
        }
        return FILEEXT_MIMETYPE_MAP.get(fileExt);
    }

    public static String getFileExtensionForMimeType(String mimeType) {
        if (MIMETYPE_FILEEXT_MAP.containsKey(mimeType)) {
            return MIMETYPE_FILEEXT_MAP.get(mimeType);
        }
        return "dat";
    }

    public static Collection<String> getSupportedMimeTypes() {
        return FILEEXT_MIMETYPE_MAP.values();
    }


    public static byte[] readFileContents(String filePathString) throws FileNotFoundException, IOException{
        byte[] bytes = null;
        File file = new File(filePathString);
        if (file.exists()) {
            int size = (int) file.length();
            bytes = new byte[size];

            try (BufferedInputStream buf = new BufferedInputStream(new FileInputStream(file))) {
                buf.read(bytes, 0, bytes.length);
            }
        } else  {

            URL fileURL = FileUtils.class.getClassLoader().getResource(filePathString);
            if (fileURL != null) {
                file = new File(fileURL.getPath());
            } else {
                return null;
            }

        }
        return bytes;
    }



    public static void writeFileContents(String filePathString, byte[] fileContents) throws FileNotFoundException, IOException {
        File file = new File(filePathString);
        if (file.exists()) {
            file.delete();
        }

        try (BufferedOutputStream buf = new BufferedOutputStream(new FileOutputStream(filePathString))) {
            buf.write(fileContents);
        }
    }

}
