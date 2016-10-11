/*
 * MIMEUtils.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.util;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.ResourceBundle;
import java.util.regex.Pattern;
import javax.activation.MimetypesFileTypeMap;

/**
 *
 * @author Giacomo Benincasa        (gbenincasa@ihmc.us)
 */
public class MIMEUtils
{
    public static final Pattern MSDOC = Pattern.compile ("application/.*\\.openxmlformats-officedocument\\..*");
    public static final Pattern MSDOC_OLD = Pattern.compile ("application/.*(\\.)*((.*excel)|(.*msword)|(.*powerpoint))");
    public static final Pattern OPENDOC = Pattern.compile ("application/.*\\.opendocument\\..*");

    private static final String MIME_TYPE_FILE = "/us/ihmc/util/mime/MIMETypes.properties";
    private static final String MIME_TYPE_FILEP = "us.ihmc.util.mime.MIMETypes";

    // File extension -> MIME Type
    // Load custom mime-types file (since its location appears not to be that standard...
    private static final MimetypesFileTypeMap _mimeTypes = new MimetypesFileTypeMap (MIMEUtils.class.getResourceAsStream (MIME_TYPE_FILE));

    // MIME Type -> File extension
    private static final HashMap<String, String> _fileExtensions = new HashMap<String, String>();
    static {
        ResourceBundle bundle = ResourceBundle.getBundle (MIME_TYPE_FILEP);
        for (Enumeration e = bundle.getKeys(); e.hasMoreElements(); ) {
            String type = (String) e.nextElement();
            String[] extensions = (bundle.getString (type)).split ("s+");
            if (_fileExtensions.get (type) == null) {
                _fileExtensions.put (type, extensions[0]);
            }
        }
    }

    public static String getMIMEType (File file) throws Exception
    {
        if (file.exists()) {
            String type = _mimeTypes.getContentType (file);
            if (type == null) {
                throw new Exception ("No MIME type for file " + file.getAbsolutePath());
            }
            return type;
        }
        throw new FileNotFoundException ("File " + file.getAbsolutePath() + " not found");
    }

    /**
     * Given the MIME type, it returns a possible file extension (or null if a
     * mapping was not found)
     */
    public static String getExtension (String mimeType)
    {
        if (mimeType == null || mimeType.length() == 0) {
            return null;
        }
        return _fileExtensions.get (mimeType);
    }

    /**
     * Return true if the MIMI type matches MSDOC or MSDOC_OLD or OPENDOC
     */
    public static boolean isDocumentType (String mimeType)
    {
        if (MSDOC.matcher(mimeType).matches() ||
            MSDOC_OLD.matcher(mimeType).matches() ||
            OPENDOC.matcher(mimeType).matches()) {
            return true;
        }
        return false;
    }

    public static boolean isText (String mimeType)
    {
        if (mimeType.contains("wordprocessing") ||
            mimeType.contains("opendocument.text") ||
            mimeType.contains("msword")) {
            return true;
        }
        return false;
    }

    public static boolean isSpreadsheet (String mimeType)
    {
        if (mimeType.endsWith (".sheet") ||
            mimeType.contains("spreadsheet") ||
            mimeType.contains("excel")) {
            return true;
        }
        return false;
    }

    public static boolean isPresentation (String mimeType)
    {
        if (mimeType.contains("presentation") ||
            mimeType.contains("ms-powerpoint") ||
            mimeType.contains("slideshow")) {
            return true;
        }
        return false;
    }

    public static boolean isImage (String mimeType)
    {
        if (mimeType.contains("image") ||
            mimeType.contains("IMAGE")) {
            return true;
        }
        return false;
    }

    public static void main (String[] args) throws Exception
    {
        for (String file : args) {
            System.out.println ("The MIME type of file " + file + " is: " + getMIMEType (new File (file)));
        }
        ArrayList<String> strings = new ArrayList<String>();

        strings.add ("application/vnd.openxmlformats-officedocument.wordprocessingml.document");
        strings.add ("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
        strings.add ("application/vnd.openxmlformats-officedocument.presentationml.presentation");
        strings.add ("application/vnd.openxmlformats-officedocument.presentationml.slideshow");
        for (String string : strings) {
            if (!MSDOC.matcher(string).matches()) {
                System.out.println (string + " does not match" + " MSDOC");
            }
        }

        strings.clear();
        strings.add ("application/msword");
        strings.add ("application/vnd.ms-excel");
        strings.add ("application/vnd.ms-powerpoint");
        for (String string : strings) {
            if (!MSDOC_OLD.matcher(string).matches()) {
                System.out.println (string + " does not match" + " MSDOC_OLD");
            }
        }

        strings.clear();
        strings.add ("application/vnd.oasis.opendocument.text");
        strings.add ("application/vnd.oasis.opendocument.spreadsheet");
        strings.add ("application/vnd.oasis.opendocument.presentation");
        for (String string : strings) {
            if (!OPENDOC.matcher(string).matches()) {
                System.out.println (string + " does not match" + " OPENDOC");
            }
        }

    }
}
