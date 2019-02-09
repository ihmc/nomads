package us.ihmc.aci.envMonitor;

import java.util.Hashtable;

/**
 * EnvironmentalChangeListener
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @author Niranjan Suri (nsuri@ihmc.us)
 * @version $Revision$
 *          Created on May 17, 2004 at 4:06:38 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public interface EnvironmentalChangeListener
{
    public void attributeValueChanged (String attrName, Object value) throws Exception;

    public void attributeValuesChanged (Hashtable attributes) throws Exception;
}
