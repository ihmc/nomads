package us.ihmc.aci.gss;

import us.ihmc.aci.envMonitor.EnvironmentalChangeListener;

import java.util.Hashtable;

/**
 * EnvironmentalMonitorAdaptor
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on May 17, 2004 at 4:06:38 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class EnvironmentalMonitorAdaptor implements EnvironmentalChangeListener
{
    public EnvironmentalMonitorAdaptor (String envName)
    {
        _envName = envName;
    }

    public void attributeValueChanged (String attrName, Object value)
    {

    }

    public void attributeValuesChanged(Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    private String _envName;
}
