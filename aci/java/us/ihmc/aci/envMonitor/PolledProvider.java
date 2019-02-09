package us.ihmc.aci.envMonitor;

/**
 * EnvironmentalMonitorAdaptor
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @author Niranjan Suri (nsuri@ihmc.us)
 * @version $Revision$
 *          Created on May 17, 2004 at 4:06:38 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public abstract class PolledProvider
{
    public abstract void update();

    public abstract void init() throws Exception;

    void setMonitor (EnvironmentalMonitor em)
    {
        _environmentalMonitor = em;
    }

    protected EnvironmentalMonitor _environmentalMonitor;
}
