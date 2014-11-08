/*
 * URIEncoder.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.util;

public class URIEncoder
{
    /**
     * This specification uses the term URI as defined in [URI] (see also [RFC1630]).
     *  http://www.ietf.org/rfc/rfc1630.txt
     * 
     * Takes all a string's "special characters", characters that are not of
     * type [0-9], [a-z], [A-Z] and converts them to ascii values preceded by a '%'.
     *
     * @param stringWithSpecialChars The string that will have all the special
     *        characters removed.
     * @return String A String that has has all of the containing special
     *         chars replaced with placeholders.
     *
     * @author dwelch
     * @version $Revision: 1.6 $
     */
    public static String removeSpecialChars (String stringWithSpecialChars)
    {
        StringBuffer sb = new StringBuffer();
        char currentChar;
        int isAllowed;

        if (null != stringWithSpecialChars) {
            for (int i = 0; i < stringWithSpecialChars.trim().length(); i++) {
                currentChar = stringWithSpecialChars.charAt (i);
                isAllowed = ALLOWED_CHARACTERS.indexOf (currentChar);
                if (isAllowed >= 0) {
                    sb.append (stringWithSpecialChars.charAt (i));
                }
                else {
                    String hexValue;
                    sb.append ("%");
                    hexValue = Integer.toHexString ((int) currentChar);
                    if (hexValue.length() < 2) {
                        sb.append ("0" + hexValue);
                    }
                    else {
                        sb.append (hexValue);
                    }
                }
            }
            return sb.toString();
        }
        return null;
    }

    /**
     * Takes all a string's "formatted" ascii values and converts them back to
     * "special characters".
     *
     * @param stringwithAsciiChars the string that will have the ASCII char
     *        representations replaced with the real chars.
     *
     * @return String a string that has special characters in it that were
     *         represented with %<hexvalue>
     *
     * @see removeSpecialChars
     *
     */
    public static String includeSpecialChars (String stringWithAsciiChars)
    {
        int stringLength = stringWithAsciiChars.length();
        int j = 0; 
        byte[] byteArray;
        byte[] newByteArray;

        byteArray = stringWithAsciiChars.getBytes();
        newByteArray = stringWithAsciiChars.getBytes();
        for (int init = 0; init < stringLength; init++) {
            newByteArray[init] = 32;   // inserts spaces into the new empty byte array
        }
        for (int i = 0; i < stringLength; i++, j++) {
            if (byteArray[i] != 37) {   // if the hex-encoded character isn't a '%'
                newByteArray[j] = byteArray[i];
            }
            else {
                byte b = toByte (byteArray[i+1], byteArray[i+2]);
                newByteArray[j] = b;
                i += 2;
            }
        }
        String tmp = new String (newByteArray);
        return tmp;
    }

    /**
     * Takes two bytes representing the first and second digit of hex values
     * for a single character and converts them to a single decimal value
     * representation of an ascii value of a charater.
     */
    private static byte toByte (byte left, byte right)
    {
        if (left >= '0' && left <= '9') {
            left = (byte) (left - '0');
        }
        else if (left >= 'a' && left <= 'f') {
            left = (byte) ((left - 'a') + 10);
        }
        else if (left >= 'A' && left < 'F') {
            left = (byte) ((left - 'A') + 10);
        }

        if (right >= '0' && right <= '9') {
            right = (byte) (right - '0');
        }
        else if (right >= 'a' && right <= 'f') {
            right = (byte) ((right -'a') + 10);
        }
        else if (right >= 'A' && right < 'F') {
            right = (byte) ((right - 'A') + 10);
        }
        return (byte) ((left << 4) + right);
    }

    // Contains all of the "non-special" characters that won't be converted
    private static final String ALLOWED_CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
}
