package us.ihmc.aci.netviewer;

import javafx.embed.swing.JFXPanel;
import org.apache.log4j.PropertyConfigurator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.netviewer.conf.NodeMonProxyConf;
import us.ihmc.aci.netviewer.proxy.*;
//import us.ihmc.aci.netviewer.scenarios.AB14Scenario;
//import us.ihmc.aci.netviewer.scenarios.Command;
//import us.ihmc.aci.netviewer.scenarios.SampleScenario;
import us.ihmc.aci.netviewer.tabs.ChartTabsListener;
import us.ihmc.aci.netviewer.tabs.LinkTabsListener;
import us.ihmc.aci.netviewer.tabs.NodeTabsListener;
import us.ihmc.aci.netviewer.tabs.TabsPanel;
import us.ihmc.aci.netviewer.util.*;
import us.ihmc.aci.netviewer.util.Label;
import us.ihmc.aci.netviewer.views.charts.DemoChartsActionListener;
import us.ihmc.aci.netviewer.views.gauges.DemoGaugesActionListener;
import us.ihmc.aci.netviewer.views.gauges.GaugeType;
import us.ihmc.aci.netviewer.views.graph.GContainer;
import us.ihmc.charts.ChartsPanel;
import us.ihmc.charts.model.Bar2DData;
import us.ihmc.charts.model.Bar2DSeriesData;
import us.ihmc.charts.model.ChartInfo;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.Config;
import us.ihmc.util.serialization.SerializationException;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.net.Socket;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

/**
 * Displays nodes of a network as a graph attaching information about each node and connections
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class NetViewer extends JFrame implements NodeTabsListener, LinkTabsListener, ChartTabsListener, NodeMonUpdatesListener
{
    public NetViewer (String configFile, String log4jFile, boolean exitOnClose) throws SerializationException
    {
        initLogger (log4jFile);

        setTitle ("ACI Kernel Monitor");
        if (exitOnClose) {
            setDefaultCloseOperation (WindowConstants.EXIT_ON_CLOSE);
        }
        else {
            setDefaultCloseOperation (WindowConstants.DISPOSE_ON_CLOSE);
        }

        _tabsTextMap = new ConcurrentHashMap<>();

        _chScenarioCommands = new CommHelper();

        JMenuBar menuBar = new JMenuBar();
//        JMenu menuPlayScenario = new JMenu ("Play scenario");
//        _menuPlaySampleScenario = new JMenuItem ("Sample");
//        _menuPlaySampleScenario.addActionListener (new ActionListener()
//        {
//            @Override
//            public void actionPerformed (ActionEvent actionEvent)
//            {
//                _menuPlaySampleScenario.setEnabled(false);
//                _menuPlayAB14Scenario.setEnabled(false);
//                _leftPanel.setDividerLocation(0.85);
//                _leftPanel.setOneTouchExpandable(true);
//                String os = System.getProperty("os.name").toLowerCase();
//
//                try {
//                    if (os.contains("win")) {
//                        Runtime.getRuntime().exec(new String[]{"cmd.exe", "/C", "\"start; runSampleScenario.bat \""});
//                    } else {
//                        Runtime.getRuntime().exec(new String[]{"xterm", "-e", "./runSampleScenario.sh  ; le_exec"});
//                    }
//                }
//                catch (IOException e) {
//                    log.error("Impossible to run the sample scenario script");
//                    return;
//                }
//
//                try {
//                    _chScenarioCommands.init(new Socket("127.0.0.1", SampleScenario.COMMANDS_PORT));
//                } catch (IOException e) {
//                    log.error("Problem with the scenario commands comm helper", e);
//                }
//            }
//        });
//        menuPlayScenario.add(_menuPlaySampleScenario);
//
//        _menuPlayAB14Scenario = new JMenuItem ("Agile Bloodhound 2014");
//        _menuPlayAB14Scenario.addActionListener (new ActionListener()
//        {
//            @Override
//            public void actionPerformed(ActionEvent actionEvent)
//            {
//                _menuPlaySampleScenario.setEnabled(false);
//                _menuPlayAB14Scenario.setEnabled(false);
//                _leftPanel.setDividerLocation(0.85);
//                _leftPanel.setOneTouchExpandable(true);
//                String os = System.getProperty("os.name").toLowerCase();
//
//                try {
//                    if (os.contains("win")) {
//                        Runtime.getRuntime().exec(new String[]{"cmd.exe", "/C", "\"start; runAB14Scenario.bat \""});
//                    } else {
//                        Runtime.getRuntime().exec(new String[]{"xterm", "-e", "./runAB14Scenario.sh  ; le_exec"});
//                    }
//                } catch (IOException e) {
//                    log.error("Impossible to run the sample scenario script");
//                    return;
//                }
//
//                try {
//                    _chScenarioCommands.init(new Socket("127.0.0.1", AB14Scenario.COMMANDS_PORT));
//                } catch (IOException e) {
//                    log.error("Problem with the scenario commands comm helper", e);
//                }
//            }
//        });
//        menuPlayScenario.add(_menuPlayAB14Scenario);
//
//        menuBar.add (menuPlayScenario);

        JMenu demoGauge = new JMenu ("Demo Gauge");
        JMenuItem item = new JMenuItem (GaugeType.digital_radial.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.display_rectangular.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.linear.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.linear_bar_graph.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.radial.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.radial_one_square.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.radial_one_vertical.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.radial_two_top.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.radial_bar_graph.name());
        item.addActionListener(new DemoGaugesActionListener("Incoming traffic", "MB/s"));
        demoGauge.add(item);
        item = new JMenuItem (GaugeType.radial_counter.name());
        item.addActionListener (new DemoGaugesActionListener ("Incoming traffic", "MB/s"));
        demoGauge.add (item);

        menuBar.add (demoGauge);

        JMenu demoCharts = new JMenu ("Demo Charts");
        item = new JMenuItem ("Bar 2D");
        item.addActionListener (new DemoChartsActionListener ("Charts"));
        demoCharts.add (item);

        menuBar.add(demoCharts);

        setJMenuBar(menuBar);

        _nodesTabsPanel = new TabsPanel();
        _nodesTabsPanel.setPreferredSize (new Dimension (300, GContainer.VIEWER_HEIGHT + 200));
        _nodesTabsPanel.setBorder (BorderFactory.createTitledBorder ("NODES INFORMATION"));

        _linksTabsPanel = new TabsPanel();
        _linksTabsPanel.setPreferredSize (new Dimension (300, GContainer.VIEWER_HEIGHT + 200));
        _linksTabsPanel.setBorder (BorderFactory.createTitledBorder ("LINKS INFORMATION"));

        JSplitPane textInfoSplitPane = new JSplitPane (JSplitPane.HORIZONTAL_SPLIT, _nodesTabsPanel, _linksTabsPanel);
        textInfoSplitPane.setOneTouchExpandable (true);

        _chartsTabsPanel = new TabsPanel();
        _chartsTabsPanel.setPreferredSize (new Dimension (400, GContainer.VIEWER_HEIGHT + 200));
        _chartsTabsPanel.setBorder (BorderFactory.createTitledBorder ("CHARTS"));

        JSplitPane infoSplitPane = new JSplitPane (JSplitPane.HORIZONTAL_SPLIT, textInfoSplitPane, _chartsTabsPanel);
        infoSplitPane.setOneTouchExpandable (true);

        _gcontainer = new GContainer (this, this, this);
        JPanel graphPanel = (JPanel) (_gcontainer).getComponent();

        JPanel playerPanel = new JPanel();
//        playerPanel.setLayout (new GridBagLayout());
//        GridBagConstraints gbc = new GridBagConstraints();
//        JButton btnPlay = new JButton (Label.play.get());
//        btnPlay.setPreferredSize (new Dimension(80, 40));
//        btnPlay.addActionListener (new ActionListener()
//        {
//            @Override
//            public void actionPerformed (ActionEvent actionEvent)
//            {
//                try {
//                    _chScenarioCommands.sendLine (Command.play.toString());
//                }
//                catch (CommException e) {
//                    log.error ("Problem in sending the scenario command", e);
//                }
//            }
//        });
//        gbc.insets = new Insets (0,0,0,10);
//        gbc.gridx = 0;
//        gbc.gridy = 0;
//        playerPanel.add(btnPlay, gbc);
//
//        JButton btnPause = new JButton (Label.pause.get());
//        btnPause.setPreferredSize(new Dimension(80, 40));
//        btnPause.addActionListener (new ActionListener()
//        {
//            @Override
//            public void actionPerformed (ActionEvent actionEvent)
//            {
//                try {
//                    _chScenarioCommands.sendLine (Command.pause.toString());
//                }
//                catch (CommException e) {
//                    log.error ("Problem in sending the scenario command", e);
//                }
//            }
//        });
//        gbc.insets = new Insets (0,10,0,10);
//        gbc.gridx = 1;
//        gbc.gridy = 0;
//        playerPanel.add (btnPause, gbc);
//
//        JButton btnStop = new JButton (Label.stop.get());
//        btnStop.setPreferredSize (new Dimension (80, 40));
//        btnStop.addActionListener (new ActionListener()
//        {
//            @Override
//            public void actionPerformed (ActionEvent actionEvent)
//            {
//                int result = JOptionPane.showConfirmDialog (NetViewer.this, "Do you want to stop the scenario",
//                        "Stopping the scenario", JOptionPane.YES_NO_OPTION);
//                if (result == JOptionPane.NO_OPTION) {
//                    return;
//                }
//
//                try {
//                    _chScenarioCommands.sendLine (Command.stop.toString());
//                    connectionClosed();
//                    _leftPanel.setDividerLocation (1.0);
//                    _leftPanel.setOneTouchExpandable (false);
//                    _menuPlaySampleScenario.setEnabled (true);
//                    _menuPlayAB14Scenario.setEnabled (true);
//                }
//                catch (CommException e) {
//                    log.error ("Problem in sending the scenario command", e);
//                }
//            }
//        });
//        gbc.insets = new Insets (0,10,0,0);
//        gbc.gridx = 2;
//        gbc.gridy = 0;
//        playerPanel.add (btnStop, gbc);
//        playerPanel.setBorder (BorderFactory.createEmptyBorder (10, 0, 30, 0));

        _leftPanel = new JSplitPane (JSplitPane.VERTICAL_SPLIT, graphPanel, playerPanel);
        _leftPanel.setOneTouchExpandable (false);

        JSplitPane splitPane = new JSplitPane (JSplitPane.HORIZONTAL_SPLIT, _leftPanel, infoSplitPane);
        splitPane.setOneTouchExpandable (true);

        getContentPane().add (splitPane);
        validate();
        pack();
        setVisible (true);

        _leftPanel.setDividerLocation (1.0);

        Config.loadConfig (configFile);
        String clientName = Config.getStringValue (NodeMonProxyConf.CLIENT_NAME, NodeMonProxyConf.DEFAULT_CLIENT_NAME);
        String host = Config.getStringValue (NodeMonProxyConf.SERVER_HOST, NodeMonProxyConf.DEFAULT_SERVER_HOST);
        int port = Config.getIntegerValue (NodeMonProxyConf.SERVER_PORT, NodeMonProxyConf.DEFAULT_SERVER_PORT);

        JLabel lblClientName = new JLabel ("Client Name");
        JTextField txtClientName = new JTextField (10);
        txtClientName.setText (clientName);
        JLabel lblHost = new JLabel ("Host");
        JTextField txtHost = new JTextField (10);
        txtHost.setText (host);
        JLabel lblPort = new JLabel ("Port");
        JTextField txtPort = new JTextField (10);
        txtPort.setText (String.valueOf (port));
        JPanel pnlNMCConf = new JPanel (new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.WEST;
        gbc.gridx = 0;
        gbc.gridy = 0;
        pnlNMCConf.add (lblClientName, gbc);
        gbc.gridx = 1;
        gbc.gridy = 0;
        pnlNMCConf.add (txtClientName, gbc);
        gbc.gridx = 0;
        gbc.gridy = 1;
        pnlNMCConf.add (lblHost, gbc);
        gbc.gridx = 1;
        gbc.gridy = 1;
        pnlNMCConf.add (txtHost, gbc);
        gbc.gridx = 0;
        gbc.gridy = 2;
        pnlNMCConf.add (lblPort, gbc);
        gbc.gridx = 1;
        gbc.gridy = 2;
        pnlNMCConf.add (txtPort, gbc);

        int result = JOptionPane.showConfirmDialog (this, pnlNMCConf, "Node Monitor Proxy Configuration",
                JOptionPane.OK_CANCEL_OPTION);
        if (result == JOptionPane.OK_OPTION) {
            clientName = txtClientName.getText();
            host = txtHost.getText();
            try {
                port = Integer.parseInt (txtPort.getText());
            }
            catch (NumberFormatException e) {
                log.error ("Wrong port value");
            }
        }
        else {
            _nmConnector = null;
            return;
        }


        log.info (clientName + " trying to establish the connection with Node Monitor at " + host + ":" + port);
        _nmConnector = new NodeMonConnector (clientName, host, port);
        _nmConnector.addNodeMonUpdatesListener (this);
        _nmConnector.startup();

//        new TestProxyMethods (this);
    }

    // <-------------------------------------------------------------------------------------------------------------->
    // <-- Methods implementing NodeTabsListener                                                                    -->
    // <-------------------------------------------------------------------------------------------------------------->

    @Override
    public synchronized void addNodeTab (String id, String name)
    {
        if ((name == null) || (id == null)) {
            return;
        }

        for (int i=0; i<_nodesTabsPanel.getTabCount(); i++) {
            if (id.equals (_nodesTabsPanel.getIdAt (i))) {
                _nodesTabsPanel.setSelectedIndex (i);   // Not necessary to use SwingUtilities.invokeAndWait here because we are already on the EDT here
                return;
            }
        }

        _nodesTabsPanel.addTab (id, name, _tabsTextMap.get (id));
        _nodesTabsPanel.setSelectedIndex (_nodesTabsPanel.getTabCount() - 1);
    }

    @Override
    public synchronized void removeNodeTab (String id)
    {
        if (id == null) {
            return;
        }

        for (int i=0; i<_nodesTabsPanel.getTabCount(); i++) {
            if (id.equals (_nodesTabsPanel.getIdAt (i))) {
                _nodesTabsPanel.remove (i);     // Not necessary to use SwingUtilities.invokeAndWait here because we are already on the EDT here
            }
        }
    }

    // <-------------------------------------------------------------------------------------------------------------->

    // <-------------------------------------------------------------------------------------------------------------->
    // <-- Methods implementing LinkTabsListener                                                                    -->
    // <-------------------------------------------------------------------------------------------------------------->

    @Override
    public synchronized void addLinkTab (String id, String name)
    {
        if ((name == null) || (id == null)) {
            return;
        }

        for (int i=0; i<_linksTabsPanel.getTabCount(); i++) {
            if (id.equals (_linksTabsPanel.getIdAt (i))) {
                _linksTabsPanel.setSelectedIndex (i);   // Not necessary to use SwingUtilities.invokeAndWait here because we are already on the EDT here
                return;
            }
        }

        _linksTabsPanel.addTab (id, name, _tabsTextMap.get (id));
        _linksTabsPanel.setSelectedIndex (_linksTabsPanel.getTabCount() - 1);
    }

    @Override
    public synchronized void removeLinkTab (String id)
    {
        if (id == null) {
            return;
        }

        for (int i=0; i<_linksTabsPanel.getTabCount(); i++) {
            if (id.equals (_linksTabsPanel.getIdAt (i))) {
                _linksTabsPanel.remove (i);     // Not necessary to use SwingUtilities.invokeAndWait here because we are already on the EDT here
            }
        }
    }

    // <-------------------------------------------------------------------------------------------------------------->

    // <-------------------------------------------------------------------------------------------------------------->
    // <-- Methods implementing ChartTabsListener                                                                   -->
    // <-------------------------------------------------------------------------------------------------------------->

    @Override
    public void addChartTab (String id, String name)
    {
        if ((name == null) || (id == null)) {
            return;
        }

        for (int i=0; i<_chartsTabsPanel.getTabCount(); i++) {
            if (id.equals (_chartsTabsPanel.getIdAt (i))) {
                _chartsTabsPanel.setSelectedIndex (i);   // Not necessary to use SwingUtilities.invokeAndWait here because we are already on the EDT here
                return;
            }
        }

        List<ChartInfo> chartInfoList = new ArrayList<>();

        Bar2DData bar2DData = new Bar2DData();
        chartInfoList.add (new ChartInfo ("Traffic by IP", "IP", "y axis", bar2DData));

        Bar2DSeriesData seriesData = new Bar2DSeriesData (Bar2DSeriesData.ChartType.stack);
        chartInfoList.add (new ChartInfo ("Traffic", "years", "y axis",seriesData));

        JFXPanel fxPanel = new ChartsPanel().initPanel (chartInfoList);

        _chartsTabsPanel.addTab (id, name, fxPanel);
        _chartsTabsPanel.setSelectedIndex (_chartsTabsPanel.getTabCount() - 1);
    }

    @Override
    public void removeChartTab (String id)
    {
        if (id == null) {
            return;
        }

        for (int i=0; i<_chartsTabsPanel.getTabCount(); i++) {
            if (id.equals (_chartsTabsPanel.getIdAt (i))) {
                _chartsTabsPanel.remove (i);     // Not necessary to use SwingUtilities.invokeAndWait here because we are already on the EDT here
            }
        }
    }

    // <-------------------------------------------------------------------------------------------------------------->

    // <-------------------------------------------------------------------------------------------------------------->
    // <-- Methods implementing NodMonUpdatesListener                                                               -->
    // <-------------------------------------------------------------------------------------------------------------->

    @Override
    public void newNode (final UpdateContainer updateContainer)
    {
        if (updateContainer == null) {
            return;
        }

        try {
            SwingUtilities.invokeAndWait (new Runnable()
            {
                @Override
                public void run()
                {
                    String nodeId = updateContainer.getNodeId();
                    String nodeName = updateContainer.getNodeName();
                    _tabsTextMap.put (nodeId, updateContainer.getNodeInfo());
                    _gcontainer.addVertex (nodeId, nodeName, updateContainer.getIPs());
//                    _gcontainer.updateAggrTraffic (nodeId, updateContainer.getAggrInTraffic(),
//                            updateContainer.getAggrOutTraffic());
                    _gcontainer.updateAggrTraffic (nodeId, updateContainer.getIncomingSummary(),
                            updateContainer.getOutgoingSummary());
                    Map<String, NeighborLinkInfo> linksInfo = updateContainer.getNeighborLinkInfo();
                    for (String neighId : linksInfo.keySet()) {
                        _tabsTextMap.put (Utils.buildId (nodeId, neighId), linksInfo.get (neighId).getLinkInfo());
                        _gcontainer.addEdge (nodeId, neighId);
                    }
                    _gcontainer.updateSingleTraffic (nodeId, linksInfo);
                }
            });
        }
        catch (Exception e) {
            log.error ("Problem in adding the new node " + updateContainer.getNodeName(), e);
        }
    }

    @Override
    public void updateNode (final UpdateContainer updateContainer)
    {
        if (updateContainer == null) {
            return;
        }

        try {
            SwingUtilities.invokeAndWait (new Runnable()
            {
                @Override
                public void run()
                {
                    String nodeId = updateContainer.getNodeId();
                    String nodeName = updateContainer.getNodeName();
                    _tabsTextMap.put (nodeId, updateContainer.getNodeInfo());
//                    _gcontainer.updateAggrTraffic (nodeId, updateContainer.getAggrInTraffic(),
//                            updateContainer.getAggrOutTraffic());
                    _gcontainer.updateAggrTraffic (nodeId, updateContainer.getIncomingSummary(),
                            updateContainer.getOutgoingSummary());
                    _nodesTabsPanel.updateText (nodeId, updateContainer.getNodeInfo());
                    Map<String, NeighborLinkInfo> linksInfo = updateContainer.getNeighborLinkInfo();
                    for (String neighId : linksInfo.keySet()) {
                        String builtId = Utils.buildId (nodeId, neighId);
                        _tabsTextMap.put (builtId, linksInfo.get (neighId).getLinkInfo());
                        _gcontainer.addEdge (nodeId, neighId); // TODO: this is very inefficient
                        _linksTabsPanel.updateText (builtId, linksInfo.get (neighId).getLinkInfo());
                    }
                    _gcontainer.updateSingleTraffic (nodeId, linksInfo);


                    // TODO
//                    List<Data> data = new ArrayList<>();
//                    Bar2DData bar2DData = new Bar2DData();
//                    for (String ip : updateContainer.getTrafficByIPMap().keySet()) {
//
//                    }
//                    _chartsTabsPanel.updateCharts (nodeId, new ChartsPanel().initPanel (data))
                }
            });
        }
        catch (InterruptedException e) {
            log.error ("Problem in updating the node " + updateContainer.getNodeName(), e);
        }
        catch (InvocationTargetException e) {
            log.error ("Problem in updating the node " + updateContainer.getNodeName(), e);
            log.error ("Target exception: ", e.getTargetException());
        }

    }

    @Override
    public void deadNode (final String id)
    {
        try {
            SwingUtilities.invokeAndWait (new Runnable()
            {
                @Override
                public void run()
                {
                    removeNodeTab(id);
                    List<String> ids = _gcontainer.getEdgesIds (id);
                    for (String eid : ids) {
                        removeLinkTab (eid);
                    }
                    _gcontainer.removeVertex (id);
                }
            });
        }
        catch (Exception e) {
            log.error ("Problem in deleting the node id " + id, e);
        }
    }

    @Override
    public void connectionClosed()
    {
        _gcontainer.clean();
        _nodesTabsPanel.removeAll();
        _linksTabsPanel.removeAll();
        _chartsTabsPanel.removeAll();
    }

    // <-------------------------------------------------------------------------------------------------------------->

    private void initLogger (String log4jFile)
    {
        Properties log4jProperties = new Properties();
        try {
            log4jProperties.load (new FileInputStream (log4jFile));

            Date day = new Date();
            String formattedDate = new SimpleDateFormat("yyyyMMddHHmm").format (day);

            String logFileName = log4jProperties.getProperty ("log4j.appender.rollingFile.File");
            log4jProperties.setProperty ("log4j.appender.rollingFile.File", String.format ("../../log/%s-%s",
                    formattedDate, logFileName));
        }
        catch (FileNotFoundException e) {
            System.err.println ("Unable to load log4j configuration, file not found");
            e.printStackTrace();
        }
        catch (IOException e) {
            System.err.println ("Unable to load log4j configuration, error while I/O on disk");
            e.printStackTrace();
        }

        PropertyConfigurator.configure (log4jProperties);
    }


    public static void main (String[] args)
    {
        String configFile = DEFAULT_CONFIG_FILE;
        String log4jFile = DEFAULT_LOG_CONFIG_FILE;

        for (int i = 0; i < args.length; i++) {
            if (args[i].equals ("-c")) {
                configFile = args[++i];
            }
            else if (args[i].equals ("-log4j")) {
                log4jFile = args[++i];
            }
        }

        try {
            new NetViewer (configFile, log4jFile, true);
        }
        catch (SerializationException e) {
            log.error ("Problem in initializing the data serializer", e);
            System.exit (-1);
        }
    }



    private final JSplitPane _leftPanel;
    private final TabsPanel _nodesTabsPanel;
    private final TabsPanel _linksTabsPanel;
    private final ConcurrentMap<String, String> _tabsTextMap;
    private final TabsPanel _chartsTabsPanel;

    private final GContainer _gcontainer;

    public final static String DEFAULT_CONFIG_FILE = "../../conf/netviewer.properties";
    public final static String DEFAULT_LOG_CONFIG_FILE = "../../conf/netviewer.log4j.properties";
    private final NodeMonConnector _nmConnector;

//    private final JMenuItem _menuPlaySampleScenario;
//    private final JMenuItem _menuPlayAB14Scenario;

    private final CommHelper _chScenarioCommands;

    private static final Logger log = LoggerFactory.getLogger (NetViewer.class);
}
