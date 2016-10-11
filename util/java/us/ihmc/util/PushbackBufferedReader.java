/*
 * PushbackBufferedReader.java
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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;

class PushbackBufferedReader extends BufferedReader
{
    public PushbackBufferedReader (Reader r)
    {
        super (r);
    }
    
    public PushbackBufferedReader (Reader r, int bufSize)
    {
        super (r, bufSize);
    }
    
    public String readLine()
        throws IOException
    {
        if (_pushedBackLine == null) {
            return super.readLine();
        }
        else {
            String line = _pushedBackLine;
            _pushedBackLine = null;
            return line;
        }
    }

    public void pushBackLine (String line)
        throws IOException
    {
        if (_pushedBackLine == null) {
            _pushedBackLine = line;
        }
        else {
            throw new IOException ("pushback buffer full");
        }
    }
    
    private String _pushedBackLine;
}
