/*
 * BrowserLauncher.java
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

import java.io.IOException;

/**
 * Class containing a static method displayURL() for displaying a URL using
 * the system's browser.
 *
 * @author Matteo Rebeschini <mrebeschini@ihmc.us>
 * @version $Revision$ $Date$
 */
public class BrowserLauncher
{
    /**
     * Display a file in the system browser.  If you want to display a
     * file, you must include the absolute path name.
     *
     * @param url the file's url (the url must start with either "http://" or
     * "file://").
     * @return <code>true</code> if succeeded to open the system's browser
     *         <code>false</code> in case of failure
     */
    public static boolean displayURL (String url)
    {
        try {
            String cmd = getLaunchCommand (url);
            if (cmd != null) {
                Runtime.getRuntime().exec (getLaunchCommand (url));
            }
            else {
                return false;
            }
        }
        catch (IOException ioe) {
            return false;
        }

        return true;
    }

    /**
     * Utility method for constructing the command used for opening the
     * system browser (dependent on the OS).
     *
     * @param url URL to display
     * @return System Command
     */
    private static String getLaunchCommand (String url)
    {
       if (System.getProperty ("os.name").startsWith ("Windows")) {
           return "rundll32 url.dll,FileProtocolHandler " + url;
        }
        else if (System.getProperty ("os.name").equals ("Linux")) {
            try {
                Process p = Runtime.getRuntime().exec ("which mozilla-firefox");
                if (p.waitFor() == 0) {
                    return  "mozilla-firefox " + url;

                }
            }
            catch (IOException ioe) {}
            catch (InterruptedException ie) {}

            try {
                Process p = Runtime.getRuntime().exec ("which konqueror");
                if (p.waitFor() == 0) {
                    return "konqueror " + url;
                }
            }
            catch (IOException ioe) {}
            catch (InterruptedException ie) {}

            try {
                Process p = Runtime.getRuntime().exec ("which netscape");
                if (p.waitFor() == 0){
                    return  "netscape -remote openURL(" + url + ")";
                }
            }
            catch (IOException ioe) {}
            catch (InterruptedException ie) {}
        }
        else if (System.getProperty("os.name").startsWith("Mac")) {
            try {
                Process p = Runtime.getRuntime().exec("which open");
                if (p.waitFor() == 0) {
                    return "open " + url;
                }
            }
            catch (IOException e) {}
            catch (InterruptedException e) {}
        }

           return null;
    }

    /**
     * Usage example
     */
    public static void main (String[] args)
    {
        if (args.length == 1) {
            displayURL (args[0]);
        }
        else {
            System.err.println ("Usage: java BrowserLauncher <URL>");
        }
    }
}
