package us.ihmc.netLogViewer;

import javax.swing.*;
import java.awt.*;
import java.awt.Color;

import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.Serializable;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.ui.RectangleInsets;

/**
 * The path viewer is invoked when a line is received from the log messages.
 *
 * @author: Maggie Breedy
 * Date: Jan 17, 2008
 * Time: 10:28:57 AM
 * @version $Revision$
 */

public class PathViewer extends JDialog
{
    public PathViewer (String name)
    {
        _dataTable = new Hashtable();
        _lfrk = new LogFileReaderKalman();
        _lfr = new LogFileReader();
        _lfrc = new LogFileReaderCricket();
        createGUI (name);
    }

    public void processKalmanLine (String line)
    {
        setTitle ("GPS Kalman Plotter");
        if (_lfrk.processLine (line)) {
            String ip = _lfrk.getIPAddress();
            double x = _lfrk.getX ();
            double y = _lfrk.getY ();
            //System.out.println("K IPAddr " + ip + " + has x:" + x + " y:" +y);
            _xyDataset.addPoint (ip, x, y);
        }
    }

    public void processNoKalmanLine (String line)
    {
        setTitle ("GPS Plotter");
        System.out.println("****line:" + line);
        if (_lfr.processNoKalmanLine (line)) {
            String ip = _lfr.getIPAddress();
            double x = _lfr.getX ();
            double y = _lfr.getY ();
            //System.out.println("NOK IPAddr " + ip + " + has x:" + x + " y:" + y);
            _xyDataset.addPoint (ip, x, y);
        }
    }

    public void processCricketLine (String line)
    {
        setTitle ("Cricket Plotter");
        System.out.println("****PathViewer line:" + line);
        if (_lfrc.processCricketLine (line)) {
            String ip = _lfrc.getIPAddress();
            double x = _lfrc.getX ();
            double y = _lfrc.getY ();
            System.out.println("Cricket IPAddr " + ip + " + has x:" + x + " y:" + y);
            _xyDataset.addPoint (ip, x, y);
        }
    }

    /**
     * The coordinates x, y and heading values are store in a hashtable where the key is the ip
     * and the value is a vector of vectors containing the coordinates and heading for each ip.
     */
    public void go (String fileName, String arg)
    {
        try {
            if (arg.equals ("kalman")) {
                System.out.println ("-->>Kalman in use");
                _lfrk.init (fileName);
                while (_lfrk.readNextEntry()) {
                    String ip = _lfrk.getIPAddress();
                    double x = _lfrk.getX ();
                    double y = _lfrk.getY ();
                    _xyDataset.addPoint (ip, x, y);
                }
            }
            else if (arg.equals ("nokalman")) {
                System.out.println("-->>NO Kalman in use");
                _lfr.init (fileName);
                while (_lfr.readNextEntry()) {
                    String ip = _lfr.getIPAddress();
                    double x = _lfr.getX ();
                    double y = _lfr.getY ();
                    _xyDataset.addPoint (ip, x, y);
                }
            }
            else if (arg.equals ("cricket")) {
                System.out.println("-->>Cricket in use");
                _lfrc.init (fileName);
                while (_lfrc.readNextEntry()) {
                    String ip = _lfr.getIPAddress();
                    double x = _lfr.getX ();
                    double y = _lfr.getY ();
                    _xyDataset.addPoint (ip, x, y);
                }
            }
       }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    protected JFreeChart createChart()
    {
        _xyDataset = new NodePositionsDataset();
        JFreeChart chart = ChartFactory.createXYLineChart ("GPS Coordinates per device", // title
                                                            "x coordinates",             // x-axis label
                                                            "y coordinates",             // y-axis label
                                                            _xyDataset,                  // data
                                                            PlotOrientation.VERTICAL,    // plot orientation
                                                            true,                        //legend
                                                            true,                       // generate tooltips?
                                                            false);                     //generates urls?

        chart.setBackgroundPaint (Color.white);

        XYPlot plot = (XYPlot) chart.getPlot();
        plot.setBackgroundPaint (Color.lightGray);
        plot.setDomainGridlinePaint (Color.white);
        plot.setRangeGridlinePaint (Color.white);
        plot.setAxisOffset (new RectangleInsets (5.0, 5.0, 5.0, 5.0));
        plot.setDomainCrosshairVisible (true);
        plot.setRangeCrosshairVisible (true);

        return chart;
    }

    protected void createGUI (String name)
    {
        setResizable (true);
		setTitle (name);
		getContentPane().setLayout (new GridBagLayout ());
		setSize (800, 400);
        setVisible (false);
        GridBagConstraints gbc = new GridBagConstraints();

        //Panel
        JFreeChart chart = createChart();
        _chartPanel = new ChartPanel (chart);
        gbc.gridx = 0;
		gbc.gridy = 0;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (_chartPanel, gbc);
        _chartPanel.setEnabled (false);

        this.addWindowListener(new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                _dataTable.clear ();
                dispose();
                System.exit (1);
            }
        });
    }

    public JPanel createDemoPanel()
    {
        JFreeChart chart = createChart();
        return new ChartPanel(chart);
    }

    public void setFrameLocation()
	{
	    Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension d = tk.getScreenSize();
        Dimension dd = this.getPreferredSize();
        int width = ((d.width/3) - (dd.width/3));
        int height = ((d.height/4) - (dd.height/4));
        this.setLocation (width, height);
	}

    /**
     * The object that contains the x and y coordinates
     */
    public class ValueObject implements Serializable
    {
        public ValueObject()
        {
            numValues = 0;
            xPoints = new double [5000];
            yPoints = new double [5000];
        }

        public double[] xPoints;
        public double[] yPoints;
        public int numValues;
    }

    public class LogFileReaderKalman
    {
        public LogFileReaderKalman()
        {
        }

        public void init (String fileName)
            throws IOException
        {
            _br = new BufferedReader (new FileReader(fileName));
        }

        public boolean processLine (String line)
        {
            StringTokenizer st = new StringTokenizer (line);
            _ipAddr = st.nextToken ("/:");
            //System.out.println ("ipAddr = " + _ipAddr);
            st.nextToken (":"); // Skipping this one
            st.nextToken (":"); // Skipping this one
            String xValue = st.nextToken (":, ");
            _x = Double.parseDouble (xValue);
            System.out.println ("Kalman:xValue = " + xValue);
            String yValue = st.nextToken (",; ");
            _y = Double.parseDouble (yValue);
            System.out.println ("Kalman:yValue = " + yValue);
            st.nextToken (":");
            String heading = st.nextToken (": ");
            _heading = Double.parseDouble (heading);
            //System.out.println ("heading = " + heading);
            return true;
        }

        /**
         * Reads the next entry in the file
         * Returns true if an entry was read successfully, returns false no more entries were found
         */
        public boolean readNextEntry()
        {
            try {
                String line;
                while (true) {
                    if ((line = _br.readLine()) == null) {
                        return false;
                    }
                    else if (line.indexOf ("kalman") > 0) {
                        break;
                    }
                }
                processLine (line);
            }
            catch (Exception e) {
                return false;
            }
            return true;
        }

        String getIPAddress()
        {
            return _ipAddr;
        }

        double getX()
        {
            return _x;
        }

        double getY()
        {
            return _y;
        }

        double getHeading()
        {
            return _heading;
        }

        private BufferedReader _br;
        private String _ipAddr;
        private double _x;
        private double _y;
        private double _heading;
    }

    public class LogFileReader
    {
        public LogFileReader()
        {
        }

        public void init (String fileName)
            throws IOException
        {
            _br = new BufferedReader (new FileReader(fileName));
        }

        public boolean processNoKalmanLine (String line)
        {
            StringTokenizer st = new StringTokenizer (line);
            _ipAddr = st.nextToken ("/:");
            //System.out.println("*processNoKalmanLine:ipAddr = " + _ipAddr);
            st.nextToken (":"); // Skipping this one
            st.nextToken (": is ,");
            st.nextToken (" "); // Skipping this one
            st.nextToken (" "); // Skipping this one
            st.nextToken (" "); // Skipping this one
            st.nextToken (" "); // Skipping this one

            String xValue = st.nextToken (" ,");
             _x = Double.parseDouble (xValue);
            System.out.println ("xValue = " + xValue);
            st.nextToken (" "); // Skipping this one
            String yValue = st.nextToken (" ");
            _y = Double.parseDouble (yValue);
            System.out.println ("yValue = " + yValue);
            return true;
        }

        /**
         * Reads the next entry in the file
         * Returns true if an entry was read successfully, returns false no more entries were found
         */
        public boolean readNextEntry()
        {
            try {
                String line;
                while (true) {
                    if ((line = _br.readLine()) == null) {
                        return false;
                    }
                    else if (line.indexOf ("recomputed position") > 0) {
                        //System.out.println ("line = " + line);
                        break;
                    }
                }
                processNoKalmanLine (line);
            }
            catch (Exception e) {
                return false;
            }
            return true;
        }

        String getIPAddress()
        {
            return _ipAddr;
        }

        double getX()
        {
            return _x;
        }

        double getY()
        {
            return _y;
        }

        double getHeading() {
            return _heading;
        }

        private BufferedReader _br;
        private String _ipAddr;
        private double _x;
        private double _y;
        private double _heading;
    }

    //Reads the log message line when using cricket
    public class LogFileReaderCricket
    {
        public LogFileReaderCricket()
        {
        }

        public void init (String fileName)
            throws IOException
        {
            _br = new BufferedReader (new FileReader(fileName));
        }

        public boolean processCricketLine (String line)
        {        
            System.out.println ("**ProcessLine cricket line: " + line);
            //Pattern p = Pattern.compile ("/([^:]*):\\S+\\s+\\S+\\s+\\S+\\s+\\S+\\s+\\S+\\s+\\S+\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)");
            Pattern p = Pattern.compile("/([^:]*):\\S+\\s+\\S+\\s+\\S+\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)");
            Matcher m = p.matcher(line);
            if (m.matches()) {
                _ipAddr = m.group(1);
                System.out.println ("ipAddr = " + _ipAddr);
                _x =  Double.parseDouble (m.group(2));
                System.out.println("X: " + _x);
                _y = Double.parseDouble (m.group(3));
                System.out.println("Y: " + _y);
                _z = Double.parseDouble (m.group(4));

            }
            return true;
        }

        /**
         * Reads the next entry in the file
         * Returns true if an entry was read successfully, returns false no more entries were found
         */
        public boolean readNextEntry()
        {
            try {
                String line;
                while (true) {
                    if ((line = _br.readLine()) == null) {
                        return false;
                    }
                    else if (line.indexOf ("Cricket") > 0) {
                        break;
                    }
                }
                processCricketLine (line);
            }
            catch (Exception e) {
                return false;
            }
            return true;
        }

        String getIPAddress()
        {
            return _ipAddr;
        }

        double getX()
        {
            return _x;
        }

        double getY()
        {
            return _y;
        }

        private BufferedReader _br;
        private String _ipAddr;
        private double _x;
        private double _y;
        private double _z;
    }

    public static void main (String args[])
        throws Exception
    {
        //System.out.println ("args[]: " + args[0]);
        if (args.length < 2) {
            System.out.println ("usage: <file name> <kalman or nokalman or cricket>" );
            return;
        }

        PathViewer viewer = new PathViewer ("Path Viewer");
        viewer.pack();
        viewer.setFrameLocation();
        viewer.setVisible (true);
        if (args.length == 2) {
            viewer.go (args[0], args[1]);
        }
    }

    //Variables
    protected LogFileReaderCricket _lfrc;
    protected LogFileReaderKalman _lfrk;
    protected LogFileReader _lfr;
    protected ChartPanel _chartPanel;
    protected NodePositionsDataset _xyDataset;
    protected Hashtable _dataTable;
    protected Vector _valueVector;
}
