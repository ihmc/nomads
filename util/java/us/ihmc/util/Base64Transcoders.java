/*
 * Base64Transcoders.java
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
import java.io.Serializable;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.lang.String;

public class Base64Transcoders
{
    public Base64Transcoders()
    {
    }
	
    public Serializable convertB64StringToObject (String b64String)
        throws ClassNotFoundException, IOException, Base64FormatException
    {
        // Convert from Base64 string to byte array
        ByteArrayInputStream b64In = new ByteArrayInputStream (b64String.getBytes ("ISO-8859-1"));
        ByteArrayOutputStream bOut = new ByteArrayOutputStream();
        Base64Decoder b64conv = new Base64Decoder (b64In, bOut);
        b64conv.process();

        // Convert from byte array to serializable object
        ByteArrayInputStream bIn = new ByteArrayInputStream (bOut.toByteArray());
        ObjectInputStream In = new ObjectInputStream (bIn);
        Object o = In.readObject();
        return (Serializable) o;
    }

    /** 
     * In the event that a classLoader is specified, utilize an subclass of the 
     * ObjectInputStream so that its resolveClass method can be overridden.
     * 
     * @param b64String
     * @param loader
     * @return
     * @throws ClassNotFoundException
     * @throws IOException
     * @throws Base64FormatException
     */
    public Serializable convertB64StringToObject (String b64String, ClassLoader loader)
        throws ClassNotFoundException, IOException, Base64FormatException
    {
        // Convert from Base64 string to byte array
        ByteArrayInputStream b64In = new ByteArrayInputStream (b64String.getBytes ("ISO-8859-1"));
        ByteArrayOutputStream bOut = new ByteArrayOutputStream();
        Base64Decoder b64conv = new Base64Decoder (b64In, bOut);
        b64conv.process();

        // Convert from byte array to serializable object
        ByteArrayInputStream bIn = new ByteArrayInputStream (bOut.toByteArray());
        CustomObjectInputStream In = new CustomObjectInputStream (bIn, loader);
        Object o = In.readObject();
        In.close();
        return (Serializable) o;
    }
	
    public static byte[] convertB64StringToByteArray (String b64String)
        throws IOException, Base64FormatException
    {
        // Convert from Base64 string to byte array
        ByteArrayInputStream b64In = new ByteArrayInputStream (b64String.getBytes ("ISO-8859-1"));
        ByteArrayOutputStream bOut = new ByteArrayOutputStream();
        Base64Decoder b64conv = new Base64Decoder (b64In, bOut);
        b64conv.process();
        return bOut.toByteArray();
    }

    /**
     * Converts a serializable object to a Base64-encoded String.
     */
    public String convertObjectToB64String (Serializable o)
        throws IOException
    {
        // Convert the object to a byte array
        ByteArrayOutputStream bOut = new ByteArrayOutputStream();
        ObjectOutputStream oOut = new ObjectOutputStream (bOut);
        oOut.writeObject (o);
        oOut.flush();
        oOut.close();

        // Convert the binary encoded byte array to a Base64 encoded byte array
        ByteArrayInputStream bIn = new ByteArrayInputStream (bOut.toByteArray());
        ByteArrayOutputStream b64Out = new ByteArrayOutputStream();
        Base64Encoder b64conv = new Base64Encoder (bIn, b64Out);
        b64conv.processWithoutNewlines();
        return new String (b64Out.toByteArray(),"ISO-8859-1");
    }

    /**
     * Converts a binary encoded byte array to a Base64-encoded String.
     */
    public static String convertByteArrayToB64String (byte[] byteArray)
        throws IOException
    {
        // Convert the binary encoded byte array to a Base64 encoded byte array
        ByteArrayInputStream bIn = new ByteArrayInputStream (byteArray);
        ByteArrayOutputStream b64Out = new ByteArrayOutputStream();
        Base64Encoder b64conv = new Base64Encoder (bIn, b64Out);
        b64conv.processWithoutNewlines();
        return new String (b64Out.toByteArray(),"ISO-8859-1");
    }
}
