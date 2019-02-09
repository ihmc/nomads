package us.ihmc.aci.gss;

import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.ds.fgraph.FGraphServer;
import us.ihmc.util.ConfigLoader;

import java.net.URI;
import java.util.Hashtable;

/**
 * GSSServer
 * Global System State (GSS) Server
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on May 17, 2004 at 4:06:38 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class GSSServer implements FGraphEventListener
{
    public GSSServer (int fgraphServerPort) throws Exception
    {
        _fgraph = FGraph.getServer (fgraphServerPort);
        _fgraph.addGraphEventListener(this);
    }

    public GSSServer () throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        int fgraphServerPort = cloader.getPropertyAsInt("gss.server.default.port");
        _fgraph = FGraph.getServer (fgraphServerPort);
        _fgraph.addGraphEventListener(this);
    }

    public GSSServer (int fgraphServerPort, boolean allowDuplicates) throws Exception
    {
        _fgraph = FGraph.getServer (fgraphServerPort);
        _fgraph.addGraphEventListener(this);
        ((FGraphServer) _fgraph).setAllowDuplicates(allowDuplicates);
    }

    public GSSServer (boolean allowDuplicates) throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        int fgraphServerPort = cloader.getPropertyAsInt("gss.server.default.port");
        _fgraph = FGraph.getServer (fgraphServerPort);
        _fgraph.addGraphEventListener(this);
        ((FGraphServer) _fgraph).setAllowDuplicates(allowDuplicates);
    }

    public URI getServerURI () throws Exception
    {
        return (_fgraph.getURI());
    }

    public static void main (String[] args)
    {
        int fgraphServerPort = 0;
        boolean allowDuplicates = false;

        ConfigLoader.initDefaultConfigLoader(System.getProperty("nomads.home"), "/conf");

        try {
            for (int i=0; i<args.length; i++) {
                String arg = args[i];
                if (arg.compareTo("-help")==0) {
                    showUsage();
                }
                if (arg.compareTo("-gss.fgraph.port")==0 && args.length > i+1) {
                    fgraphServerPort = Integer.parseInt (args[i+1]);
                }
                if (arg.compareTo("-allow.duplicates")==0) {
                    allowDuplicates = true;
                }
            }

            GSSServer gssServer = null;
            if (allowDuplicates) {
                if (fgraphServerPort == 0) {
                    gssServer = new GSSServer (true);
                }
                else {
                    gssServer = new GSSServer (fgraphServerPort, true);
                }
            }
            else {
                if (fgraphServerPort == 0) {
                    gssServer = new GSSServer ();
                }
                else {
                    gssServer = new GSSServer (fgraphServerPort);
                }
            }

            /*
            if (args.length ==0) {
                GSSServer gssServer = new GSSServer ();
            }
            else {
                for (int i=0; i<args.length; i++) {
                    String arg = args[i];
                    if (arg.compareTo("-gss.fgraph.port")==0) {
                        if (args.length > i+1) {
                            fgraphServerPort = Integer.parseInt (args[i+1]);
                            GSSServer gssServer = new GSSServer (fgraphServerPort);
                        }
                    }
                    if (arg.compareTo("-h")==0 || arg.compareTo("-help")==0) {
                        showUsage();
                    }
                    if (arg.compareTo("-allow.duplicates")==0) {
                        showUsage();
                    }
                }
            }
            */
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //////////////////////// FGraphEventListener Inteface Methods ///////////////////////////
    public void vertexAdded(String vertexID)
    {
        debugMsg("VERTEX_ADDED --------------------------------------------\n");
        debugMsg(_fgraph.toString());
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexRemoved(String vertexID, Hashtable attributeList)
    {
        debugMsg("VERTEX_REMOVED --------------------------------------------\n");
        debugMsg(_fgraph.toString());
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        debugMsg("VERTEX_ATTRIBUTE_SET (" + vertexID + ":" + attKey + ")\n");
        debugMsg(_fgraph.toString());
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribListSet(String vertexID, Hashtable attribute)
    {
        debugMsg("VERTEX_ATTRIBUTELIST_SET (" + vertexID + ")\n");
        debugMsg(_fgraph.toString());
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        debugMsg("VERTEX_ATTRIBUTE_REMOVED" +
                 " (" + vertexID + ":" + attKey + ")\n");
        debugMsg(_fgraph.toString());
    }
/*
    public void vertexAttribChanged(String vertexID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }
*/
    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        debugMsg("EDGE_ADDED --------------------------------------------\n");
        //debugMsg(_fgraph.toString());
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeRemoved(String edgeID, String source, String dest, Hashtable attributeList)
    {
        debugMsg("EDGE_REMOVED --------------------------------------------\n");
        //debugMsg(_fgraph.toString());
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribListSet(String edgeID, Hashtable attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }
/*
    public void edgeAttribChanged(String edgeID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }
*/
    public void connectionLost()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void connected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }


    private static void showUsage()
    {
        System.out.println ("Global System State Server (GSSServer)");
        System.out.println ("Usage: us.ihmc.aci.gss.GSSServer -gss.fgraph.port <serverPort>");
        System.out.println ("NOTE: use -Daci.conf.dir to specify the location (dir) of the aci.properties file");
        System.exit(0);
    }

    private void debugMsg (String smsg)
    {
        if (_debug == true) {
            System.out.println ("[GSSServer] " + smsg);
        }
    }

    private boolean _debug = true;
    private FGraph _fgraph;
}
