package us.ihmc.aci.coord.resvis;

import java.awt.event.*;
import java.awt.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;
import javax.swing.border.EtchedBorder;
import javax.swing.DefaultListModel;
import javax.swing.JList;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;

import java.io.Serializable;
//import java.net.InetAddress;
//import java.net.Socket;
//import java.security.*;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import org.jfree.chart.*;
import org.jfree.chart.axis.CategoryAxis;
import org.jfree.chart.axis.CategoryLabelPositions;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.axis.SubCategoryAxis;
import org.jfree.chart.plot.CategoryPlot;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.Plot;
import org.jfree.chart.renderer.category.BarRenderer;
import org.jfree.chart.renderer.category.GroupedStackedBarRenderer;
import org.jfree.data.category.CategoryDataset;
import org.jfree.data.category.DefaultCategoryDataset;
import org.jfree.data.KeyToGroupMap;
import org.jfree.ui.GradientPaintTransformType;
import org.jfree.ui.StandardGradientPaintTransformer;

import us.ihmc.aci.coord.NodeInfo;

/**
 * The Resource Visualizer shows a chart representation of resources related to a
 * node. 
 * 
 * @author  Maggie Breedy <Nomads team>
 * @version $Revision$
 *
 **/

public class ResourceVisualizer
{
    public ResourceVisualizer()
    {
        ResourceVisualizerFrame resVisualizer = new ResourceVisualizerFrame();
        resVisualizer.setVisible (true);
        _dataInfoHashtable = new Hashtable();
        _nodeInfoHashtable = new Hashtable();
        setFrameLocation (resVisualizer);
    }

    /**
     * Updates the viewer everytime a peer node is created.
     * 
     * @param nodeName  the new node name
     * @param uuid      the new node uuid.
     */
    public void newPeer (String nodeName, String uuid)
    {
        System.out.println ("RV:newPeer - " + uuid);
        NodeInfoObject nio = new NodeInfoObject();
        nio.grpName = nodeName;
        nio.uuid = uuid;
        if (_deadNodeListModel.size() > 0) {
            for (int i=0; i<_deadNodeListModel.size(); i++) {
                String deadNodeName = (String) _deadNodeListModel.getElementAt (i);
                if (deadNodeName.equals (nio.grpName)) {
                    _deadNodeListModel.removeElementAt (i);
                }
            }
        }
        _nodeInfoHashtable.put (uuid, nio);
        _dataInfoHashtable.put (nodeName, uuid);
        Vector row = new Vector();
        row.add (nodeName);
        row.add (new Boolean (false));
        //_nodeListVector.addElement (row);
        _nodeListVector.insertElementAt (row, _nodeListVector.size());
        _nodeListTable.revalidate();

    }
    
    /**
     * Update the viewer list when a peer node dies.
     * 
     * @param uuid      the dead node uuid
     */
    public void deadPeer (String uuid)
    {
        System.out.println ("Peer - " + uuid + " died");
        NodeInfoObject nio = (NodeInfoObject) _nodeInfoHashtable.get (uuid);
        for (Enumeration e = _nodeListVector.elements(); e.hasMoreElements();) {
            Vector v = (Vector) e.nextElement();
            String nodeName = (String) v.elementAt(0);
            if (nio.grpName.equals (nodeName) && (nio.uuid.equals (uuid))) {
                _deadNodeListModel.addElement (nio.grpName);
                _nodeInfoHashtable.remove (uuid);
                _dataInfoHashtable.remove (nio.grpName);
                _nodeListVector.remove (v);
                break;
            }
            else {
                continue;
            }
        }
        _nodeListTable.revalidate();
    }
    
    /**
     * Updates the nodeInfo object everytime a new node information is 
     * received.
     * 
     * @param uuid          the uuid of the node
     * @param nodeInfo      the nodeInfo object
     */
    public void updateNodeInfo (String uuid, NodeInfo nodeInfo)
    {
        if (_nodeInfoHashtable.containsKey (uuid)) {
            NodeInfoObject nodeInfoObj = (NodeInfoObject) _nodeInfoHashtable.get (uuid);
            nodeInfoObj.nodeInfo = nodeInfo;
            updateDataset();
        }
        else {
        	System.out.println ("->The node is not in the list");
        }
    }
    
    /**
     * Returns a sample dataset.
     * 
     * @return The dataset.
     */
    public void updateDataset()
    {
        // create the dataset...
        //For each machine plot the resources.
        if (_nodeInfoHashtable.isEmpty()) {
        	System.out.println ("-->NodeInfoHashtable is empty: " + _nodeInfoHashtable);
            return;
        }
        //System.out.println ("-->>_nodeInfoHashtable: " + _nodeInfoHashtable);
        _dataset.clear();
        long maxMem = Long.MIN_VALUE;
        long maxDisk = Long.MIN_VALUE;
        int maxNetOut = Integer.MIN_VALUE;
        int maxNetIn = Integer.MIN_VALUE;

        //int maxNetTotMem = Integer.MIN_VALUE;
        //needs to add <ni.totalMemory>
        for (Enumeration e = _nodeInfoHashtable.elements(); e.hasMoreElements();) {
            NodeInfoObject nodeInfoObj = (NodeInfoObject) e.nextElement();
            //System.out.println ("-->>**NodeName: " + nodeInfoObj.grpName);
            //System.out.println ("-->>**isSelected: " + nodeInfoObj.isSelected);
            if (nodeInfoObj.isSelected == false) {
                continue;
            }
            else {
                NodeInfo ni = nodeInfoObj.nodeInfo;
                if (ni == null) {
                    return;
                }
                if (ni.freeDisk > maxDisk) {
                   maxDisk = ni.freeDisk;
                }
                if (ni.inboundNetworkUtilization > maxNetIn) {
                    maxNetIn = ni.inboundNetworkUtilization;
                }               
                if (ni.outboundNetworkUtilization > maxNetOut) {
                    maxNetOut = ni.outboundNetworkUtilization;
                }
                if (ni.memoryUtilization > maxMem) {
                    maxMem = ni.memoryUtilization;
                }
            }
        }

        maxDisk = roundUpToNearestMultipleOf10 (maxDisk);
        maxNetIn = roundUpToNearestMultipleOf10 (maxNetIn);
        maxNetOut = roundUpToNearestMultipleOf10 (maxNetOut);
        maxMem = roundUpToNearestMultipleOf10 (maxMem);
        if (maxNetIn < maxNetOut) {
            maxNetIn = maxNetOut;
        }
        else {
            maxNetOut = maxNetIn;
        }

        //String series1 = "ACI_CPU (%)";
        //String series1A= "Other_CPU(%)";
        //series2 = "DISK (x"+ maxDisk/100 + " MB)";
        //System.out.println ("-->>series2: " + series2);
        //series3 = "IN_NETWORK (x"+ maxNetIn/100 + " Bps)";
        //series4 = "OUT_NETWORK (x"+ maxNetOut/100 + " Bps)";
        //String series5 = "MEMORY (%)";

       for (Enumeration e = _nodeInfoHashtable.elements(); e.hasMoreElements();) {
            NodeInfoObject nodeInfoObj = (NodeInfoObject) e.nextElement();
            //System.out.println ("-->>**NodeName: " + nodeInfoObj.grpName);
            //System.out.println ("-->>**isSelected: " + nodeInfoObj.isSelected);
            if (nodeInfoObj.isSelected == false) {
                continue;
            }
            else {
                NodeInfo ni = nodeInfoObj.nodeInfo;
                if (ni == null) {
                    return;
                }
                System.out.println ("-->>ni.cpuUtilizationACI: " + ni.cpuUtilizationACI);
                System.out.println ("-->>ni.cpuUtilizationOther: " + ni.cpuUtilizationOther);
                System.out.println ("-->>ni.disk " + ni.freeDisk);
                System.out.println ("-->>ni.memoryUtilization " + ni.memoryUtilization);
                System.out.println ("-->>ni.inboundNetworkUtilization " + ni.inboundNetworkUtilization);
                System.out.println ("-->>ni.outboundNetworkUtilization " + ni.outboundNetworkUtilization);
                //System.out.println ("-->>nodeInfoObj.grpName: " + nodeInfoObj.grpName);
                if (_isByNode) {
                    if (ni.cpuUtilizationOther > 0) {
                        _dataset.addValue (ni.cpuUtilizationOther, _series1, nodeInfoObj.grpName);
                    }
                    else {
                        _dataset.addValue (0, _series1, nodeInfoObj.grpName);
                    }
                    if (ni.cpuUtilizationACI > 0) {
                        _dataset.addValue (ni.cpuUtilizationACI, _series1A, nodeInfoObj.grpName);
                    }
                    else {
                        _dataset.addValue (0, _series1A, nodeInfoObj.grpName);
                    }
                    if (ni.freeDisk > 0) {
                        if (maxDisk > 0) {
                            _dataset.addValue ((ni.freeDisk*100)/maxDisk, series2, nodeInfoObj.grpName);
                        }
                    }
                    else {
                        _dataset.addValue (0, series2, nodeInfoObj.grpName);
                    }
                    if (ni.inboundNetworkUtilization > 0) {
                        if (maxNetIn > 0) {
                            _dataset.addValue ((ni.inboundNetworkUtilization*100)/maxNetIn, series3, nodeInfoObj.grpName);
                        }
                    }
                    else {
                        _dataset.addValue (0, series3, nodeInfoObj.grpName);
                    }
                    if (ni.outboundNetworkUtilization > 0) {
                        if (maxNetOut > 0) {
                            _dataset.addValue ((ni.outboundNetworkUtilization*100)/maxNetOut, series4, nodeInfoObj.grpName);
                        }
                    }
                    else {
                        _dataset.addValue (0, series4, nodeInfoObj.grpName);
                    }
                    if (ni.memoryUtilization > 0) {
                         if (maxMem > 0) {
                            _dataset.addValue ((ni.memoryUtilization*100)/maxMem, series5, nodeInfoObj.grpName);
                         }
                    }
                    else {
                        _dataset.addValue (0, series5, nodeInfoObj.grpName);
                    }
                }
                else {
                    if (ni.cpuUtilizationOther > 0) {
                        _dataset.addValue (ni.cpuUtilizationOther, nodeInfoObj.grpName, _series1);
                    }
                    else {
                        _dataset.addValue (0, nodeInfoObj.grpName, _series1);
                    }
                    if (ni.cpuUtilizationACI > 0) {
                        _dataset.addValue (ni.cpuUtilizationACI, nodeInfoObj.grpName, _series1A);
                    }
                    else {
                        _dataset.addValue (0, nodeInfoObj.grpName, _series1A);
                    }
                    if (ni.freeDisk > 0) {
                        if (maxDisk > 0) {
                            _dataset.addValue ((ni.freeDisk*100)/maxDisk, nodeInfoObj.grpName, series2);
                        }
                    }
                    else {
                        _dataset.addValue (0, nodeInfoObj.grpName, series2);
                    }
                    if (ni.inboundNetworkUtilization > 0) {
                        if (maxNetIn > 0) {
                            _dataset.addValue ((ni.inboundNetworkUtilization*100)/maxNetIn, nodeInfoObj.grpName, series3);
                        }
                    }
                    else {
                        _dataset.addValue (0, nodeInfoObj.grpName, series3);
                    }
                    if (ni.outboundNetworkUtilization > 0) {
                        if (maxNetOut > 0) {
                            _dataset.addValue ((ni.outboundNetworkUtilization*100)/maxNetOut, nodeInfoObj.grpName, series4);
                        }
                    }
                    else {
                        _dataset.addValue (0, nodeInfoObj.grpName, series4);
                    }
                    if (ni.memoryUtilization > 0) {
                        if (maxMem > 0) {
                            _dataset.addValue ((ni.memoryUtilization*100)/maxMem, nodeInfoObj.grpName, series5);
                        }
                    }
                    else {
                        _dataset.addValue (0, nodeInfoObj.grpName, series5);
                    }
                }
                //updateLabels (ni.freeDisk, ni.inboundNetworkUtilization, ni.outboundNetworkUtilization);
            }

        }

        //_chartPanel.revalidate();
    }

    /*protected void updateLabels (long maxDisk, int maxNetIn, int maxNetOut)
    {
        _series1 = "ACI_CPU (%)";
        _series1A= "Other_CPU(%)";
        series2 = "DISK (x"+ maxDisk/100 + " MB)";
        series3 = "IN_NETWORK (x"+ maxNetIn/100 + " Bps)";
        series4 = "OUT_NETWORK (x"+ maxNetOut/100 + " Bps)";
        series5 = "MEMORY (%)";
    }*/

    /**
     * Creates a sample chart.
     * 
     * @param dataset  the dataset.
     * 
     * @return The chart.
     */
    public JFreeChart createChart (CategoryDataset dataset) 
    {
        // create the chart...
        JFreeChart chart = ChartFactory.createBarChart ("Resource Chart",         // chart title
                                                        "Node_Name",               // domain axis label
                                                        "Value",                  // range axis label
                                                        dataset,                  // data
                                                        PlotOrientation.VERTICAL, // orientation
                                                        true,                     // include legend
                                                        true,                     // tooltips?
                                                        false);                   //urls?

        // set the background color for the chart...
        chart.setBackgroundPaint(Color.white);

        // get a reference to the plot for further customisation...
        CategoryPlot plot = chart.getCategoryPlot();
        plot.setBackgroundPaint(Color.lightGray);
        plot.setDomainGridlinePaint(Color.white);
        plot.setDomainGridlinesVisible(true);
        plot.setRangeGridlinePaint(Color.white);

        // set the range axis to display integers only...
        final NumberAxis rangeAxis = (NumberAxis) plot.getRangeAxis();
        rangeAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());

        GroupedStackedBarRenderer stackRenderer = new GroupedStackedBarRenderer();
        KeyToGroupMap map = new KeyToGroupMap ("G1");
        map.mapKeyToGroup (_series1, "G1");
        map.mapKeyToGroup (_series1A, "G1");
        map.mapKeyToGroup (series2, "G2");
        map.mapKeyToGroup (series3, "G3");
        map.mapKeyToGroup (series4, "G4");
        map.mapKeyToGroup (series5, "G5");

        stackRenderer.setSeriesToGroupMap (map);
        stackRenderer.setItemMargin (0.1);
        stackRenderer.setDrawBarOutline(false);
        plot.setRenderer (stackRenderer);

        // set up gradient paints for series...
        Paint p1 = new GradientPaint (0.0f, 0.0f, Color.blue, 0.0f, 0.0f, Color.blue);
        stackRenderer.setSeriesPaint (0, p1);

        Paint p2 = new GradientPaint (0.0f, 0.0f, Color.gray, 0.0f, 0.0f, Color.gray);
        stackRenderer.setSeriesPaint (1, p2);

        Paint p3 = new GradientPaint (0.0f, 0.0f, Color.green, 0.0f, 0.0f, Color.green);
        stackRenderer.setSeriesPaint (2, p3);
        
        Paint p4 = new GradientPaint (0.0f, 0.0f, Color.red, 0.0f, 0.0f, Color.red);
        stackRenderer.setSeriesPaint (3, p4);

        Paint p5 = new GradientPaint (0.0f, 0.0f, Color.CYAN, 0.0f, 0.0f, Color.CYAN);
        stackRenderer.setSeriesPaint (4, p5);

        Paint p6 = new GradientPaint (0.0f, 0.0f, new Color (109,40,170), 0.0f, 0.0f, new Color (109,40,170));
        stackRenderer.setSeriesPaint (5, p6);

        stackRenderer.setGradientPaintTransformer (new StandardGradientPaintTransformer (GradientPaintTransformType.HORIZONTAL));

        plot.setFixedLegendItems(createLegendItems());
        CategoryAxis domainAxis = plot.getDomainAxis();
        domainAxis.setCategoryLabelPositions (CategoryLabelPositions.createUpRotationLabelPositions (Math.PI / 10.0));  //6.0
        return chart;
    }

    protected static LegendItemCollection createLegendItems()
    {
        LegendItemCollection legenditemcollection = new LegendItemCollection();
        LegendItem legenditem = new LegendItem ("ACI_CPU", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.blue);
        LegendItem legenditem1 = new LegendItem ("Other_CPU", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.gray);
        LegendItem legenditem2 = new LegendItem ("DISK", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.green);
        LegendItem legenditem3 = new LegendItem ("IN_NETWORK", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.red);
        LegendItem legenditem4 = new LegendItem ("OUT_NETWORK", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.CYAN);
        LegendItem legenditem5 = new LegendItem ("MEMORY", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, new Color (109,40,170));
        legenditemcollection.add(legenditem);
        legenditemcollection.add(legenditem1);
        legenditemcollection.add(legenditem2);
        legenditemcollection.add(legenditem3);
        legenditemcollection.add(legenditem4);
        legenditemcollection.add(legenditem5);
        return legenditemcollection;
  }


    /**
     * Sets the frame location at the center of the screen
     * 
     * @param frame     the frame to be displayed.
     */
	public void setFrameLocation (JFrame frame)
	{
	    Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension d = tk.getScreenSize();
        Dimension dd = frame.getPreferredSize();
        int width = ((d.width/3) - (dd.width/3)) + 80;
        int height = ((d.height/3) - (dd.height/3)) + 80;
        frame.setLocation (width, height);
	}

    protected int roundUpToNearestMultipleOf10 (int value)
    {
        int i = 10;
        while (((double) value / (double) i) > 1.0) {
            i *= 10;
        }
        return i;
    }

    protected int roundUpToNearestMultipleOf10 (long value)
    {
        int i = 10;
        while (((double) value / (double) i) > 1.0) {
            i *= 10;
        }
        return i;
    }

     /**
     * Build the Gui components
     */
    private class ResourceVisualizerFrame extends JFrame
    {
        public ResourceVisualizerFrame()
        {
            createGui();
        }
        
        protected void createGui()
        {
            setResizable (true);
            setTitle ("Resource Visualizer");
            getContentPane().setLayout (new GridBagLayout());
            setSize (700, 500);
            setVisible (false);
            GridBagConstraints gbc = new GridBagConstraints();
            
            //Panel1
            _leftPanel = new JPanel (new GridBagLayout());
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.gridheight = 2;
            gbc.gridwidth = 1;
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.NORTHWEST;
            gbc.weightx = 0.8;
            gbc.weighty = 1.0;
            gbc.insets = new Insets (0,0,0,0);
            _leftPanel.setBorder (new EtchedBorder());
            getContentPane().add (_leftPanel,gbc);
            
            //Panel2
            JPanel rightPanel = new JPanel (new GridBagLayout());
            gbc.gridx = 1;
            gbc.gridy = 0;
            gbc.gridwidth = 1;
            gbc.gridheight = 2;
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.SOUTHEAST;
            gbc.weightx = 0.2;
            gbc.weighty = 1.0;
            gbc.insets = new Insets (0,0,0,0);
            rightPanel.setBorder (new EtchedBorder());
            getContentPane().add (rightPanel,gbc);
            
            //rightPanel component
            gbc = new GridBagConstraints();
            JPanel topPanel = new JPanel (new GridBagLayout());
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.gridwidth = 1;
            gbc.gridheight = 1;
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.EAST;
            gbc.weightx = 0.2;
            gbc.weighty = 0.9;
            gbc.insets = new Insets (0,0,0,0);
            rightPanel.add (topPanel,gbc);
            
            JPanel bottonPanel = new JPanel (new GridBagLayout());
            gbc.gridx = 0;
            gbc.gridy = 1;
            gbc.gridwidth = 1;
            gbc.gridheight = 1;
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.SOUTHEAST;
            gbc.weightx = 0.2;
            gbc.weighty = 0.5;
            gbc.insets = new Insets (0,0,0,0);
            rightPanel.add (bottonPanel,gbc);
            
            //topPanel
            gbc = new GridBagConstraints();
            _nodeListVector = new Vector();
            _model = new NodeNameTableModel (_nodeListVector);           
            _nodeListTable = new JTable (_model);
            _nodeListTable.getColumn("A").setHeaderValue ("Node Name");
            _nodeListTable.getColumn("B").setHeaderValue ("Selection");
            
            for (int i = 0; i < 2; i++) {
                TableColumn tableMod = _nodeListTable.getColumnModel().getColumn(i);
                if (i == 0) {
                    tableMod.setPreferredWidth (75);
                }
                else if (i == 1) {
                    tableMod.setPreferredWidth (25);
                }
            }            
            //create scrollpane containing table
            JScrollPane scrollPane = new JScrollPane (_nodeListTable);
            scrollPane.getViewport().setBackground (_nodeListTable.getBackground());
            
            _nodeListTable.setCellSelectionEnabled (true);
            _nodeListTable.setShowGrid (true);
            _nodeListTable.setShowHorizontalLines (false);
            _nodeListTable.setShowVerticalLines (false);
            _nodeListTable.setColumnSelectionAllowed (false);
            _nodeListTable.setRowSelectionAllowed (true);
 
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.CENTER;
            gbc.insets = new Insets (5,5,0,5);
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.weightx = 1.0;
            gbc.weighty = 1.0;
            topPanel.add (scrollPane, gbc);
            
            JList list = new JList();
            ListSelectionModel lsm = list.getSelectionModel();
            lsm.setSelectionMode (ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
            lsm.addListSelectionListener (new TableListSelectionHandler());
            _nodeListTable.setSelectionModel(lsm);
            
            //bottonPanel
            gbc = new GridBagConstraints();
            JLabel jLabel2 = new JLabel ("Dead Nodes");
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.insets = new Insets (5,5,0,5);
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.anchor = GridBagConstraints.SOUTH;
            bottonPanel.add (jLabel2, gbc);
            
            _deadNodeList = new JList();
            _deadNodeListModel = (new DefaultListModel());
            _deadNodeList.setModel (_deadNodeListModel);
            JScrollPane sp2 = new JScrollPane (_deadNodeList);
            _deadNodeList.setBorder (new EtchedBorder());
            sp2.setViewportView (_deadNodeList);
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.SOUTH;
            gbc.insets = new Insets (5,5,5,5);
            gbc.gridx = 0;
            gbc.gridy = 1;
            gbc.weightx = 1.0;
            gbc.weighty = 0.9;
            bottonPanel.add (sp2, gbc);
            
            //LeftPanel Visualizer components
            gbc = new GridBagConstraints();
            _dataset = new DefaultCategoryDataset();
            JFreeChart chart = createChart (_dataset);
            _chartPanel = new ChartPanel (chart, false);
            gbc.gridx = 0;
            gbc.gridy = 2;
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.CENTER;
            gbc.weightx = 1.0;
            gbc.weighty = 1.0;
            gbc.insets = new Insets (5, 5, 5, 5);
            _leftPanel.add (_chartPanel, gbc);
            _chartPanel.setEnabled (false);
            
            //Menu items
            JMenuBar menuBar = new JMenuBar();
            setJMenuBar (menuBar);
            
            _viewMenu = new JMenu();
            _viewMenu.setBackground (java.awt.Color.lightGray);
            _viewMenu.setText ("View");
            _viewMenu.setActionCommand ("View");
            _viewMenu.setMnemonic ((int)'V');
            menuBar.add (_viewMenu);

            JMenuItem byNodeMenuItem = new JMenuItem();
            byNodeMenuItem.setText ("By Node");
            byNodeMenuItem.setActionCommand ("By Node");
            byNodeMenuItem.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_N, KeyEvent.CTRL_MASK));
            byNodeMenuItem.setMnemonic ((int)'N');
            _viewMenu.add (byNodeMenuItem);
            JSeparator JSeparator1 = new JSeparator();
            _viewMenu.add (JSeparator1);
            
            JMenuItem byResourceMenuItem = new JMenuItem();
            byResourceMenuItem.setText ("By Resource");
            byResourceMenuItem.setActionCommand ("By Resource");
            byResourceMenuItem.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_N, KeyEvent.CTRL_MASK));
            byResourceMenuItem.setMnemonic ((int)'R');
            _viewMenu.add (byResourceMenuItem);
            JSeparator JSeparator2 = new JSeparator();
            _viewMenu.add (JSeparator2);
            
            byNodeMenuItem.addActionListener (new ActionListener() {
                public void actionPerformed (ActionEvent e) {
                    _isByNode = true;
                }
            });
            
            byResourceMenuItem.addActionListener (new ActionListener() {
                public void actionPerformed (ActionEvent e) {
                    _isByNode = false;
                }
            });
            
            _nodeListTable.getModel().addTableModelListener (new TableModelListener() {
                public void tableChanged (TableModelEvent e) {
                    TableModel model = (TableModel) e.getSource();
                    int row = _nodeListTable.getSelectedRow();
                    //System.out.println ("->>>>Row number: " + row);
                    Object data = _model.getValueAt (row, 0);
                    String nodeName = data.toString();
                    Boolean bol = (Boolean) model.getValueAt (row, 1);
                    //System.out.println ("->>>>Boolean: " + bol.booleanValue());
                    String uuid = (String) _dataInfoHashtable.get (nodeName);
                    NodeInfoObject nodeInfoObj = (NodeInfoObject) _nodeInfoHashtable.get (uuid);
                    if (bol.booleanValue() == false) {
                        //System.out.println ("->>>>Unselected node");
                        nodeInfoObj.isSelected = false; 
                    }
                    else {
                        nodeInfoObj.isSelected = true;                            
                    }
                    updateDataset();
                }
            });
            
            this.addWindowListener(new WindowAdapter() {
                public void windowClosing (WindowEvent e) {
                    dispose();
                    _dataInfoHashtable.clear();
                    _nodeInfoHashtable.clear();
                    _nodeListVector.removeAllElements();
                    System.exit (1);
                }
             });
        }
     }
    
    /**
     *  Node name Table Model
     */
    protected class NodeNameTableModel extends AbstractTableModel 
    {    
        public NodeNameTableModel (Vector nodeVector) 
        {
            super();
            _nodeVector = nodeVector;
        }

        public int getRowCount() 
        {
            int count = -1;
            if (_nodeVector != null) {
                count = _nodeVector.size();
            }
            else {
                count = 0;
            }
            return count;
        }
        
        // Add a string and a check box to the table cell.
        public Class getColumnClass(int c) 
        {
            if (c==0) {
                return (new String()).getClass();
            }
            else if (c==1) {
                return (new Boolean (false)).getClass();
            }
            else {
                return null;
            }
        }

        public int getColumnCount()
        {
            return 2;
        }
        
        // Only column 1 is editable.
        public boolean isCellEditable(int row, int col) 
        {
            if (col == 1) {
                return true;
            } 
            else {
                return false;
            }
        }

        public Object getValueAt (int row, int column) 
        {
            if (_nodeVector == null) {
                return null;
            }
            else if (_nodeVector.isEmpty()) {
                return null;
            }
            else {
                //System.out.println ("---->>Vector _nodeVector: " + _nodeVector);
                Object obj = (((Vector) _nodeVector.get (row)).get (column));
                return obj;
            }
        }

        public void setValueAt (Object value, int row, int col) 
        {
            Vector rowVec = (Vector) _nodeVector.get (row);
            rowVec.removeElementAt (col);
            rowVec.insertElementAt (value, col);
            fireTableCellUpdated (row, col);
        }
        
        public void removeNodeAtRow (int row)
        {
            _nodeVector.removeElementAt (row);
            fireTableRowsDeleted (row, row);
            //fireTableDataChanged();
        }
        
        public void removeAllNodeNames()
        {
            _nodeVector.removeAllElements();
            fireTableDataChanged();
        }
        
        /**
         * Adds a row.
         */
        public void addRow (Vector v)
        {
            addRow (getRowCount(), v);
        }

        /**
         * Adds a row at the given index.
         */
        public void addRow (int index, Vector v)
        {
            _nodeVector.add (index, v);
            fireTableRowsInserted (index, index);
        }       
        protected Vector _nodeVector;
    }
    
    /**
     * The Table List Selection handler.
     */
    public class TableListSelectionHandler implements ListSelectionListener
    {
        public void valueChanged (ListSelectionEvent evt)
        {
            ListSelectionModel lsm = (ListSelectionModel) evt.getSource();
            if (lsm.isSelectionEmpty()) {
                System.out.println ("No element was selected.");
                return;
            } 
            int row = _nodeListTable.getSelectedRow();
            int column = _nodeListTable.getSelectedColumn();
            
            Object data = _model.getValueAt (row, 0);            
        }
    }
    
    /**
     * The Info object that contains the group name, member UUID and the ip of
     * the selected group.
     */
    public class NodeInfoObject implements Serializable
    {
        public NodeInfoObject()
        {
        }

        public boolean isSelected;
        public String grpName;
        public String uuid;
        public NodeInfo nodeInfo;
    }

    public static void main (String args[])
    {
        ResourceVisualizer resvis = new ResourceVisualizer();
    }

    protected boolean _isByNode = true;
    protected ChartPanel _chartPanel;
    protected DefaultListModel _deadNodeListModel;
    protected JPanel _leftPanel;
    protected JList _deadNodeList;
    protected JMenu _viewMenu;
    protected JTable _nodeListTable;
    protected Hashtable _dataInfoHashtable;
    protected Hashtable _nodeInfoHashtable;
    protected DefaultCategoryDataset _dataset;
    protected NodeNameTableModel _model;
    protected String _series1 = "ACI_CPU";
    protected String _series1A= "Other_CPU";
    protected String series2 = "DISK";
    protected String series3 = "IN_NETWORK";
    protected String series4 = "OUT_NETWORK";
    protected String series5 = "MEMORY";
    protected Vector _nodeListVector;


}