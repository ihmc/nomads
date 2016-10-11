/*
 * StringUtil.java
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

import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.util.ArrayList;

public class StringUtil
{
    public StringUtil()
    {
    }

    /**
     * modify by @author: Maggie Breedy on 05/18/04
     * 
     * Determines if provided string has purely numeric content or not.
     * <p>
     * @param potentialNumber string to be examined to see if it is numeric in nature.
     * @return boolean -true if string is purely numeric
     */
    public static boolean isNumeric (String potentialNumber)
    {
        try {
            Integer.parseInt (potentialNumber);           
            return (true);
        }
        catch (NumberFormatException nfe){
            return (false);
        }
    }

    public String decodeString (String sourceString)
    {
        StringBuffer decodedString = new StringBuffer (sourceString.length());
        int i = 0;
        while (i < sourceString.length()) {
            if (sourceString.charAt(i) == '%') {
                if ((sourceString.charAt(i+1) == '2') && (sourceString.charAt(i+2) == '0')) {
                    decodedString.append (' ');
                    i += 3;
                }
                else if ((sourceString.charAt(i+1) == '2') && (sourceString.charAt(i+2) == '5')) {
                    decodedString.append ('%');
                    i += 3;
                }
            }
            else {
                decodedString.append (sourceString.charAt(i));
                i++;
            }
        }
        return (decodedString.toString());
    }
    
    /**
     * Check for a blank or % characters in a string and replace them with the ASCII
     * symbols. 
     */
    public String encodeString (String str)
    {
        String encodedStr = replaceAll (str, "%", "%25");
        encodedStr = replaceAll (encodedStr, " ", "%20");
        return encodedStr;
    }
    
    /**
     * Replace string patterns. (Tom Cowin 05/21/04)
     */
    public String replaceAll (String searchString, String searchPattern, String replacePattern)
    {
        String result = searchString;
        if (result != null && result.length() > 0) {
            int primaryIndex = 0;
            int secondaryIndex = 0;
            while (true) {
                primaryIndex = result.indexOf (searchPattern, secondaryIndex);
                if (primaryIndex != -1) {
                    result = result.substring (0, primaryIndex) + replacePattern + 
                             result.substring (primaryIndex + searchPattern.length());
                    secondaryIndex = primaryIndex + replacePattern.length();
                }
                else
                    break;
            }
        }
        return result;
    }

    /**
     * Splits a String composed of "separator" separated words in an array
     * of Strings. The String.split() could do this but it's not
     * available in java 1.2.
     * 
     * @author Matteo Rebeschini <mrebeschini@ihmc.us>
     */
    @SuppressWarnings({ "unchecked", "rawtypes" })
    public String[] split(String s, char separator)
    {
    	if (s.equals("")) {
    		return new String[0];
    	}

    	ArrayList str = new ArrayList();

    	char c = 'x';
    	int i = 0;
            while (i < s.length()) {
                c = s.charAt(i++);

                if (c == separator) {
                str.add(s.substring(0, i-1).trim());
                        s = s.substring(i, s.length()).trim();
                i = 0;
            }
    	}
        str.add(s);
        str.trimToSize();

        //Convert the ArrayList in a Array of Strings
        String[] stringArray = new String[str.size()];	
        for (int j=0; j< str.size(); j++) {
            stringArray[j] = (String) str.get(j);
        }
    	return stringArray;
    }

    /**
     * Prints the Throwable object's stack trace into a string, so that it can
     * be safely printed without the possibility of throwing an exception
     *
     * @author Giacomo Benincasa <gbenincasa@ihmc.us>
     */
    public static String getStackTraceAsString (Throwable t)
    {
        final Writer sw = new StringWriter();
        t.printStackTrace(new PrintWriter(sw));
        return sw.toString();
    }
}

