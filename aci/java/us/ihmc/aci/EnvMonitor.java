package us.ihmc.aci;

import us.ihmc.aci.gss.GSSClient;
import us.ihmc.aci.gss.EnvironmentalMonitorAdaptor;
import us.ihmc.aci.envMonitor.EnvironmentalMonitor;
import us.ihmc.util.ConfigLoader;
import us.ihmc.ds.fgraph.FGraph;

import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Random;
import java.util.zip.CRC32;

/**
 * EnvMonitor
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Jul 22, 2004 at 4:30:16 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class EnvMonitor implements Runnable
{
    EnvMonitor(String envName, String domainList)
    {
        try {
            _envName = envName;
            _domainList = domainList;
            _gssClient = new GSSClient();
            EnvironmentalMonitorAdaptor envMonAdaptor = new EnvironmentalMonitorAdaptor(_envName);
            _environmentalMonitor = new EnvironmentalMonitor(_envName);
            _environmentalMonitor.addChangeListener(envMonAdaptor);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static EnvMonitor getInstance(String envName, String domainList)
    {
        if (_envMonitor == null) {
            _envMonitor = new EnvMonitor (envName, domainList);
        }
        return (_envMonitor);
    }

    public void run()
    {
        init();
    }

    public void init ()
    {
        try {
            registerEnvironment ();
            while (_running) {
                synchronized (this) {
                    wait(_updateInterval);
                    _environmentalMonitor.update();
                }
            }
            deregisterEnvironment ();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public long getUpdateInterval ()
    {
        return (_updateInterval);
    }

    public void setUpdateInterval (long updateInterval)
    {
        _updateInterval = updateInterval;
    }

    public GSSClient getGSSClient ()
    {
        return _gssClient;
    }

    public FGraph getFGraphClient ()
    {
        return _gssClient.getFGraph();
    }

    public EnvironmentalMonitor getEnvironmentalMonitor ()
    {
        return _environmentalMonitor;
    }

    public String getName()
    {
        return (_envName);
    }

    public void terminate()
    {
        synchronized (this) {
            _running = false;
            notifyAll();
        }
    }

    //-------------------------- Private Methods --------------------------
    private synchronized void registerEnvironment ()
            throws Exception
    {
        if (_envName == null) {
            Random rand = new Random();
            CRC32 crc32 = new CRC32();
            String sencode = rand.nextLong() + "." + System.currentTimeMillis() + "." + rand.nextDouble();
            crc32.update(sencode.getBytes());
            _envName = "EM" + crc32.getValue();
        }
        Hashtable attrib = new Hashtable ();
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        double  transfCost = cloader.getPropertyAsDouble("transformation.cost", 1.0);
        Double DtransfCost = new Double (transfCost);
        attrib.put(AttributeList.NODE_TRANSFORM_COST, DtransfCost);

        double  relayCost = cloader.getPropertyAsDouble("relay.cost", 1.0);
        Double DrelayCost = new Double (relayCost);
        attrib.put(AttributeList.NODE_RELAY_COST, DrelayCost);

        System.out.println("Registering with Attributes");
        Enumeration en = attrib.keys();
        while (en.hasMoreElements()) {
            String skey = (String) en.nextElement();
            Object obj = (Object) attrib.get(skey);
            System.out.println(skey + " > " + obj.toString());
        }

        if (_domainList != null) {
            attrib.put(AttributeList.NODE_DOMAIN, _domainList);
        }
        try {
            _gssClient.registerEnvironmentWithAttributes(_envName, attrib);
        }
        catch (Exception e) {
            throw e;
        }
    }

    private synchronized void deregisterEnvironment ()
            throws Exception
    {
        try {
            Enumeration connAgents = _gssClient.getAgentsConnectedToEnvironment(_envName);
            while (connAgents.hasMoreElements()) {
                String agentID = (String) connAgents.nextElement();
                _gssClient.deregister(agentID);
            }
            _gssClient.deregister(_envName);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static EnvMonitor _envMonitor;
    private boolean _running = false;
    private String _domainList;
    private long _updateInterval = 5000;
    private String _envName;
    private GSSClient _gssClient;
    private EnvironmentalMonitor _environmentalMonitor;

}
