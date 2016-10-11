package us.ihmc.gst.util;

import org.apache.log4j.Logger;

import java.net.MalformedURLException;
import java.net.URL;

/**
 *
 * @author Giacomo Benincasa        (gbenincasa@ihmc.us)
 */

public class GHubUtils 
{
    /**
     * ghub:// with http://hostorip:port/CDMWeb/repo
     * 
     * @param ghubReferredObjectId
     * @return 
     */
    public static URL getURL (String ghubReferredObjectId, String host) throws MalformedURLException
    {
        return getURL (ghubReferredObjectId, host, port);
    }

    public static URL getURL (String ghubReferredObjectId, String host, int port) throws MalformedURLException
    {
        return new URL (ghubReferredObjectId.replace (ghub_protocol + "://",
                                                      "http://" + host + ":" + port + "/CDMWeb/repo"));
    }

    public static String getURLFileName(String url)
    {
        String fileName = url.substring(url.lastIndexOf('/')+1, url.length());
        log.info("URL: " + url + ". File name is: " + fileName);
        return fileName;
    }

    public static String getURLFileNameExtension(String url)
    {
        String urlFileName = getURLFileName(url);
        log.info("Filename of this URL is: " + urlFileName);
        String ext =  urlFileName.substring(urlFileName.lastIndexOf('.'), urlFileName.length());
        log.info("Extension of this URL is : " + ext);

        return ext;
    }

    public static String getURLMimeType (String url)
    {
        String ext = getURLFileNameExtension(url);

        if (ext == null) {
            log.info("Unable to fetch extension of this URL.");
            return null;
        }

        if (ext.contains("jpg") || ext.contains("jpeg")) {
            log.info("Mimetype is: " + "image/jpeg");
            return "image/jpeg";
        }
        else if (ext.contains("png")) {
            log.info("Mimetype is: " + "image/png");
            return "image/png";
        }
        else if (ext.contains("bmp")) {
            log.info("Mimetype is: " + "image/bmp");
            return "image/bmp";
        }
        else if (ext.contains("gif")) {
            log.info("Mimetype is: " + "image/gif");
            return "image/gif";
        }
        else {
            return null;
        }
    }

    private static final int port = 8080;
    private static final String ghub_protocol = "ghub";

    private static final Logger log = Logger.getLogger(GHubUtils.class);
}
