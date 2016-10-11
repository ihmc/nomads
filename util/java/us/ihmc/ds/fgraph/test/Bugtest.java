/*
 * Bugtest.java
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

package us.ihmc.ds.fgraph.test;

import java.net.*;
import java.util.*;

//import us.ihmc.aci.gss.*;
import us.ihmc.ds.fgraph.*;

public class Bugtest implements FGraphEventListener
{
  private static FGraph fgraph;

  public Bugtest(String[] args) {
      String param = "tcp://localhost:7495";
      try {
        fgraph = FGraph.getClient(new URI(param));
          fgraph.addGraphEventListener(this);
        //bt.createTestGraphMarco();
        //bt.preprocessFGraph_test();
        //bt.testFilter();
        //bt.testBlocking();

          testEcho (true);

          /*
          boolean control = false;
          if (args.length > 0) {
              for (int i=0; i<args.length; i++) {
                  if (args[i].compareTo("echo")==0) {
                      control = true;
                  }
              }
          }
          if (control) {
              System.out.println ("ECHO IS ON");
          }
          else {
              System.out.println ("ECHO IS OFF");
          }
          //fgraph.setBlockingMode (!control);
          this.testEcho(control);
          */
      }
      catch (Exception ex) {
      }
  }

  public static void main(String args[]) {
    Bugtest bt = new Bugtest(args);

  }
    private void testBlocking ()
    {
        for (int i=0; i<50; i++) {
            try {
                if (i==40) {
                    fgraph.setCommitRequiredMode(false);
                }
                String sVtx = "A" + i;
                System.out.println ("Adding FGInfoVertex: " + sVtx);
                fgraph.addVertex(sVtx);
                fgraph.setVertexAttribute(sVtx, "TEST", "TEST");
            }
            catch (Exception e) {
                System.out.println (e.getMessage());
            }
        }
    }


    private void testEcho (boolean echo)
    {
        Random rand = new Random();
        fgraph.setEcho(echo);
        int counter = 0;
        for (int i=0; i<30; i++)
        {
            try {
                String svertex = "V" + Math.abs(rand.nextInt());
                System.out.println ("vertex: " + svertex);
                if (counter++ > 5) {
                    System.out.println(fgraph.toString());
                    counter = 0;
                }
                fgraph.addVertex(svertex);
                Thread.sleep(1000);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }

    }


    private void testFilter ()
    {
        try {
            fgraph.addVertex("V1");
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        try {
            fgraph.addVertex("V2");
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            Hashtable attA = new Hashtable();
            attA.put("EDGE_TYPE","TYPE_A");
            attA.put("EDGE_COST",new Double(2));
            fgraph.addEdge("E1", "V1", "V2", attA);
            fgraph.addEdge("E2", "V1", "V2", attA);
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            Hashtable attB = new Hashtable();
            attB.put("EDGE_TYPE","TYPE_B");
            attB.put("EDGE_COST",new Double(2));
            fgraph.addEdge("E4", "V1", "V2", attB);
            fgraph.addEdge("E5", "V1", "V2", attB);
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            Hashtable attC = new Hashtable();
            attC.put("EDGE_TYPE","TYPE_A");
            attC.put("EDGE_COST",new Double(1));
            fgraph.addEdge("E3", "V1", "V2", attC);
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            Hashtable attD = new Hashtable();
            attD.put("EDGE_TYPE","TYPE_B");
            attD.put("EDGE_COST",new Double(1));
            fgraph.addEdge("E6", "V1", "V2", attD);
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            System.out.println ("\nListing ALL The edges");
            Enumeration en= fgraph.getEdges();
            while (en.hasMoreElements()) {
                System.out.print ((String) en.nextElement() + " ");
            }
            System.out.println();

            System.out.println ("\nListing EDGES - FILTER 1");
            Hashtable filter1 = new Hashtable();
            filter1.put("EDGE_TYPE","TYPE_A");
            en= fgraph.getEdges(filter1);
            while (en.hasMoreElements()) {
                System.out.print ((String) en.nextElement() + " ");
            }
            System.out.println();

            System.out.println ("\nListing EDGES - FILTER 2");
            Hashtable filter2 = new Hashtable();
            filter2.put("EDGE_COST",new Double(2));
            en= fgraph.getEdges(filter2);
            while (en.hasMoreElements()) {
                System.out.print ((String) en.nextElement() + " ");
            }
            System.out.println();

        }
        catch (Exception e) {
            e.printStackTrace();
        }


    }


  /**
   * createTestGraphMarco
   *
   */
  private void createTestGraphMarco() {
    if (fgraph == null) {
      return;
    }
    try {
      Hashtable hEnv = new Hashtable();
      Hashtable hAgent = new Hashtable();
      Hashtable hEdge = new Hashtable();
      hEnv.put("node_type", "ENVIRONMENT");
      hAgent.put("node_type", "AGENT");
      hEdge.put("edge_type", "STREAM_REQUEST");

      fgraph.addVertex("V1", hAgent);
      fgraph.addVertex("V2", hAgent);
      fgraph.addVertex("V3", hEnv);
      fgraph.addVertex("V4", hEnv);
      fgraph.addEdge("E1", "V1", "V2", hEdge);

      hEdge.put("edge_type", "MEMBERSHIP");
      fgraph.addEdge("E2", "V1", "V3", hEdge);
      fgraph.addEdge("E3", "V2", "V4", hEdge);
      fgraph.addEdge("E4", "V3", "V4", hEdge);

      try {
        Thread.sleep(200);
      }
      catch (InterruptedException ex1) {
      }

      System.err.println(fgraph.toString());

    }
    catch (FGraphException ex) {
    }

  }

  private void preprocessFGraph_test() {
    String edgeId = "E1";
    String agentSource = "V1";
    String agentTarget = "V2";
    String envSource = "V3";
    String envTarget = "V4";

    // now envSource and envTarget should be set
    // otherwise the fginfo structure is incorrect!

    Hashtable h = null;
    try {
      h = fgraph.getEdgeAttributeList(edgeId);

      // remove old STREAM_REQUEST edge
      fgraph.removeEdge(edgeId);
      System.err.println("removed E1");
      outputGraph();

      // add a new STREAM_REQUEST edge (and hopefully keep the hashtable!)
      System.err.println("new FGInfoEdge: " + edgeId + ": " + envSource + " - " + envTarget);
      //fgraph.addEdge(edgeId+"changedName", envSource, envTarget, h);
      fgraph.addEdge(edgeId, envSource, envTarget, h);

      outputGraph();

      fgraph.removeVertex("V1");
      System.err.println("removed V1");
      outputGraph();

      fgraph.removeVertex("V2");
      System.err.println("removed V2");
      outputGraph();

    }
    catch (FGraphException ex) {
    }
  }

  private void outputGraph(){
    try {
      Thread.sleep(200);
    }
    catch (InterruptedException ex1) {
    }
    System.err.println(fgraph.toString());

  }

    public void vertexAdded(String vertexID)
    {
        System.out.println ("Got Notified FGInfoVertex Added (" + vertexID + ")");
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexRemoved(String vertexID, Hashtable attList)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribListSet(String vertexID, Hashtable attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeRemoved(String edgeID, String source, String sink, Hashtable attList)
    {
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

    public void connectionLost()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void connected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

}
