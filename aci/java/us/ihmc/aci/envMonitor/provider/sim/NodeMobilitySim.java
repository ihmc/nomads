package us.ihmc.aci.envMonitor.provider.sim;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.util.ConfigLoader;
import us.ihmc.aci.AttributeList;
import us.ihmc.ds.fgraph.FGraphClient;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphEventListener;

import java.net.URI;
import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Random;

/**
 * NodeMobilitySim
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Jul 23, 2004 at 12:37:36 AM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class NodeMobilitySim extends AsyncProvider implements FGraphEventListener
{
    public void init()
            throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        String gssServerHost = cloader.getProperty("gss.server.default.host");
        int gssPort = cloader.getPropertyAsInt("gss.server.default.port");
        if (gssServerHost == null) {
            throw new Exception ("Uknown gssServer host - Faild to locate property (gss.server.default.host)");
        }
        _envFilter = new Hashtable();
        _envFilter.put(AttributeList.NODE_TYPE, "ENVIRONMENT");
        _fgraph = FGraph.getClient(new URI("tcp://" + gssServerHost + ":" + gssPort));
    }

    public void run()
    {
        try {
            _envName = _environmentalMonitor.getNodeName();
            _fgraph.addGraphEventListener(this);
            while (_running) {
                try {
                    Thread.sleep(_defaultSleepInterval);
                    calculateNodePosition();
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void calculateNodePosition()
            throws Exception
    {
        double step = _defaultStepSize;
        if (_curXpos != null && _curYpos != null && _targetXpos != null && _targetYpos != null) {
            double dblCurXpos = _curXpos.doubleValue();
            double dblCurYpos = _curYpos.doubleValue();
            double dblTargetXpos = _targetXpos.doubleValue();
            double dblTargetYpos = _targetYpos.doubleValue();

            if (dblCurXpos != dblTargetXpos || dblCurYpos != dblTargetYpos ) {
                double xdist = (dblTargetXpos - dblCurXpos) + step/1000.0;  //to avoid div/zero
                double ydist = (dblTargetYpos - dblCurYpos);
                double angle = Math.atan(ydist / xdist);
                if (xdist < 0) {
                    if (ydist > 0) {
                        angle = - angle + Math.PI/2.0;
                    }
                    else {
                        angle = angle + Math.PI;
                    }
                }
                _curXpos = new Double(_curXpos.doubleValue() + step * Math.cos(angle));
                _curYpos = new Double(_curYpos.doubleValue() + step * Math.sin(angle));
                if (Math.abs(_curXpos.doubleValue() - _targetXpos.doubleValue()) <= step) {
                    _curXpos = _targetXpos;
                }
                if (Math.abs(_curYpos.doubleValue() - _targetYpos.doubleValue()) <= step) {
                    _curYpos = _targetYpos;
                }
                Hashtable newPositions = new Hashtable();
                newPositions.put(AttributeList.NODE_XPOS, _curXpos);
                newPositions.put(AttributeList.NODE_YPOS, _curYpos);
                _fgraph.setVertexAttributeList(_envName, newPositions);
            }
        }
    }

    public void terminate()
    {
        _running = false;
        synchronized (this) {
            _fgraph.terminate();
            notifyAll();
        }
    }

    public void vertexAdded(String vertexID)
    {
        //do nothing
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        //do nothing
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        if (vertexID.compareTo(_envName) == 0) {
            if (attKey.equals(AttributeList.NODE_XPOS) || attKey.equals(AttributeList.NODE_XPOS)) {
                try {
                    _curXpos = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_XPOS);
                    _curYpos = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_YPOS);
                    _targetXpos = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_TARGET_XPOS);
                    _targetYpos = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_TARGET_YPOS);

                    if (_curXpos!=null && _curYpos!=null && _targetXpos!=null && _targetYpos!=null) {
                        System.out.println(_curXpos.doubleValue() + "," + _curYpos.doubleValue() + ","
                                           + _targetXpos.doubleValue() + "," + _targetYpos.doubleValue());
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                }

            }
        }
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        if (vertexID.compareTo(_envName) == 0) {
            try {
                _curXpos = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_XPOS);
                _curYpos = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_YPOS);
                _targetXpos = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_TARGET_XPOS);
                _targetYpos = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_TARGET_YPOS);

                if (_curXpos!=null && _curYpos!=null && _targetXpos!=null && _targetYpos!=null) {
                    System.out.println(_curXpos.doubleValue() + "," + _curYpos.doubleValue() + ","
                                       + _targetXpos.doubleValue() + "," + _targetYpos.doubleValue());
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        System.out.println("Got vertexAttrib removed event (vertexID=" + vertexID + ")");
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        //do nothing
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        //do nothing
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
        //do nothing
    }

    public void edgeAttribListSet(String edgeID, Hashtable attributes)
    {
        //do nothing
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
        //do nothing
    }

    public void connectionLost()
    {
        //do nothing
    }

    public void connected()
    {
        //do nothing
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[NetConMon] " + msg);
        }
    }

    private Double _curXpos;
    private Double _curYpos;
    private Double _targetXpos;
    private Double _targetYpos;

    private boolean _debug = false;
    private Hashtable _envFilter;
    private FGraph _fgraph;
    private double _defaultStepSize = 10;
    private long _defaultSleepInterval = 500;
    private boolean _running = true;
    private String _envName;
}

