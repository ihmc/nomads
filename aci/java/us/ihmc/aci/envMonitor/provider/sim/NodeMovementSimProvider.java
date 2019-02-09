/**
 * NetConnectivitySimProvider
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 * 
 * @version     $Revision$
 *              $Date$
 */
package us.ihmc.aci.envMonitor.provider.sim;

import java.net.URI;
import java.util.Hashtable;

import us.ihmc.aci.AttributeList;
import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.util.ConfigLoader;

/**
 * 
 */
public class NodeMovementSimProvider extends AsyncProvider
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

        _envName = _environmentalMonitor.getNodeName();
        _fgraph = FGraph.getClient(new URI("tcp://" + gssServerHost + ":" + gssPort));

        boolean moveNode = !Boolean.valueOf(cloader.getProperty("node.static", "false")).booleanValue();
        if (moveNode) {
            double xCenter = cloader.getPropertyAsDouble("circle.center.x");
            double yCenter = cloader.getPropertyAsDouble("circle.center.y");
            double radius = cloader.getPropertyAsDouble("circle.radius");
            _node = new CircularNode(radius, 0.0, xCenter, yCenter, false);        
            _updateSpeed  = cloader.getPropertyAsInt("speed"); //milliseconds.            
        }
        else {
            double xPos = cloader.getPropertyAsDouble("node.x");
            double yPos = cloader.getPropertyAsDouble("node.y");
            _node = new StaticNode(xPos, yPos);
            _updateSpeed  = Integer.MAX_VALUE;
        }

        setPosition();
        
//        this.start();
    }

    public void run()
    {
        try {
            while (_running) {
                setPosition();
                _node.reposition();
                try {
                    sleep(_updateSpeed);
                }
                catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
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
    
    public void setPosition()
    {
        Hashtable attrs = new Hashtable();
        attrs.put(AttributeList.NODE_XPOS, new Double(_node.getX()));
        attrs.put(AttributeList.NODE_YPOS, new Double(_node.getY()));
        attrs.put(AttributeList.NODE_ZPOS, new Double(0.0));
        
        try {
            _fgraph.setVertexAttributeList(_envName, attrs);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    //----------------------------------------------------------------

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[NetConMon] " + msg);
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    
//    private CircularNode _node;
    private Node _node;

    private boolean _debug = false;
//    private Hashtable _edgeFilter;
//    private Hashtable _envFilter;
    private FGraph _fgraph;
    private String _envName;
    private int _updateSpeed;
    private boolean _running = true;
}

