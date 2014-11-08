/*
 * SerializerFactory.java
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

package us.ihmc.util.serialization;

import java.io.IOException;

/**
 * Creates in instance of a specific <code>Serializer</code>
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class SerializerFactory
{
    /**
     * Creates the specific instance of the <code>Serializer</code>
     * @param type <code>Serializer</code> type
     * @return the specific instance of the <code>Serializer</code>
     * @throws SerializationException if the argument <code>type</code> is wrong null or wrong
     */
    public static Serializer getSerializer (SerializerType type) throws SerializationException
    {
        if (type == null) {
            throw new SerializationException ("The argument " + SerializerType.class.getSimpleName() + " is null");
        }

        switch (type) {
            case JAVA:
                return JavaSerializer.getInstance();
            case EXTERNALIZABLE:
                return ExternalizeSerializer.getInstance();
            case KRYO:
                return KryoSerializer.getInstance();
            default:
                throw new SerializationException (SerializerType.class.getSimpleName() + ": " + type + " is wrong");
        }
    }

    public static void main (String[] args)
    {
        String message = "String to serialize";
        byte[] bMessage;
        try {
            System.out.println ("Testing the class " + JavaSerializer.class.getSimpleName());
            bMessage = SerializerFactory.getSerializer (SerializerType.JAVA).serialize (message);
            System.out.println ("Successfully serialized with " + JavaSerializer.class.getSimpleName());
            String jsMessage = SerializerFactory.getSerializer (SerializerType.JAVA).deserialize (bMessage,
                    message.getClass());
            System.out.println ("Successfully deserialized with " + JavaSerializer.class.getSimpleName() + ": " + jsMessage);

            System.out.println ("Testing the class " + ExternalizeSerializer.class.getSimpleName());
            bMessage = SerializerFactory.getSerializer (SerializerType.EXTERNALIZABLE).serialize (message);
            System.out.println ("Successfully serialized with " + ExternalizeSerializer.class.getSimpleName());
            String exMessage = SerializerFactory.getSerializer (SerializerType.EXTERNALIZABLE).deserialize (bMessage,
                    message.getClass());
            System.out.println ("Successfully deserialized with " + ExternalizeSerializer.class.getSimpleName() + ": " + exMessage);

            System.out.println ("Testing the class " + KryoSerializer.class.getSimpleName());
            bMessage = SerializerFactory.getSerializer (SerializerType.KRYO).serialize (message);
            System.out.println ("Successfully serialized with " + ExternalizeSerializer.class.getSimpleName());
            String kMessage = SerializerFactory.getSerializer (SerializerType.KRYO).deserialize (bMessage,
                    message.getClass());
            System.out.println ("Successfully deserialized with " + KryoSerializer.class.getSimpleName() + ": " + kMessage);

        }
        catch (SerializationException e) {
            System.err.println ("Problem in the serialization/deserialiazation");
            e.getStackTrace();
            System.out.println ("\n\nTEST FAILED!!");
        }
    }
}


