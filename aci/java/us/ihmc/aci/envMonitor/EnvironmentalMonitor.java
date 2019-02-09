package us.ihmc.aci.envMonitor;

import us.ihmc.util.ConfigLoader;
import us.ihmc.aci.AttributeList;

import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;

/**
 * EnvironmentalMonitor
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @author Niranjan Suri (nsuri@ihmc.us)
 * @version $Revision$
 *          Created on May 17, 2004 at 4:06:38 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class EnvironmentalMonitor implements Runnable
{
    public EnvironmentalMonitor(String nodeName)
    {
        _nodeName = nodeName;
        _updateInterval = 5000;    // Default - 5 seconds
        _changeListeners = new Vector();
        _asyncProviders = new Vector();
        _polledProviders = new Vector();
    }

    public void init()
    {
        debugMsg("loading Monitor Providers:\n");
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        for (int i=0;; i++) {
            try {
                String sPropName = "aci.monitor.provider" + i;
                String sprop = cloader.getProperty (sPropName);
                if (sprop != null) {
                    debugMsg (" (loading): "  + sprop);
                    Class clz = Class.forName(sprop);
                    Object obj = clz.newInstance();
                    if (obj instanceof PolledProvider) {
                        ((PolledProvider) obj).setMonitor(this);
                        ((PolledProvider) obj).init();
                        _polledProviders.addElement((PolledProvider) obj);
                    }
                    else if (obj instanceof AsyncProvider) {
                        ((AsyncProvider) obj).setMonitor(this);
                        ((AsyncProvider) obj).init();
                        _asyncProviders.addElement((AsyncProvider) obj);
                        ((AsyncProvider) obj).start();
                    }
                } else  {
                    break;
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
    
    /**
     * If this instance is started in a separate thread, then this
     * method will periodically cycle through all the PolledProviders
     * calling their update method
     */
    public void run()
    {
        while (true) {
            update();
            try {
                Thread.sleep (_updateInterval);
            }
            catch (InterruptedException e) {}
        }
    }

    public void update()
    {
        for (Enumeration e = _polledProviders.elements(); e.hasMoreElements();) {
            PolledProvider pp = (PolledProvider) e.nextElement();
            pp.update();
        }
    }

    public synchronized void addChangeListener (EnvironmentalChangeListener ecl)
    {
        if (!_changeListeners.contains(ecl)) {
            debugMsg ("Adding Change Listener");
            _changeListeners.addElement(ecl);
        }
    }

    public synchronized void removeChangeListener (EnvironmentalChangeListener ecl)
    {
        _changeListeners.removeElement(ecl);
    }

    public synchronized void updateValue (String attrName, Object value)
    {
        debugMsg ("Got Update from a provider (" + attrName+ " : " + value.toString() + ")");
        for (Enumeration en = _changeListeners.elements(); en.hasMoreElements();) {
            EnvironmentalChangeListener ecl = (EnvironmentalChangeListener) en.nextElement();
            try {
                ecl.attributeValueChanged (attrName, value);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public synchronized void updateValues (Hashtable attributes)
    {
        debugMsg ("Got Hashtable Update from provider");
        if (_debug) {
            Enumeration en = attributes.keys();
            while (en.hasMoreElements()) {
                String skey = (String) en.nextElement();
                Object obj = attributes.get(skey);
                System.out.println("\t" + skey + " : " + obj.toString());
            }
            System.out.println("");
        }

        for (Enumeration en = _changeListeners.elements(); en.hasMoreElements();) {
            EnvironmentalChangeListener ecl = (EnvironmentalChangeListener) en.nextElement();
            try {
                debugMsg ("sent hashtbable to listener");
                ecl.attributeValuesChanged (attributes);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public String getNodeName ()
    {
        return (_nodeName);
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[Environ.Monitor] " + msg);
        }
    }

    private String _nodeName;
    private boolean _debug = true;
    private int _updateInterval;
    private Vector _changeListeners;
    private Vector _asyncProviders;
    private Vector _polledProviders;

}
