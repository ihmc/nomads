/*
 * PersonalizedExternalizable.java
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

import java.io.Externalizable;
import java.io.IOException;
import java.io.ObjectInput;
import java.io.ObjectOutput;

/**
 * Class that actually performs the serialization
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class PersonalizedExternalizable implements Externalizable
{
    public PersonalizedExternalizable()
    {
    }

    PersonalizedExternalizable (Object obj)
    {
        _objData = obj;
    }

    @Override
    public void writeExternal (ObjectOutput out) throws IOException
    {
        if (_objData == null) {
            throw new IOException ("The object to serialize is null");
        }

        out.writeObject (_objData);
    }

    @Override
    public void readExternal (ObjectInput in) throws IOException, ClassNotFoundException
    {
        _objData = in.readObject();
    }

    public Object getDeserializedObject()
    {
        return _objData;
    }

    private Object _objData;
}
