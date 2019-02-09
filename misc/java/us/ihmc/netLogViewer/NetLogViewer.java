package us.ihmc.netLogViewer;

import java.awt.*;
import java.awt.event.*;

import javax.swing.*;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.ListSelectionEvent;
import javax.swing.border.EtchedBorder;

import java.io.FileWriter;
import java.io.PrintWriter;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.util.Vector;
import java.util.Enumeration;

/**
 * author: Maggie Breedy
 * Date: Nov 6, 2007
 * Time: 3:20:33 PM
 * @version $Revision$
 */
public class NetLogViewer extends JDialog
{
    public NetLogViewer (int port)
        throws Exception
    {
        createGUI();
        setFrameLocation();
        _filterItemVector = new Vector();
        _dgSocket = new DatagramSocket (port);
        _pwLogFile = new PrintWriter (new FileWriter ("NetLogViewer.log"));
    }

    /**
     * Checks if the line received from the socket contains kalman or recomputed position or cricket so the
     * path viewer can process the line.
     */
    public void go()
    {
        try {
            while (true) {
                DatagramPacket dgPacket = new DatagramPacket (new byte [65535], 65535);
                _dgSocket.receive (dgPacket);
                String logMsg = dgPacket.getAddress() + ":" + dgPacket.getPort() + " " + new String (dgPacket.getData(),
                                0, dgPacket.getLength());
                System.out.println ("**NetLogViewer logMsg: " + logMsg);
                _pwLogFile.print (logMsg);
                _pwLogFile.flush();
                // Check if this line has "kalman" and call the processKalmanLine                
                if ((logMsg.indexOf ("kalman") > 0) && (_pathKalmanViewer != null)) {
                    _pathKalmanViewer.processKalmanLine (logMsg);
                }
                // Check if this line has "recomputed position" and call the processNoKalmanLine
                if ((logMsg.indexOf ("recomputed position") > 0) && (_pathViewer != null)) {
                    _pathViewer.processNoKalmanLine (logMsg);
                }
                if ((logMsg.indexOf ("Cricket") > 0) && (_pathViewer != null)) {
                    System.out.println ("**NetLogViewer logMsg: " + logMsg);
                    System.out.println ("**NetLogViewer _pathViewer: " + _pathViewer);
                    _pathViewer.processCricketLine (logMsg);
                }

                if (_filterItemVector.isEmpty()) {
                    _msgListModel.addElement (logMsg);
                }
                else {
                    //System.out.println ("_includeFilterItems: " + _includeFilterItems);
                    //System.out.println ("_filterItemVector: " + _filterItemVector);
                    if (_includeFilterItems) {
                        for (Enumeration e = _filterItemVector.elements (); e.hasMoreElements ();) {
                            String str = (String) e.nextElement ();
                            //System.out.println ("string: " + str);
                            if (logMsg.indexOf (str) > 0) {
                                //System.out.println ("Including: " + logMsg);
                                _msgListModel.addElement (logMsg);
                            }
                        }
                        _includeFilterItems = false;
                    }
                    else {
                        for (Enumeration e = _filterItemVector.elements (); e.hasMoreElements ();) {
                            String str = (String) e.nextElement ();
                            //System.out.println ("string2: " + str);
                            if (logMsg.indexOf (str) > 0) {
                                //System.out.println ("excluding: " + logMsg);
                                continue;
                            }
                            else {
                                 //System.out.println ("Including: " + logMsg);
                                _msgListModel.addElement (logMsg);
                            }
                            _filterItemVector.removeElement (str);
                        }
                    }
                }
                int position = _msgListModel.getSize();
                _msgList.ensureIndexIsVisible(position - 1); //Show last row of the list
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static final int DEFAULT_PORT = 1300;

    // Creates the logger viewer
    protected void createGUI()
    {
        setResizable (true);
		setTitle ("Network Logging Viewer");
		getContentPane().setLayout (new GridBagLayout ());
		setSize (600, 700);
        setVisible (false);
        GridBagConstraints gbc = new GridBagConstraints();

        //Panel1
        JPanel panel = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
		gbc.gridy = 0;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (panel, gbc);

        _msgList = new JList();
        _msgListModel = new DefaultListModel();
        _msgList.setModel (_msgListModel);
        JScrollPane sp1 = new JScrollPane (_msgList);
        _msgList.setBorder (new EtchedBorder ());
        sp1.setViewportView (_msgList);
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        panel.add (sp1, gbc);

         //Menu items
        JMenuBar menuBar = new JMenuBar();
        setJMenuBar (menuBar);

        JMenu fileMenu = new JMenu();
        fileMenu.setBackground (java.awt.Color.lightGray);
        fileMenu.setText ("File");
        fileMenu.setActionCommand ("File");
        fileMenu.setMnemonic ((int)'F');
        menuBar.add (fileMenu);

        JMenu editMenu = new JMenu();
        editMenu.setBackground (java.awt.Color.lightGray);
        editMenu.setText ("Edit");
        editMenu.setActionCommand ("Edit");
        editMenu.setMnemonic ((int)'E');
        menuBar.add (editMenu);

        JMenu windowMenu = new JMenu();
        windowMenu.setBackground (java.awt.Color.lightGray);
        windowMenu.setText ("Window");
        windowMenu.setActionCommand ("Window");
        windowMenu.setMnemonic ((int)'W');
        menuBar.add (windowMenu);

        JMenuItem settings = new JMenuItem();
        settings.setText ("Settings");
        settings.setActionCommand ("Settings");
        settings.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_S, KeyEvent.CTRL_MASK));
        settings.setMnemonic ((int)'S');
        fileMenu.add (settings);
        JSeparator JSeparator1 = new JSeparator();
        fileMenu.add (JSeparator1);

        JMenuItem exitMenuItem = new JMenuItem();
        exitMenuItem.setText ("Exit");
        exitMenuItem.setActionCommand ("Exit");
        exitMenuItem.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_E, KeyEvent.CTRL_MASK));
        exitMenuItem.setMnemonic ((int)'E');
        fileMenu.add (exitMenuItem);

        JMenuItem filterMenuItem = new JMenuItem();
        filterMenuItem.setText ("Filter");
        filterMenuItem.setActionCommand ("Filter");
        filterMenuItem.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_F, KeyEvent.CTRL_MASK));
        filterMenuItem.setMnemonic ((int)'F');
        editMenu.add (filterMenuItem);

        JSeparator JSeparator2 = new JSeparator();
        fileMenu.add (JSeparator2);

        JMenuItem clearMenuItem = new JMenuItem();
        clearMenuItem.setText ("Clear");
        clearMenuItem.setActionCommand ("Clear");
        clearMenuItem.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_C, KeyEvent.CTRL_MASK));
        clearMenuItem.setMnemonic ((int)'C');
        editMenu.add (clearMenuItem);

        JMenuItem kalmanMenuItem = new JMenuItem();
        kalmanMenuItem.setText ("Show Kalman");
        kalmanMenuItem.setActionCommand ("Show Kalman");
        kalmanMenuItem.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_K, KeyEvent.CTRL_MASK));
        kalmanMenuItem.setMnemonic ((int)'K');
        windowMenu.add (kalmanMenuItem);
        windowMenu.add (JSeparator1);

        JMenuItem noKalmanMenuItem = new JMenuItem();
        noKalmanMenuItem.setText ("Show No Kalman");
        noKalmanMenuItem.setActionCommand ("Show No Kalman");
        noKalmanMenuItem.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_N, KeyEvent.CTRL_MASK));
        noKalmanMenuItem.setMnemonic ((int)'N');
        windowMenu.add (noKalmanMenuItem);
        windowMenu.add (JSeparator1);

        JMenuItem cricketMenuItem = new JMenuItem();
        cricketMenuItem.setText ("Show Cricket");
        cricketMenuItem.setActionCommand ("Show Cricket");
        cricketMenuItem.setAccelerator (KeyStroke.getKeyStroke (KeyEvent.VK_T, KeyEvent.CTRL_MASK));
        cricketMenuItem.setMnemonic ((int)'T');
        windowMenu.add (cricketMenuItem);
        windowMenu.add (JSeparator1);

        kalmanMenuItem.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _pathKalmanViewer = new PathViewer ("Path Kalman Viewer");
                _pathKalmanViewer.pack();
                _pathKalmanViewer.setFrameLocation();
                _pathKalmanViewer.setVisible(true);
            }
         });

        noKalmanMenuItem.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _pathViewer = new PathViewer ("GPS Plotter");
                _pathViewer.pack();
                _pathViewer.setFrameLocation();
                _pathViewer.setVisible(true);
            }
         });

        cricketMenuItem.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _pathViewer = new PathViewer ("Cricket Plotter");
                _pathViewer.pack();
                _pathViewer.setFrameLocation();
                _pathViewer.setVisible(true);
            }
         });

        settings.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {

            }
         });

        exitMenuItem.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                dispose();
                System.exit (1);
            }
         });

        filterMenuItem.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                FilterListDialog dialog = new FilterListDialog (_filterItemVector);
                _deleteBtn.setEnabled (false);
                dialog.setVisible(true);
            }
         });

        clearMenuItem.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _msgListModel.removeAllElements ();
            }
         });

        this.addWindowListener(new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                dispose();
                System.exit (1);
            }
        });
    }

    public void setFrameLocation()
	{
	    Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension d = tk.getScreenSize();
        Dimension dd = this.getPreferredSize();
        int width = ((d.width/3) - (dd.width/3));// + 80;
        int height = ((d.height/4) - (dd.height/4));// + 80;
        this.setLocation (width, height);
	}

    public static void main (String args[])
        throws Exception
    {
        int portToUse = DEFAULT_PORT;
        int i = 0;
        while (i < args.length) {
            if ((args[i].equalsIgnoreCase ("-p")) || (args[i].equalsIgnoreCase ("-port"))) {
                i++;
                if (i < args.length) {
                    portToUse = Integer.parseInt (args[i]);
                }
            }
            i++;
        }
        System.out.println ("NetLogViewer: using port " + portToUse);
        NetLogViewer viewer = new NetLogViewer (portToUse);
        viewer.setVisible (true);
        viewer.go();
    }

    protected class FilterListDialog extends JDialog
    {
        public FilterListDialog (Vector filterItemVector)
        {
            createGui();
            System.out.println ("-->>>filterItemVector " + filterItemVector);
            Enumeration e = filterItemVector.elements();
            while (e.hasMoreElements()) {
                _filterListModel.addElement (e.nextElement());
            }
        }

        protected void createGui()
        {
            setResizable (true);
            setTitle ("Filter Frame");
            getContentPane().setLayout (new GridBagLayout());
            setSize (350, 550);
            setVisible (false);
            setModal (true);
            Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
            setBounds ((screenSize.width-350)/2, (screenSize.height-500)/2, 350, 500);
            GridBagConstraints gbc = new GridBagConstraints();

            //TopPanel component
            gbc = new GridBagConstraints();
            JPanel topPanel = new JPanel (new GridBagLayout());
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.gridwidth = 1;
            gbc.gridheight = 1;
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.NORTH;
            gbc.weightx = 0.8;
            gbc.weighty = 0.8;
            gbc.insets = new Insets (0,0,0,0);
            getContentPane().add (topPanel,gbc);

            JPanel midPanel = new JPanel (new GridBagLayout());
            gbc.gridx = 0;
            gbc.gridy = 1;
            gbc.gridwidth = 1;
            gbc.gridheight = 1;
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.SOUTH;
            gbc.weightx = 0.1;
            gbc.weighty = 0.1;
            gbc.insets = new Insets (0,0,0,0);
            getContentPane().add (midPanel,gbc);
            
            JPanel bottonPanel = new JPanel (new GridBagLayout());
            gbc.gridx = 0;
            gbc.gridy = 2;
            gbc.gridwidth = 1;
            gbc.gridheight = 1;
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.SOUTH;
            gbc.weightx = 0.1;
            gbc.weighty = 0.1;
            gbc.insets = new Insets (0,0,0,0);
            getContentPane().add (bottonPanel,gbc);

            //TopPanel components
            gbc = new GridBagConstraints();
             //radio buttons
            JPanel buttonsPanel = new JPanel (new GridBagLayout());
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.anchor = GridBagConstraints.SOUTHEAST;
            gbc.weightx = 0.1;
            gbc.gridwidth = 1;
            gbc.insets = new Insets (5, 5, 5, 5);
            topPanel.add (buttonsPanel,gbc);

            JRadioButton incRadioButton = new JRadioButton();
            JRadioButton excRadioButton = new JRadioButton();
            buttonsPanel.setBorder (new EtchedBorder());
            incRadioButton.setText ("Include");
            incRadioButton.setSelected (true);
            incRadioButton.setToolTipText ("Include the names in the list");            
            gbc.gridx = 0;
            buttonsPanel.add (incRadioButton, gbc);
            gbc.gridx = 1;
            buttonsPanel.add (excRadioButton, gbc);
            excRadioButton.setText ("Exclude");
            excRadioButton.setToolTipText ("Exclude the names from the list");
            ButtonGroup buttonGroup = new ButtonGroup();
            buttonGroup.add (incRadioButton);
            buttonGroup.add (excRadioButton);

            incRadioButton.addActionListener (new ActionListener() {
                public void actionPerformed (ActionEvent e) {
                    _includeFilterItems = true;
                }
            });

            excRadioButton.addActionListener (new ActionListener() {
                public void actionPerformed (ActionEvent e) {
                    _includeFilterItems = false;
                }
            });

            _filterList = new JList();
            _filterListModel = new DefaultListModel();
            _filterList.setModel (_filterListModel);
            JScrollPane sp1 = new JScrollPane (_filterList);
            _filterList.setBorder (new EtchedBorder ());
            sp1.setViewportView (_filterList);
            gbc.fill = GridBagConstraints.BOTH;
            gbc.anchor = GridBagConstraints.SOUTH;
            gbc.insets = new Insets (5,5,5,5);
            gbc.gridx = 0;
            gbc.gridy = 1;
            gbc.weightx = 1.0;
            gbc.weighty = 1.0;
            topPanel.add (sp1, gbc);

            //MidPanel components
             gbc = new GridBagConstraints();
            _addField = new JTextField ();
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.anchor = GridBagConstraints.NORTHWEST;
            gbc.weightx = 1.0;
            gbc.ipady = 7;
            gbc.insets = new Insets (5, 5, 5, 5);
            _addField.requestFocus();
            midPanel.add (_addField, gbc);

            _addField.addKeyListener (new KeyAdapter() {
                public void keyPressed (KeyEvent event)
                {
                    if (event.getKeyCode() == KeyEvent.VK_ENTER) {
                        _filterListModel.addElement (_addField.getText());
                        _addField.setText ("");
                        _addField.requestFocus ();
                    }
                }
             });

            _filterList.addListSelectionListener (new ListSelectionListener () {
                public void valueChanged (ListSelectionEvent e) {
                    //_selItems = _filterList.getSelectedValues ();
                    if (_filterList.isSelectionEmpty ()) {
                        _deleteBtn.setEnabled (false);
                    }
                    else {
                        _deleteBtn.setEnabled (true);
                    }
                }
            });

            //BottonPanel components
            gbc = new GridBagConstraints();
            JButton addBtn = new JButton ("Add");
            addBtn.setEnabled (true);
            addBtn.setToolTipText ("Add an item to filter");
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.anchor = GridBagConstraints.EAST;
            gbc.weightx = 0.05;
            gbc.insets = new Insets (5, 5, 5, 5);
            bottonPanel.add (addBtn, gbc);

            addBtn.addActionListener (new ActionListener() {
                public void actionPerformed (ActionEvent e) {
                    _filterListModel.addElement (_addField.getText());
                    _addField.setText ("");
                    _addField.requestFocus ();
                }
            });

           addBtn.addKeyListener (new KeyAdapter() {
                public void keyPressed (KeyEvent event)
                {
                    if (event.getKeyCode() == KeyEvent.VK_ENTER) {
                        _filterListModel.addElement (_addField.getText());
                        _addField.setText ("");
                        _addField.requestFocus ();
                    }
                }
            });

            _deleteBtn = new JButton ("Delete");
            _deleteBtn.setEnabled (true);
            gbc.gridx = 1;
            gbc.gridy = 0;
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.anchor = GridBagConstraints.CENTER;
            gbc.weightx = 0.1;
            gbc.insets = new Insets (5, 5, 5, 5);
            bottonPanel.add (_deleteBtn, gbc);

            JButton okBtn = new JButton ("Ok");
            okBtn.setEnabled (true);
            gbc.gridx = 0;
            gbc.gridy = 1;
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.anchor = GridBagConstraints.CENTER;
            gbc.weightx = 0.1;
            gbc.insets = new Insets (5, 5, 5, 5);
            bottonPanel.add (okBtn, gbc);

            JButton cancelBtn = new JButton ("Cancel");
            cancelBtn.setEnabled (true);
            gbc.gridx = 1;
            gbc.gridy = 1;
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.anchor = GridBagConstraints.CENTER;
            gbc.weightx = 0.1;
            gbc.insets = new Insets (5, 5, 5, 5);
            bottonPanel.add (cancelBtn, gbc);

            _deleteBtn.addActionListener (new ActionListener() {
                public void actionPerformed (ActionEvent e) {
                    int index = _filterList.getSelectedIndex ();
                    _filterListModel.removeElementAt (index);
                }
            });

            okBtn.addActionListener (new ActionListener() {
                public void actionPerformed (ActionEvent e) {
                    int listSize = _filterListModel.size();
                    _filterItemVector.removeAllElements();
                    for (int i=0; i<listSize; i++) {
                        _filterItemVector.addElement (_filterListModel.getElementAt (i));
                    }
                    _includeFilterItems = _include;
                    dispose();
                }
            });

            cancelBtn.addActionListener (new ActionListener() {
                public void actionPerformed (ActionEvent e) {
                    dispose();
                }
            });
        }
        protected JList _filterList;
        protected DefaultListModel _filterListModel;
        protected boolean _include;
    }

    // Variables
    protected PathViewer _pathKalmanViewer;
    protected PathViewer _pathViewer;
    protected JList _msgList;
    protected DatagramSocket _dgSocket;
    protected PrintWriter _pwLogFile;
    protected DefaultListModel _msgListModel;
    protected JButton _deleteBtn;
    protected JTextField _addField;
    protected Object[] _selItems;
    protected Vector _filterItemVector;
    protected boolean _includeFilterItems = false;
}
