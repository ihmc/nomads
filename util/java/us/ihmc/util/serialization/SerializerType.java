/*
 * SerializerType.java
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

/**
 * <code>SerializerFactory</code> type
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public enum SerializerType
{
    JAVA ("java"),
    EXTERNALIZABLE ("externalizable"),
    KRYO ("kryo");

    /**
     * Constructor for <code>SerializerType</code>
     * @param type serializer type
     */
    SerializerType (String type)
    {
        _type = type;
    }

    /**
     * Returns the type of serializer
     * @return the <code>String</code> representation of the serializer type
     */
    public final String toString()
    {
        return _type;
    }

    /**
     * Returns the enum constant with the specified name.
     * </p>
     * The name must match exactly an identifier used to declare an enum constant in this type
     * @param name the name of the constant to return
     * @return the enum constant with the specified name
     * @throws SerializationException if <code>name</code> is null or if the specified enum type has no constant with
     * the specified name
     */
    public static SerializerType getEnum (String name) throws SerializationException
    {
        if (name == null) {
            throw new SerializationException ("The enum name can not be null");
        }
        for (SerializerType st : SerializerType.values()) {
            if (name.equals (st.toString())) {
                return st;
            }
        }
        throw new SerializationException ("No enum constant found with name " + name);
    }

    private final String _type;
}
