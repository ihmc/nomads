/*
 * Validator.java
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
 */

//Returns true if a string contains valid characters and false if the string contains
//invalid characters.

package us.ihmc.util;

import java.lang.Character;


public class Validator
{
    public Validator()
    {
    }
    
    public static boolean validateIdentifier (String identifier)
    {
        //Invalid characters that are not allowed in the string
        String invalidChars = "=: \t\r\n";
        //Valid characters that are allowed in the string
        String validChars = "!@#$%^&*()-_+{}[]|;'<>?,./~";
        boolean isStringValid = false;
        int ilength = identifier.length();
        for (int i=0; i<ilength; i++) {
            char ci = identifier.charAt(i);
            if (Character.isLetterOrDigit(ci)) {
                isStringValid = true;
                continue;
            }
            else if (validChars.indexOf(ci)>=0) {
                isStringValid = true;
                continue;
            }
            else if (invalidChars.indexOf(ci)>=0) {                
                isStringValid = false;
                break;
            }
        }
        return isStringValid;
    }
}