package us.ihmc.util;

import java.io.*;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Provides utilities to zip/unzip files
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class ZIPFileUtils
{
    /**
     * Unzips <code>zipFile</code>
     * @param zipFileName zip file instance
     * @param outputFolder output folder
     * @throws IOException if problems occur
     */
    public static void unzip (String zipFileName, String outputFolder) throws IOException
    {
        // create output directory if not exists
        File folder = new File (outputFolder);
        if(!folder.exists()) {
            System.out.println ("Creating directory " + outputFolder);
            folder.mkdir();
        }

        ZipFile zipFile = new ZipFile (zipFileName);
        Enumeration<?> enu = zipFile.entries();
        while (enu.hasMoreElements()) {
            ZipEntry zipEntry = (ZipEntry) enu.nextElement();

            String name = zipEntry.getName();
            long size = zipEntry.getSize();
            long compressedSize = zipEntry.getCompressedSize();
            System.out.printf ("name: %-20s | size: %6d | compressed size: %6d\n", name, size, compressedSize);

            String absName = outputFolder + File.separator + name;
            File file = new File (absName);
            if (absName.endsWith ("/")) {
                file.mkdirs();
                continue;
            }

            File parent = file.getParentFile();
            if (parent != null) {
                parent.mkdirs();
            }

            InputStream is = zipFile.getInputStream (zipEntry);
            FileOutputStream fos = new FileOutputStream (file);
            byte[] bytes = new byte[1024];
            int length;
            while ((length = is.read (bytes)) >= 0) {
                fos.write (bytes, 0, length);
            }
            is.close();
            fos.close();

        }
        zipFile.close();
    }
}
