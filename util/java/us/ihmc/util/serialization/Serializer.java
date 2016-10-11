/*
 * Serializer.java
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

package us.ihmc.util.serialization;

import java.io.IOException;

/**
 * Exposes the method every serializer must implement
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public interface Serializer
{
    /**
     * Register a new class with the <code>Kyro</code> object if necessary
     * @param c <code>Class</code> to be registered
     */
    public void register (Class c);

    /**
     * Serialize the given object
     * @param obj object to be serialized
     * @return the byte array representation of the object
     * @throws SerializationException if errors occurs in the serialization process
     */
    public byte[] serialize (Object obj) throws SerializationException;

    /**
     * Deserialize the byte array
     * @param buffer byte array to be deserialized
     * @param objClass <code>Class</code> the final object needs to be cast to
     * @param <T>
     * @return the instance of the deserialized object
     * @throws SerializationException if errors occurs in the deserialization process
     */
    public <T> T deserialize (byte[] buffer, Class<T> objClass) throws SerializationException;
}
