/*
 * MocketCommHelper.java
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

import us.ihmc.comm.CommHelper;
import us.ihmc.io.LineReaderInputStream;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.OutputStreamWriter;

import us.ihmc.mockets.Mocket;

/**
 *
 * @author  nsuri
 */
public class MocketCommHelper extends CommHelper
{
    public MocketCommHelper()
    {
    }

    public boolean init (Mocket m)
    {
        _mocket = m;
        try {
            _bufferedOutputStream = new BufferedOutputStream (m.getOutputStream());
            _outputWriter = new OutputStreamWriter (_bufferedOutputStream);
            BufferedInputStream bis = new BufferedInputStream (m.getInputStream());
            _lineReaderInputStream = new LineReaderInputStream (bis);
        }
        catch (Exception e) {
            System.out.println ("CommHelper: Got exception");
            e.printStackTrace();
            return false;
        }

        return true;
    }

    public void closeConnection()
    {
        try {
            _mocket.close();
        }
        catch (Throwable t) {
            t.printStackTrace();
        }
    }

    private Mocket _mocket;
}
