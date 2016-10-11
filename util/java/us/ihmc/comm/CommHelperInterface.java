/**
 * CommHelperInterface.java
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

package us.ihmc.comm;

/**
 * CommHelperInterface.java
 * <p/>
 * Interface <code>CommHelperInterface</code> defines common methods for CommHelper instances.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface CommHelperInterface
{
    public void sendLine (String buf) throws CommException;

    public void sendBlob (byte[] buf) throws CommException;

    public void sendBlob (byte[] buf, int off, int len) throws CommException;

    public String receiveLine () throws CommException;

    public String receiveRemainingLine (String startsWith) throws CommException, ProtocolException;

    public String[] receiveRemainingParsed (String startsWith) throws CommException, ProtocolException;

    public void receiveMatch (String matchWith) throws CommException, ProtocolException;

    public byte[] receiveBlob (int bufSize) throws CommException;

    public void write8 (byte val) throws CommException;

    public void write32 (int i32Val) throws CommException;

    public byte read8 () throws CommException, ProtocolException;

    public int read32 () throws CommException, ProtocolException;

    public String[] receiveParsed () throws CommException;

    public String[] receiveParsedSpecific (String format) throws CommException, ProtocolException;

    public void closeConnection ();
}
