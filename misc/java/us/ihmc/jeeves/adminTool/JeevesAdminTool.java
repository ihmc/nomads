package us.ihmc.jeeves.adminTool;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.Serializable;
import java.security.*;

import java.net.InetAddress;
import java.net.Socket;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Vector;

import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.*;
import java.awt.Insets;
import java.awt.Toolkit;
import javax.swing.*;
import javax.swing.table.*;
import javax.swing.border.EtchedBorder;
import javax.swing.DefaultListModel;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.filechooser.FileFilter;

import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.MocketStatusListener;

import us.ihmc.util.MocketCommHelper;
import us.ihmc.util.Base64Transcoders;

import us.ihmc.comm.CommHelper;
import us.ihmc.ds.Diff;

/**
 * NewTool for jeeves
 *
 * @author  Maggie Breedy
 * @version $Revision$
 *
 **/

public class JeevesAdminTool extends JFrame
{
    public JeevesAdminTool()
    {
        _serviceName = System.getProperty ("serviceName");
        try {
            _controlPort = Integer.parseInt (System.getProperty ("CtrlPort"));
        }
        catch (Exception ex) {} //intentionally left blank.

        try {
            _finderPort = Integer.parseInt (System.getProperty ("FinderPort") );
        }
        catch (Exception ex) {} //intentionally left blank.

        System.out.println ("JeevesAdminTool :: CtrlPort == [" + _controlPort + "]" );
        System.out.println ("JeevesAdminTool :: FinderPort == [" + _finderPort + "]");
        System.out.println ("JeevesAdminTool :: serviceName == [" + _serviceName + "]");
        
        /*LoggerWrapper logger = new LoggerWrapper();
        String path = System.getProperty ("user.dir") + System.getProperty ("file.separator") + "logger.cfg";
        System.out.println ("->Creating LoggerWrapper:path:workingDir: " + path);
        logger.initialize (confLog);*/
                
        _fileRepositoryVector = new Vector();
        createGui();
        _jlLog.setModel (new DefaultListModel());
        _objectVector = new Vector();
        _objectDirVector = new Vector();
        _updateDirsVector = new Vector();
        _downloadFileVector = new Vector();
        _relAbsTable = new Hashtable();
        setFrameLocation();
    }
    
    /**
     * Build the Gui components
     */
    protected void createGui()
    {
        setResizable (true);
		setTitle ("Jeeves Admin Tool");
		getContentPane().setLayout (new GridBagLayout());
		setSize (700, 500);
        setVisible (false);
        GridBagConstraints gbc = new GridBagConstraints();
        
        //Main Panels
        JPanel panel1 = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
		gbc.gridy = 0;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 0.8;
        gbc.weighty = 0.3;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (panel1,gbc);
        
        JPanel panel2 = new JPanel (new GridBagLayout());
        gbc.gridx = 1;
		gbc.gridy = 0;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.weighty = 0.3;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (panel2,gbc);
        
        JPanel panel3 = new JPanel (new GridBagLayout());
        gbc.gridx = 2;
		gbc.gridy = 0;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.3;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (panel3,gbc);
        
        JPanel panel4 = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
		gbc.gridy = 1;
        gbc.gridwidth = 3;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.weightx = 0.3;
        gbc.weighty = 0.8;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (panel4,gbc);
        
        //Panel1 component.
        gbc = new GridBagConstraints();
        Vector columnNames = new Vector (3);
        columnNames.addElement ("Platform");
        columnNames.addElement ("Port");
        columnNames.addElement ("Status");
        
        _platfTableModel = new PlatformTableModel (_rowDataVector, columnNames);
        _platfTable = new JTable (_platfTableModel) {
            public boolean isCellEditable (int row, int column) {
                return false;
            }                        
        };
        
        for (int i = 0; i < 2; i++) {
            TableColumn tableMod = _platfTable.getColumnModel().getColumn(i);
            if (i == 0) {
                tableMod.setPreferredWidth (155);
            }
            else if (i == 1) {
                tableMod.setPreferredWidth (50);
            }
            else {
                tableMod.setPreferredWidth (62);
            }
        }
        
        JScrollPane ctbScrollPane = new JScrollPane (_platfTable);
        ctbScrollPane.setViewportView (_platfTable);
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (13,13,13,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        panel1.add (ctbScrollPane, gbc);
        
        _platfTable.setCellSelectionEnabled (false);
        _platfTable.setShowGrid (true);
        _platfTable.setShowHorizontalLines (true);
        _platfTable.setShowVerticalLines (false);
        _platfTable.setColumnSelectionAllowed (false);
        _platfTable.setRowSelectionAllowed (true);
        
        //Panel2 component.
        gbc = new GridBagConstraints();
        JButton addPltBtn = new JButton ("Add Platform...");
        //addPltBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        addPltBtn.setEnabled (true);
        addPltBtn.setToolTipText ("Add a Platform by Entering the Address");
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (13, 13, 5, 13);
        panel2.add (addPltBtn, gbc);
        
        JButton loadBtn = new JButton ("Load Platforms...");
        //loadBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        loadBtn.setEnabled (true);
        loadBtn.setToolTipText ("Load List of Platforms from a File");
        gbc.gridx = 1;
        gbc.gridy = 1;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (loadBtn, gbc);
        
        JButton saveBtn = new JButton ("Save Platforms...");
        //saveBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        saveBtn.setEnabled (true);
        saveBtn.setToolTipText ("Save List of Platforms to a File");
        gbc.gridx = 1;
        gbc.gridy = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (saveBtn, gbc);
        
        JButton startBtn = new JButton ("Start Service");
        //startBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        startBtn.setEnabled (true);
        startBtn.setToolTipText ("Start Selected Platforms");
        gbc.gridx = 1;
        gbc.gridy = 3;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (startBtn, gbc);
        
        JButton stopBtn = new JButton ("Stop Service");
        //stopBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        stopBtn.setEnabled (true);
        stopBtn.setToolTipText ("Stop Selected Platforms");
        gbc.gridx = 1;
        gbc.gridy = 4;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (stopBtn, gbc);
        
        JButton updateFileBtn = new JButton ("Update File...");
        //updateFileBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        updateFileBtn.setEnabled (true);
        updateFileBtn.setToolTipText ("Update Files on Platforms");
        gbc.gridx = 1;
        gbc.gridy = 5;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (updateFileBtn, gbc);
        
        JButton downFileBtn = new JButton ("Download File...");
        //downFileBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        downFileBtn.setEnabled (true);
        downFileBtn.setToolTipText ("Download Files on Platforms");
        gbc.gridx = 1;
        gbc.gridy = 6;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (downFileBtn, gbc);
        
        JButton updateDirBtn = new JButton ("Update Directories...");
        //updateDirBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        updateDirBtn.setEnabled (true);
        updateDirBtn.setToolTipText ("Update Directories on Platforms");
        gbc.gridx = 1;
        gbc.gridy = 7;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (updateDirBtn, gbc);
        
        JButton removeBtn = new JButton ("Remove Platforms...");
        //removeBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        removeBtn.setEnabled (true);
        removeBtn.setToolTipText ("Removes Platforms");
        gbc.gridx = 1;
        gbc.gridy = 8;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 13, 13);
        panel2.add (removeBtn, gbc);
        
        //Panel3 components
        gbc = new GridBagConstraints();
        _findBtn = new JButton ("Find...");
        //_findBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        _findBtn.setEnabled (true);
        _findBtn.setToolTipText ("Find Platforms by Broadcast");
        gbc.gridx = 2;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (13, 13, 5, 13);
        panel3.add (_findBtn, gbc);
        
        _upStatusBtn = new JButton ("Update Status");
        //_upStatusBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        _upStatusBtn.setEnabled (true);
        _upStatusBtn.setToolTipText ("Update Status of All Platforms");
        gbc.gridx = 2;
        gbc.gridy = 1;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel3.add (_upStatusBtn, gbc);
        
        _upStSelBtn = new JButton ("Update Status (Sel)");
        //_upStSelBtn.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        _upStSelBtn.setEnabled (true);
        _upStSelBtn.setToolTipText ("Update Status of Selected Platforms");
        gbc.gridx = 2;
        gbc.gridy = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        //gbc.ipady = 5;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel3.add (_upStSelBtn, gbc);
        
        //Update radio buttons
        JPanel buttonsPanel = new JPanel (new GridBagLayout());
        gbc.gridx = 2;
		gbc.gridy = 3;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.weightx = 0.1;
        gbc.gridwidth = 2;
		gbc.insets = new Insets (5, 13, 5, 13);
        panel3.add (buttonsPanel,gbc);
        
        JRadioButton pingRadioButton = new JRadioButton();
        JRadioButton connRadioButton = new JRadioButton();
		buttonsPanel.setBorder (new EtchedBorder());
        pingRadioButton.setText ("Ping");
        pingRadioButton.setToolTipText ("Use Ping to Connect to Platforms");
        JLabel updateLbl = new JLabel ("Update Using");
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
		gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets (5, 5, 5, 5);
        buttonsPanel.add (updateLbl,gbc);
        gbc.gridy = 1;
        buttonsPanel.add (pingRadioButton,gbc);
        gbc.gridy = 2;
        buttonsPanel.add (connRadioButton, gbc);
        connRadioButton.setText ("Connection");
        connRadioButton.setSelected (true);
        connRadioButton.setToolTipText ("Use TCP Sockets to Connect to Platforms");
        ButtonGroup buttonGroup = new ButtonGroup();
		buttonGroup.add (pingRadioButton);
        buttonGroup.add (connRadioButton);
        
        //Connect radio buttons
        JPanel buttonsPanel2 = new JPanel (new GridBagLayout());
        gbc.gridx = 2;
		gbc.gridy = 5;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.weightx = 0.1;
        gbc.gridwidth = 2;
		gbc.insets = new Insets (5, 13, 5, 13);
        panel3.add (buttonsPanel2,gbc);
        
        JRadioButton mockRadioButton = new JRadioButton();
        JRadioButton sockRadioButton = new JRadioButton();
		buttonsPanel2.setBorder (new EtchedBorder());
        mockRadioButton.setText ("Mockets");
        mockRadioButton.setSelected (true);
        mockRadioButton.setToolTipText ("Use Mockets to Connect to Platforms");
        JLabel connLbl = new JLabel ("Connect Using");
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
		gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets (5, 5, 5, 5);
        buttonsPanel2.add (connLbl,gbc);
        gbc.gridy = 1;
        buttonsPanel2.add (mockRadioButton,gbc);
        gbc.gridy = 2;
        buttonsPanel2.add (sockRadioButton, gbc);
        sockRadioButton.setText ("Sockets");
        sockRadioButton.setToolTipText ("Use TCP Sockets to Connect to Platforms");
        ButtonGroup buttonGroup2 = new ButtonGroup();
		buttonGroup2.add (mockRadioButton);
        buttonGroup2.add (sockRadioButton);
        
        //Panel4 component
        gbc = new GridBagConstraints();
        _jlLog = new JList();
        JScrollPane sp = new JScrollPane (_jlLog);
        _jlLog.setBorder (new EtchedBorder());
        sp.setViewportView (_jlLog);
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets (5,13,5,13);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1.0;
        gbc.weighty = 0.8;
        panel4.add (sp, gbc);
        
        //Popup Menu
        //_platformPopupMenu = new JPopupMenu();
                
        addPltBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               addPlatformToTable();
            }
        });
        
        loadBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               loadPlatform();
            }
        });
        
        saveBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               savePlatform();
            }
        });
        
        startBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               startPlatform();
            }
        });
        
        stopBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               stopPlatform();
            }
        });
        
        updateFileBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               updateFileAction();
            }
        });
        
        downFileBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               downloadFileAction();
            }
        });
        
        updateDirBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               updateDirectoryAction();
            }
        });
        
        removeBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               removePlatforms();
            }
        });
        
        _findBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               findPlatforms();
            }
        });
        
        _upStatusBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               _upStSelBtn.setSelected  (false);
               updateStatusAction();
            }
        });
        
        _upStSelBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _upStatusBtn.setSelected  (false);
                updateStatusSelectedAction();
            }
        });
        
        pingRadioButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                pingPlatforms();
            }
        });
        
        connRadioButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                connectToPlatforms();
            }
        });
        
        mockRadioButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                connWithMockets();
            }
        });
        
        sockRadioButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                connWithSockets();
            }
        });
        
        // Add Table Mouse Listener
        _platfTable.addMouseListener (new MyMouseListener());
        
        this.addWindowListener(new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                _fileRepositoryVector.removeAllElements();
                _downloadFileVector.removeAllElements();
                _updateDirsVector.removeAllElements();
                dispose();
                System.exit (1);
            }
         });
    }
    
    /**
     * Add a new platform to the table.
     */
    protected void addPlatformToTable()
    {
        AddPlatform ap = new AddPlatform();
        ap.setVisible (true);
        if (ap.update()) {
            setCursorToWait (true);
            try {
                JeevesFinder.PlatformInfo pi = new JeevesFinder.PlatformInfo();
                pi.addr = InetAddress.getByName (ap.getValue());
                //System.out.println ("-->>Adding pi.addr: " + pi.addr);
                pi.status = "Unknown";
                String port = ap.getPort();
                pi.port = Integer.parseInt (port);
                if (!_loadedPlatformList.isEmpty()) {
                    for (Enumeration en = _loadedPlatformList.elements(); en.hasMoreElements();) {
                        JeevesFinder.PlatformInfo pfpi = (JeevesFinder.PlatformInfo) en.nextElement();
                        System.out.println ("-->>loadedPlatformList pfpi: " + pfpi.toString());
                        //System.out.println ("-->>pi: " + pi.toString());
                        if (!pi.equals (pfpi)) {
                            System.out.println ("-->>Add to _loadedPlatformList: " + pi.toString());
                            _loadedPlatformList.addElement (pi);
                        }
                    }
                }
                else {
                    //System.out.println ("-->>Adding to _loadedPlatformList: " + pi.toString());
                    _loadedPlatformList.addElement (pi);
                }
                if (_newList == null || _newList.isEmpty()) {
                    //System.out.println ("-->>Updating the table with _loadedPlatformList");
                    updateTable (_loadedPlatformList);
                }
                else {
                    if (!_newList.isEmpty()) {
                        Enumeration enu = _newList.elements();
                        while (enu.hasMoreElements()) {
                            JeevesFinder.PlatformInfo pii2 = (JeevesFinder.PlatformInfo) enu.nextElement();
                            if (!pii2.equals (pi)) {
                                _newList.addElement (pi);
                                break;
                            }
                            else {
                                //System.out.println ("JeevesFinder:Platfs are same!");
                            }
                        }
                        //System.out.println ("-->>Updating the table with _newList");
                        updateTable (_newList);
                    }
                    setCursorToWait (false);
                }
                updatePlatformList();
            }
            catch (Exception e) {
                JOptionPane.showMessageDialog (this, "Error adding platform - Exception: " + e, "Error", JOptionPane.ERROR_MESSAGE);
            }
        }
    }
    
    /**
     * Update the platform information in the table 
     */
    protected void updateTable (Vector dataVector)
    {
        clearTable();
        System.out.println ("Adding row to table!");
        for (Enumeration e = dataVector.elements(); e.hasMoreElements();) {
            JeevesFinder.PlatformInfo pi = (JeevesFinder.PlatformInfo) e.nextElement();
            Vector v = new Vector();
            v.addElement (pi.addr);
            v.addElement (String.valueOf(pi.port));
            v.addElement (pi.status);
            _platfTableModel.addRow (v);
        }
    }
    
    /**
     * Remove elements from the table
     */
    protected void clearTable()    
    {
        int rows = _platfTableModel.getRowCount();
        for (int i = rows - 1; i >= 0; i--) {
            _platfTableModel.removeRow(i);    
        }
    }
    
    /**
     * Load platforms from a file
     */
    protected void loadPlatform()
    {
        JFileChooser jfc = new JFileChooser();
        int result = jfc.showOpenDialog (this);
        if (result == JFileChooser.APPROVE_OPTION) {
            File selectedFile = jfc.getSelectedFile();
            try {                
                _loadedPlatformList = new Vector();
                BufferedReader br = new BufferedReader (new FileReader (selectedFile));
                String line = null;
                String name = null;
                int port = 0;
                while ((line = br.readLine()) != null) {
                    StringTokenizer st = new StringTokenizer (line, " ");
                    while (st.hasMoreTokens()) {
                        name = st.nextToken();
                        //System.out.println ("-->>LoadPlatform:name: " + name); 
                        port = Integer.parseInt (st.nextToken());
                    }
                    JeevesFinder.PlatformInfo pi = new JeevesFinder.PlatformInfo();
                    int i = name.lastIndexOf ("/");
                    pi.addr = InetAddress.getByName (name.substring (i+1));
                    pi.port = port;
                    pi.status = "unknown";
                    _loadedPlatformList.addElement (pi);
                }
            }
            catch (Exception e) {
                //e.printStackTrace();
                JOptionPane.showMessageDialog (this, "Failed to load list of platforms from file <" + selectedFile + ">; Exception = " + e, "Error", JOptionPane.ERROR_MESSAGE);
            }
            updateTable (_loadedPlatformList);
        }
    }
    
    /**
     * Save platforms to a file.
     */
    protected void savePlatform()
    {
        JFileChooser chooser = new JFileChooser();
        JeevesFileFilter filter = new JeevesFileFilter();
        filter.addExtension ("plf");
        filter.setDescription ("List of Platforms");
        chooser.setFileFilter (filter);
        String filename = null;
        if (chooser.showOpenDialog (this) == JFileChooser.APPROVE_OPTION) {
            String fileSelected = chooser.getSelectedFile().getPath() + ".plf";
            try {
                PrintWriter pw = new PrintWriter (new FileWriter (fileSelected));
                Vector rowDataFromTable = _platfTableModel.getDataVector(); //vector of vectors of data values
                Vector platformList = buildStringFromRow (rowDataFromTable);
                for (Enumeration e = platformList.elements(); e.hasMoreElements();) {
                    String platfStr = (String) e.nextElement();
                    pw.println (platfStr);
                }
                pw.close();
            }
            catch (Exception e) {
                JOptionPane.showMessageDialog (this, "Failed to save list of platforms to file <" + fileSelected + 
                                               ">; Exception = " + e, "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            JOptionPane.showMessageDialog (this, "File saved successfully", "Confirmation", JOptionPane.INFORMATION_MESSAGE);
        }
    }
    
    /**
     * Start Jeeves in a selected platform
     */
    protected void startPlatform()
    {
        int[] selPIs = _platfTable.getSelectedRows();    
        for (int i = 0; i < selPIs.length; i++) {
            Vector v = (Vector) _platfTableModel.getRowElements (selPIs[i]);            
            JeevesFinder.PlatformInfo pi = new JeevesFinder.PlatformInfo();
            pi.addr = (InetAddress) v.elementAt (0);
            String portStr = (String) v.elementAt (1);
            pi.port = Integer.parseInt (portStr);
            String status = (String) v.elementAt (2);
            pi.status = status;
            if (status.equalsIgnoreCase ("stopped")) {
                //System.out.println ("-->StartPlatform:Current status: " + pi.status);
                if (startJeeves (pi)) {
                    pi.status = "running";
                    v.removeElementAt (2);
                    v.insertElementAt (pi.status, 2);
                    _platfTableModel.removeRow (selPIs[i]);
                    _platfTableModel.insertRow (selPIs[i], v);
                    //_platfTableModel.addRow (v);
                }
            }
            else {
                addLogMsg ("Skipping " + pi + " which is not stopped");
            }
        }
        _platfTable.revalidate();
    }
    
    /**
     * Stop Jeeves in a selected platform
     */
    protected void stopPlatform()
    {
        int[] selPIs = _platfTable.getSelectedRows();    
        for (int i = 0; i < selPIs.length; i++) {
            Vector v = (Vector) _platfTableModel.getRowElements (selPIs[i]);           
            JeevesFinder.PlatformInfo pi = new JeevesFinder.PlatformInfo();
            pi.addr = (InetAddress) v.elementAt (0);
            String portStr = (String) v.elementAt (1);
            pi.port = Integer.parseInt (portStr);
            String status = (String) v.elementAt (2);
            pi.status = status;
            System.out.println ("-->StopPlatform:pi.status: " + pi.status);
            if (status.equalsIgnoreCase ("running")) {
                if (stopJeeves (pi)) {
                    pi.status = "stopped";
                    v.removeElementAt (2);
                    v.insertElementAt (pi.status, 2);
                    _platfTableModel.removeRow (selPIs[i]);
                    _platfTableModel.insertRow (selPIs[i], v);
                    System.out.println ("-->StopPlatform:status inserted: " + v.elementAt(2));
                }
            }
            else {
                addLogMsg ("Skipping " + pi + " which is not stopped");
            }
        }
    }
    
    /**
     * Update a file in a remote machine.
     */
    protected void updateFileAction()
    {
        int[] selNum = _platfTable.getSelectedRows();
        //System.out.println ("-->>Num of selections: " + selNum.length);
        Object[] selPIs = new Object [selNum.length];
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _platfTableModel.getRowElements (selNum [j]);
            JeevesFinder.PlatformInfo pi = new JeevesFinder.PlatformInfo();
            pi.addr = (InetAddress) v.elementAt (0);
            String portStr = (String) v.elementAt (1);
            pi.port = Integer.parseInt (portStr);
            String status = (String) v.elementAt (2);
            pi.status = status;
            if (status.equals ("running")) {
                JOptionPane.showMessageDialog (this, "Please Stop the Service", "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            //System.out.println ("-->>Pi of selections: " + pi.toString());
            selPIs[j] = pi;
        }
        
        /*for (Enumeration e = _fileRepositoryVector.elements(); e.hasMoreElements();) {
            String str = (String) e.nextElement();
            System.out.println ("-->>localfilePath of _fileRepositoryVector: " + str);
        }*/
        
        UpdateFile uf = new UpdateFile();
        uf.init (_fileRepositoryVector);
        uf.setVisible (true);
        if (uf.update()) {
            String localFilePath = uf.getLocalFilePath().trim();
            //System.out.println ("-->localFilePath in update: " + localFilePath);
            if (_fileRepositoryVector.isEmpty()) {
                _fileRepositoryVector.addElement (localFilePath);
            }
            else {
                if (!_fileRepositoryVector.contains (localFilePath)) {
                    _fileRepositoryVector.addElement (localFilePath);
                }
            }
            
            String remoteRelativePath = uf.getRemoteRelativePath();
            File localFile = new File (localFilePath);
            long fileLength = localFile.length();
            if (fileLength == 0) {
                JOptionPane.showMessageDialog (this, localFilePath + " is not valid", "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            for (int i = 0; i < selPIs.length; i++) {
                FileInputStream fis = null;
                try {
                    fis = new FileInputStream (localFile);
                }
                catch (Exception e) {
                    JOptionPane.showMessageDialog (this, "Error opening file " + localFilePath, "Error", JOptionPane.ERROR_MESSAGE);
                    return;
                }
                JeevesFinder.PlatformInfo pi2 = (JeevesFinder.PlatformInfo) selPIs[i];
                
                //System.out.println ("-->Update File: " + pi2 + " at: " + remoteRelativePath);
                
                if (pi2.status.equalsIgnoreCase("stopped")) {
                    if (!updateFile (pi2, fis, fileLength, remoteRelativePath)) {
                        JOptionPane.showMessageDialog (this, "Error updating Jeeves at " + pi2.addr.getHostAddress(), "Error", JOptionPane.ERROR_MESSAGE);
                    }
                    else {
                        addLogMsg ("Successfully updated " + getNameFromPath (remoteRelativePath) + " at " + pi2.addr.getHostAddress());
                    }
                }
                else {
                    addLogMsg ("Skipping " + pi2 + " which is not stopped");
                }
                try {
                    fis.close();
                }
                catch (Exception e) {}
            }
        }
    }
    
    /**
     * Download a file from a remote machine.
     */
    protected void downloadFileAction()
    {
        int[] selNum = _platfTable.getSelectedRows();
        Object[] selPIs = new Object [selNum.length];
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _platfTableModel.getRowElements (selNum[j]);          
            JeevesFinder.PlatformInfo pi = new JeevesFinder.PlatformInfo();
            pi.addr = (InetAddress) v.elementAt (0);
            String portStr = (String) v.elementAt (1);
            pi.port = Integer.parseInt (portStr);
            String status = (String) v.elementAt (2);
            pi.status = status;
            if (status.equals ("running")) {
                JOptionPane.showMessageDialog (this, "Please Stop the Service", "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            selPIs[j] = pi;
        }
        
        if (selPIs.length != 1) {
            JOptionPane.showMessageDialog (this, "Must select one platform from which to download file", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        JeevesFinder.PlatformInfo pi2 = (JeevesFinder.PlatformInfo) selPIs[0];
        if (!pi2.status.equalsIgnoreCase("stopped")) {
            JOptionPane.showMessageDialog (this, "Platform must be stopped before downloading file", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        DownloadFile df = new DownloadFile();
        df.init (_downloadFileVector);
        df.setVisible (true);
        
        if (df.download()) {
            String localFilePath = df.getLocalFilePath();
            if (_downloadFileVector.isEmpty()) {
                _downloadFileVector.addElement (localFilePath.trim());
            }
            else {
                for (Enumeration e = _downloadFileVector.elements(); e.hasMoreElements();) {
                    String pathStr = (String) e.nextElement();
                    if (!pathStr.equals (localFilePath.trim())) {
                        _downloadFileVector.addElement (localFilePath.trim());
                    }
                    else {
                        //System.out.println ("-->The abs path is already in the box");
                        break;                        
                    }
                }
            }
            
            String remoteRelativePath = df.getRemoteRelativePath();
            File localFile = new File (localFilePath);
            if (localFile.exists()) {
                if (1 == JOptionPane.showConfirmDialog (this, "Overwrite File " + localFilePath +"?", "Confirm", JOptionPane.YES_NO_OPTION)) {
                    return;
                }
            }
            FileOutputStream fos = null;
            try {
                fos = new FileOutputStream (localFile);
            }
            catch (Exception e) {
                JOptionPane.showMessageDialog (this, "Error opening file " + localFilePath, "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            if (!downloadFile (pi2, fos, remoteRelativePath)) {
                JOptionPane.showMessageDialog (this, "Error download file " + remoteRelativePath + " from Jeeves at " + pi2.addr.getHostAddress(), "Error", JOptionPane.ERROR_MESSAGE);
            }
            else {
                addLogMsg ("Successfully downloaded " + getNameFromPath (remoteRelativePath) + " from remote site at " + pi2.addr.getHostAddress());
            }
            try {
                fos.close();
            }
            catch (Exception e) {}
        }
    }
    
    /**
     * Find local and remote platforms using the JeevesFinder.
     */
    protected void findPlatforms()
    {
        _findBtn.setEnabled (false);
        JeevesFinder sf = new JeevesFinder (this, _finderPort, 10000);
        setCursorToWait (true);
        sf.start();
    }
    
    /**
     * Synchronize directories in local and remote machines.
     */
    protected void updateDirectoryAction()
    {
        int[] selNum = _platfTable.getSelectedRows();
        Object[] selPIs = new Object [selNum.length];
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _platfTableModel.getRowElements (selNum [j]);
            JeevesFinder.PlatformInfo jpi = new JeevesFinder.PlatformInfo();
            jpi.addr = (InetAddress) v.elementAt (0);
            String portStr = (String) v.elementAt (1);
            jpi.port = Integer.parseInt (portStr);
            String status = (String) v.elementAt (2);
            jpi.status = status;
            if (status.equals ("running")) {
                JOptionPane.showMessageDialog (this, "Please Stop the Service", "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            selPIs[j] = jpi;
        }
        
        UpdateDirectory ud = new UpdateDirectory();
        ud.setVisible (true);
        if (ud.update()) {
            setCursorToWait (true);
            String localFilePath = ud.getLocalFilePath();
            //System.out.println ("-->localFilePath: " + localFilePath);
            //String remoteRelativePath = ud.getRemoteRelativePath();
            _objectVector.removeAllElements();
            _objectDirVector.removeAllElements();
            for (int i = 0; i < selPIs.length; i++) {
                //Get list of directories
                getDirStructure (new File (localFilePath));
                JeevesFinder.PlatformInfo pi = (JeevesFinder.PlatformInfo) selPIs[i];
                boolean updateIsStrict = ud.updateStrict();
                if (updateIsStrict) {
                    if (1 == JOptionPane.showConfirmDialog (this, "Do you want to delete all directories and files except " +
                                                                  localFilePath + "?", "Confirm", JOptionPane.YES_NO_OPTION)) {
                        return;
                    }
                }
                if (pi.status.equalsIgnoreCase ("stopped")) {
                    if (!updateDirectories (pi, updateIsStrict)) {
                        JOptionPane.showMessageDialog (this, "Error updating Jeeves at " + pi.addr.getHostAddress(), "Error", JOptionPane.ERROR_MESSAGE);
                    }
                    else {
                        addLogMsg ("Successfully updated " + pi.addr.getHostAddress());
                    }
                }
                else {
                    addLogMsg ("Skipping " + pi + " which is not stopped");
                }
            }
            setCursorToWait (false);
        }
    }
    
    /**
     * Remove one or more platforms from the platform table
     */
    protected void removePlatforms()
    {
        int[] selNum = _platfTable.getSelectedRows();
        System.out.println ("-->>selNum: " + selNum.length);
        for (int i = 0; i<selNum.length; i++) { 
            System.out.println ("-->>row Number: " + selNum [i]);
            Vector v = (Vector) _platfTableModel.getRowElements (selNum [i]);
           
            System.out.println ("-->>In vector To remove: " + (InetAddress) v.elementAt (0));
            
            _platfTableModel.removeRow (selNum [i]);
            //_platfTableModel.fireTableRowsDeleted (i, selNum.length);
            
            //Remove PlatformInfo object from the current List        
            JeevesFinder.PlatformInfo jpi = new JeevesFinder.PlatformInfo();
            jpi.addr = (InetAddress) v.elementAt (0);
            System.out.println ("-->>To remove: " + jpi.addr);
            String portStr = (String) v.elementAt (1);
            jpi.port = Integer.parseInt (portStr);
            String status = (String) v.elementAt (2);
            jpi.status = status;
            
            if (!_loadedPlatformList.isEmpty()) {                
                Enumeration enuu = _loadedPlatformList.elements();
                while (enuu.hasMoreElements()) {
                    JeevesFinder.PlatformInfo pi = (JeevesFinder.PlatformInfo) enuu.nextElement();
                    if (pi.equals (jpi)) {
                        _loadedPlatformList.removeElement (pi);
                        break;
                    }
                    else {
                        System.out.println ("JeevesFinder:Platfs are same!");
                    }
                }
            }
            if (!_newList.isEmpty()) {                
                Enumeration enu = _newList.elements();
                while (enu.hasMoreElements()) {
                    JeevesFinder.PlatformInfo pii = (JeevesFinder.PlatformInfo) enu.nextElement();
                    if (pii.equals (jpi)) {
                        _newList.removeElement (jpi);
                        break;
                    }
                    else {
                        System.out.println ("JeevesFinder:Platfs are same!");
                    }
                }
            }
        }
        updatePlatformList();
    }
    
    /**
     * Update the status of the platforms displayed on the Platform table
     */
    protected void updateStatusAction()
    {
        int rowNum = _platfTable.getRowCount();
        JeevesFinder.PlatformInfo pis[] = new JeevesFinder.PlatformInfo [rowNum];
        Hashtable ht = new Hashtable();
        for (int i = 0; i < rowNum; i++) {
            Vector v = (Vector) _platfTableModel.getRowElements (i);
            JeevesFinder.PlatformInfo jpi = new JeevesFinder.PlatformInfo();
            jpi.addr = (InetAddress) v.elementAt (0);
            String portStr = (String) v.elementAt (1);
            jpi.port = Integer.parseInt (portStr);
            String status = (String) v.elementAt (2);
            jpi.status = status;
            pis[i] = jpi;
            ht.put (jpi, String.valueOf (i));
        }
        if (_usePingForStatusUpdate) {
            updateStatusUsingPing (pis, ht);
        }
        else {
            Vector v = new Vector();
            for (int j = 0; j < pis.length; j++) {
                String rowStr = (String) ht.get (pis[j]);
                updateStatusUsingConnect (pis[j]);
                v.addElement (pis[j]);
            }
            clearTable();
            /*for (Enumeration e = v.elements(); e.hasMoreElements();) {
                JeevesFinder.PlatformInfo piNew = (JeevesFinder.PlatformInfo) e.nextElement();
                updateStatusInTable (piNew);
            }*/
            for (int k=0; k < v.size(); k++) {
                JeevesFinder.PlatformInfo piNew = (JeevesFinder.PlatformInfo) v.elementAt(k);
                updateRowStatusInTable (piNew, k); //**
            }
            _platfTable.revalidate();
        }
    }
    
    /**
     * Update the status of the selected platform displayed on the Platform table
     */
    protected void updateStatusSelectedAction()
    {
        int[] selNum = _platfTable.getSelectedRows();
        Object[] selPIs = new Object [selNum.length];
        Hashtable ht = new Hashtable();
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _platfTableModel.getRowElements (selNum [j]);
            JeevesFinder.PlatformInfo jpi = new JeevesFinder.PlatformInfo();
            jpi.addr = (InetAddress) v.elementAt (0);
            String portStr = (String) v.elementAt (1);
            jpi.port = Integer.parseInt (portStr);
            String status = (String) v.elementAt (2);
            jpi.status = status;
            selPIs[j] = jpi;
            //System.out.println ("-->>updateStatusSelectedAction selected rows: " + selPIs[j].toString() + " /" + selNum [j]);
            ht.put (jpi, String.valueOf (selNum [j]));
        }
        
        if (_usePingForStatusUpdate) {
            JeevesFinder.PlatformInfo pis[] = new JeevesFinder.PlatformInfo [selPIs.length];            
            for (int i = 0; i < selPIs.length; i++) {
                pis[i] = (JeevesFinder.PlatformInfo) selPIs[i];
            }
            updateStatusUsingPingSelected (pis, ht);
            
            /*if (piVector.isEmpty()) {
                piVector.addElement (pi);
                updateRowStatusInTable (pi, Integer.parseInt (rowStr));
            }
            else if (!piVector.contains (pi)) {
                updateRowStatusInTable (pi, Integer.parseInt (rowStr));
            }
            System.out.println ("-->>In updateStatusSelected Connect <Updated> selPIs: " + selPIs[i].toString());*/
        }
        else {
            Vector piVector = new Vector();
            for (int i = 0; i < selPIs.length; i++) {
                //System.out.println ("-->>In updateStatusSelected:selPIs.length: " + selPIs.length);
                JeevesFinder.PlatformInfo pi = (JeevesFinder.PlatformInfo) selPIs[i];
                //System.out.println ("-->>In updateStatusSelected:pi to remove: " + pi.toString());
                String rowStr = (String) ht.get (pi);
                //System.out.println ("-->>In updateStatusSelected:row to remove: " + Integer.parseInt (rowStr));
                _platfTableModel.removeRow (Integer.parseInt (rowStr));
                updateStatusUsingConnect (pi);
                if (piVector.isEmpty()) {
                    piVector.addElement (pi);
                    updateRowStatusInTable (pi, Integer.parseInt (rowStr));
                }
                else if (!piVector.contains (pi)) {
                    updateRowStatusInTable (pi, Integer.parseInt (rowStr));
                }
                //System.out.println ("-->>In updateStatusSelected Connect <Updated> selPIs: " + selPIs[i].toString());
            }
             _platfTable.revalidate();
        }
    }
    
    /**
     * Ping platforms to get the status
     */
    protected void pingPlatforms()
    {
        _usePingForStatusUpdate = true;
    }
    
    /**
     * Connect to platforms to get the status
     */
    protected void connectToPlatforms()
    {
        _usePingForStatusUpdate = false;
    }
    
    /**
     * Connect to platforms using Mockets
     */
    protected void connWithMockets()
    {
        _useMockets = true;
    }
    
    /**
     * Connect to platforms using Sockets
     */
    protected void connWithSockets()
    {
        _useMockets = false;
    }
    
    /**
     * Recursive function for getting the directory structure
     * 
     * @param   dir starting directory of the structure 
     */
    private void getDirStructure (File dir)
    {
        if (dir == null) {
            System.out.println ("-->The path is null<--");
        }

        File[] dirContents = dir.listFiles();
        if (dirContents == null) {
            System.out.println ("-->Directory is null");
            return;
        }
        MirrorDirObject mdo = null;
        for (int i = 0; i < dirContents.length; i++) {
            File currDir = dirContents[i];
            if (currDir.isDirectory()) {
                if (currDir.list().length == 0) {  //The directory is empty
                    mdo = new MirrorDirObject (getRelativePath (currDir.getAbsolutePath()), null);
                    _objectDirVector.addElement (mdo);
                }
                else {
                    //The directory is not empty so call recursive function
                    mdo = new MirrorDirObject (getRelativePath (currDir.getAbsolutePath()), null);
                    _objectDirVector.addElement (mdo);
                    getDirStructure (currDir);
                }
            }
            else if (currDir.isFile()) {  //If it is a file then get the MD5 string
                String absPath = currDir.getAbsolutePath().trim();
                String md5HashFile = getMD5String (new File (absPath));
                String relaPath = getRelativePath (absPath).trim();
                _relAbsTable.put (relaPath, absPath);
                mdo = new MirrorDirObject (absPath, md5HashFile);
                _objectVector.addElement (mdo);
            }
        }
    }
    
    /**
     * Show the messages in the Log list
     * 
     * @param    msg the message to show in the list
     */
    protected void addLogMsg (String msg)
    {
        String time = formatDate (System.currentTimeMillis());
        DefaultListModel dlm = (DefaultListModel) _jlLog.getModel();
        dlm.addElement ("[" + time + "] " + msg);
        int position = dlm.getSize();
        _jlLog.ensureIndexIsVisible (position-1); //Show last row of the list
    }
    
    /**
     * Return a string with the name of the file or directory to update
     * 
     * @param    pathName the path to file or directory
     */
    protected String getNameFromPath (String pathName)
    {
        int index = pathName.lastIndexOf (FILE_BREAK);
        String name = pathName.substring (index+1);
        //System.out.println ("-->name: " + name);
        if (name.endsWith (".java")) {
            name = "file " + name;
        }
        else {
            name = "directory " + name;
        }
        return name;
    }

    /**
     * Update the platform table
     */
    protected void updatePlatformList()
    {
        _platfTable.revalidate();
    }

    /**
     * Update the platform table with this vector
     * 
     * @param result  vector of data to add as a row
     */
    protected void updatePlatformList (Vector results)
    {            
        _newList = (Vector) this._loadedPlatformList.clone();        
        Enumeration enn1 = _newList.elements();
        while (enn1.hasMoreElements()) {
            JeevesFinder.PlatformInfo pi1 = (JeevesFinder.PlatformInfo) enn1.nextElement();
        }
        
        Enumeration e = results.elements();
        while (e.hasMoreElements()) {
            JeevesFinder.PlatformInfo pi = (JeevesFinder.PlatformInfo) e.nextElement();
            if (_newList.isEmpty()) {
                _newList.addElement (pi);
            }
            else {
                Enumeration enu = _newList.elements();
                while (enu.hasMoreElements()) {
                    JeevesFinder.PlatformInfo pi2 = (JeevesFinder.PlatformInfo) enu.nextElement();
                    if (!pi2.toString().equals (pi.toString())) {
                        _newList.addElement (pi);
                        break;
                    }
                    else {
                        for (int i=0; i<_newList.size(); i++) {
                            JeevesFinder.PlatformInfo pi1 = (JeevesFinder.PlatformInfo) _newList.elementAt(i);
                            if (pi1.equals (pi)) {
                                //System.out.println ("Here they are!: \n\t"+pi1+"\n\t"+pi);
                            }
                        }
                    }
                }
            }
        }
        updateTable (_newList);
        updatePlatformList();
    }

    /**
     * Indicate the end of the search 
     */
    protected void searchCompleted()
    {
        setCursorToWait (false);
        _findBtn.setEnabled (true);
    }

    /**
     * Indicate the end of ping a platform 
     */
    protected void pingCompleted()
    {
        setCursorToWait (false);
        _upStatusBtn.setEnabled (true);
       _upStSelBtn.setEnabled (true);
    }
    
    /**
     * Update the status of a platform connecting by sockets or mockets
     * 
     * @param    pis the platform status to update
     */
    private void updateStatusUsingConnect (JeevesFinder.PlatformInfo pi)
    {
        CommHelper ch = null;
        pi.status = "unknown";
        try {
            ch = connectToPlatform (pi);
            ch.sendLine ("STATUS");
            String reply = ch.receiveLine();
            //System.out.println ("-->String reply: " + reply);
            if (reply.equalsIgnoreCase ("running")) {
                pi.status = "running";
            }
            else if (reply.equalsIgnoreCase ("stopped")) {
                pi.status = "stopped";
            }
        }
        catch (Exception e) {
            addLogMsg ("Failed to get status - " + "\r\n" + e);
        }
        if (ch != null) {
            ch.closeConnection();
        }
    }

    /**
     * Update the status of a all platforms using ping
     * 
     * @param    pis the platform to update
     */
    private void updateStatusUsingPing (JeevesFinder.PlatformInfo[] pis, Hashtable ht)
    {
        _upStatusBtn.setEnabled (false);
        JeevesPinger sp = new JeevesPinger (this, _finderPort, 5000, pis, ht);
        setCursorToWait (true);
        sp.start();
    }

	/**
	 * Update the status of a selected platform using ping
	 * 
	 * @param    pis the platform to update
	 */
	private void updateStatusUsingPingSelected (JeevesFinder.PlatformInfo[] pis, Hashtable ht)
	{
		_upStSelBtn.setEnabled (false);
		JeevesPinger sp = new JeevesPinger (this, _finderPort, 5000, pis, ht);
		setCursorToWait (true);
		sp.start();
	}
    
    /**
     * Start Jeeves on the selected platform
     * 
     * @param    pi the platform to start
     */
    private boolean startJeeves (JeevesFinder.PlatformInfo pi)
    {
        CommHelper ch = null;
        boolean success = false;
        try {
            //System.out.println ("-->StartJeeves:pi: " + pi.toString());
            ch = connectToPlatform (pi);
            ch.sendLine ("START");
            ch.receiveMatch ("OK");
            success = true;
            //System.out.println ("-->StartJeevesAfter connect:pi: " + pi.toString());
        }
        catch (Exception e) {
            //e.printStackTrace();
            addLogMsg ("Failed to start Jeeves - " + "\r\n" + e);
        }
        if (ch != null) {
            ch.closeConnection();
        }
        return success;
    }

    /**
     * Stop Jeeves on the selected platform
     * 
     * @param    pi the platform to be stopped
     */
    private boolean stopJeeves (JeevesFinder.PlatformInfo pi)
    {
        CommHelper ch = null;
        boolean success = false;
        try {
            System.out.println ("-->StopJeeves:pi: " + pi.toString());
            ch = connectToPlatform (pi);
            ch.sendLine ("KILL");
            ch.receiveMatch ("OK");
            success = true;
        }
        catch (Exception e) {
            e.printStackTrace();
            addLogMsg ("Failed to stop Jeeves - " + "\r\n" + e);
            System.out.println ("Failed to stop Jeeves - " + e);
        }
        if (ch != null) {
            ch.closeConnection();
        }
        return success;
    }

    /**
     * Update files in a remote platform
     * 
     * @param   pi the platform to be updated
     * @param   fis the file input stream for transfering the file
     * @param	fileSize the size of the file to be transfered
     * @param	remoteRelativePath the relative path to the remote site
     */
    private boolean updateFile (JeevesFinder.PlatformInfo pi, FileInputStream fis, long fileSize, String remoteRelativePath)
    {
        CommHelper ch = null;
        boolean success = true;
        try {
            ch = connectToPlatform (pi);
            transferFile (ch, fis, fileSize, remoteRelativePath);
        }
        catch (Exception e) {
            success = false;
            addLogMsg ("Failed to update Jeeves - " + "\r\n" + e);
            e.printStackTrace();
        }
        if (ch != null) {
            ch.closeConnection();
        }
        return success;
    }

    /**
     * Transfer a file to a remote platform
     * 
     * @param   ch a handle to the Nomads CommHelper
     * @param   fis the file input stream for transfering the file
     * @param	fileSize the size of the file to be transfered
     * @param	remoteRelativePath the relative path to the remote site 
     */
    private boolean transferFile (CommHelper ch, FileInputStream fis, long fileSize, String remoteRelativePath)
    {
        boolean success = true;
        try {
            ch.sendLine ("UPDATE " + remoteRelativePath + " " + fileSize);
            ch.receiveMatch ("OK");
            byte[] buf = new byte[1024];
            long totalBytesRead = 0;
            while (totalBytesRead < fileSize) {
                int bytesRead = fis.read (buf);
                if (bytesRead < 0) {
                    success = false;
                    break;
                }
                totalBytesRead += bytesRead;
                ch.sendBlob (buf, 0, bytesRead);
            }
            ch.receiveMatch ("OK");
        }
        catch (Exception e) {
            success = false;
            addLogMsg ("Failed to update Jeeves - " + "\r\n" + e);
            e.printStackTrace();
        }
        return success;
    }

    /**
     * Download a file from a remote platform
     * 
     * @param   pi the platform to be updated
     * @param   fos the file output stream for downloading the file
     * @param	remoteRelativePath the relative path to the remote site 
     */
    private boolean downloadFile (JeevesFinder.PlatformInfo pi, FileOutputStream fos, String remoteRelativePath)
    {
        CommHelper ch = null;
        boolean success = true;
        try {
            ch = connectToPlatform (pi);
            ch.sendLine ("DOWNLOAD " + remoteRelativePath);
            ch.receiveMatch ("OK");
            long fileSize = Long.parseLong (ch.receiveLine());
            byte[] buf = new byte[1024];
            long totalBytesRead = 0;
            while (totalBytesRead < fileSize) {
                int bytesToRead = (int) (fileSize - totalBytesRead);
                if (bytesToRead > buf.length) {
                    bytesToRead = buf.length;
                }
                ch.receiveBlob (buf, 0, bytesToRead);
                fos.write (buf, 0, bytesToRead);
                totalBytesRead += bytesToRead;
            }
            ch.receiveMatch ("OK");
        }
        catch (Exception e) {
            success = false;
            e.printStackTrace();
            addLogMsg ("Failed to download Jeeves - " + "\r\n" + e);
        }
        if (ch != null) {
            ch.closeConnection();
        }
        return success;
    }

    /**
     * This method allows to mirror a directory on the remote site and update it
     * 
     * @param   pi the platform to be updated
     * @param	isStrict indicates if a mirror copy is desired
     */
    private boolean updateDirectories (JeevesFinder.PlatformInfo pi, boolean isStrict)
    {
        CommHelper ch = null;
        boolean success = true;
        try {
            ch = connectToPlatform (pi);
            ch.sendLine ("SYNCH_DIRS");

            ch.receiveMatch ("OK");
            Vector resultsVector = new Vector();
            Vector resultsDirVector = new Vector();
            Vector aIntbVector = new Vector();
            Vector aMinbVector = new Vector();
            Vector bMinaVector = new Vector();
            Vector localVector = new Vector();
            Vector remoteVector = new Vector();
            Hashtable ht = new Hashtable();
            while (true) {
                String[] fileNames = ch.receiveParsed();
                //System.out.println ("-->Line received 0: " + fileNames[0]);
                //System.out.println ("-->Line received 1: " + fileNames[1]);

                if (fileNames[0].equalsIgnoreCase ("END")) {
                    break;
                }
                else if (fileNames.length == 0) {
                    System.err.println ("No frame has been received");
                    return false;
                }
                else {                   
                    //Add file path and MD5 to the MirrorDir object
                    if (fileNames[0].equals ("DIR")) {
                        MirrorDirObject remMdo = new MirrorDirObject (fileNames[1], null);
                        resultsDirVector.addElement (remMdo);
                    }
                    else {
                        //Store the MD5 string and file name into a hashtable
                        ht.put (fileNames[1], fileNames[0]);
                        resultsVector.addElement (fileNames[1]);
                    }
                }
            }
            checkDirectories (resultsDirVector, ch, isStrict);

            for (Enumeration enx = _objectVector.elements(); enx.hasMoreElements();) {
                MirrorDirObject mdo1 = (MirrorDirObject) enx.nextElement();
                localVector.addElement (getRelativePath (mdo1._filePath));
            }

            boolean areDiff = Diff.vectorDiff (localVector, resultsVector, aIntbVector, aMinbVector,
                                               bMinaVector);
            if (areDiff) {
                FileInputStream fis = null;
                String filePathToSend = null;
                long fileLength = 0;
                if (!aMinbVector.isEmpty()) {
                    //Send the new files to remote system
                    for (Enumeration en2 = aMinbVector.elements(); en2.hasMoreElements();) {
                        String fileStr = (String) en2.nextElement();
                        //System.out.println ("-->MirrorDirObject aMinbVector: " + fileStr);
                        if (fileStr == null) {
                            System.out.println ("-->No files to send");
                        }
                        
                        String filePathToSend2 = (String) _relAbsTable.get (fileStr.trim());
                        try {
                            File localFile = new File (filePathToSend2);
                            fileLength = localFile.length();
                            fis = new FileInputStream (localFile);
                        }
                        catch (Exception ex) {
                            JOptionPane.showMessageDialog (this, "Error opening file " + filePathToSend, "Error", JOptionPane.ERROR_MESSAGE);
                            return false;
                        }
                        if (pi.status.equalsIgnoreCase("stopped")) {
                            String relPath = getRelativePath (filePathToSend2);
                            if (fileLength <= 0) {
                                System.out.println ("-->The file is empty!");
                                //return false;
                            }
                            if (!transferFile (ch, fis, fileLength, relPath)) {
                                JOptionPane.showMessageDialog (this, "Error updating Jeeves at " + pi.addr.getHostAddress(), "Error", JOptionPane.ERROR_MESSAGE);
                            }
                            else {
                                addLogMsg ("Successfully updated " + getNameFromPath (relPath) + " at " + pi.addr.getHostAddress());
                            }
                        }
                        else {
                            addLogMsg ("Skipping " + pi + " which is not stopped");
                        }
                    }
                    try {
                        fis.close();
                    }
                    catch (Exception e) {}
                }
                if ((!bMinaVector.isEmpty()) && (isStrict)) {
                    //Delete the files in remote system if the strict button is selected.
                    for (Enumeration en3 = bMinaVector.elements(); en3.hasMoreElements();) {
                        String relPath2 = (String) en3.nextElement();
                        ch.sendLine ("DELETE_FILE " + relPath2);
                        //ch.receiveMatch ("OK");
                    }
                }
                else {
                    bMinaVector.removeAllElements();
                }
				//Delete directories after delete the files.
				if ((!_bMinDiraVector.isEmpty()) && (isStrict)) {
					//Delete the files in remote system if the strict button is selected.
					for (Enumeration en3 = _bMinDiraVector.elements(); en3.hasMoreElements();) {
						String strDir2 = (String) en3.nextElement();
						try {
							ch.sendLine ("DELETE_FILE " + strDir2);
						}
						catch (Exception ex) {
							System.out.println ("-->DELETE_FILE Exception: " + ex);
						}
					}
				}
				else {
					_bMinDiraVector.removeAllElements();
				}
                if (!aIntbVector.isEmpty()) {
                    //Compare the MD5 and update remote files with diferent MD5 number.
                    FileInputStream fisRem = null;
                    for (Enumeration enu = _objectVector.elements(); enu.hasMoreElements();) {
                        MirrorDirObject mdo3 = (MirrorDirObject) enu.nextElement();
                        if (mdo3._filePath.endsWith ("jeeves.exe")) {
                            break;
                        }
                        for (Enumeration enun = aIntbVector.elements(); enun.hasMoreElements();) {
                            String strPath = (String) enun.nextElement();
                            String remValue = (String) ht.get (strPath);
                            if (strPath.equals (getRelativePath (mdo3._filePath))) {
                                String value = mdo3._md5value;
                                if (!value.equals (remValue)) {
                                    String pathStrToSend = mdo3._filePath;
                                    long lengthFileToSend = 0;
                                    try {
                                        File localFileToSend = new File (pathStrToSend);
                                        lengthFileToSend = localFileToSend.length();
                                        fisRem = new FileInputStream (localFileToSend);
                                    }
                                    catch (Exception ex) {
                                        JOptionPane.showMessageDialog (this, "Error opening file " + filePathToSend, "Error", JOptionPane.ERROR_MESSAGE);
                                        return false;
                                    }
                                    if (pi.status.equalsIgnoreCase("stopped")) {
                                        String relPathToSend = getRelativePath (pathStrToSend);
                                        if (!transferFile (ch, fisRem, lengthFileToSend, relPathToSend)) {
                                            JOptionPane.showMessageDialog (this, "Error updating Jeeves at " + pi.addr.getHostAddress(), "Error", JOptionPane.ERROR_MESSAGE);
                                        }
                                        else {
                                            addLogMsg ("Successfully updated " + getNameFromPath (relPathToSend) + " at " + pi.addr.getHostAddress());
                                        }
                                    }
                                    else {
                                        addLogMsg ("Skipping " + pi + " which is not stopped");
                                    }
                                }
                                else {
                                    break;
                                }
                                try {
                                    fisRem.close();
                                }
                                catch (Exception e) {}
                            }
                        }
                    }
                }
            }
            ch.sendLine ("CLOSE_CONNECTION");
        }
        catch (Exception e) {
            success = false;
            e.printStackTrace();
            addLogMsg ("Failed to synchronize remote directory - " + e);
        }
        if (ch != null) {
            ch.closeConnection();
        }
        return success;
    }
    
    /**
     * Get an absolute path and returns a string representation of the relative
     * path
     * 
     * @param	absPath the absolute path
     */
    protected String getRelativePath (String absPath)
    {
        String str = "nomads";
        int index = absPath.indexOf ("nomads");
        //System.out.println ("-->getRelativePath:index: " + index);
        String relPath = absPath.substring (index + str.length()+1);
		//System.out.println ("-->getRelativePath:relPath: " + relPath);
        return relPath;
    }

    /**
     * Check if the directories exists in the remote site. Otherwise it creates them
     * 
     * @param   ch a handle to the Nomads CommHelper
     * @param   remDirVector vector of remote directories
     * @param	isStrict indicates if a mirror copy is desired 
     */
    protected void checkDirectories (Vector remDirVector, CommHelper ch, boolean isStrict)
    {
        Vector aIntbVector = new Vector();
        Vector aMinbVector = new Vector();
        Vector _bMinDiraVector = new Vector();
        Vector localVector = new Vector();
        Vector remoteVector = new Vector();

        for (Enumeration enx = _objectDirVector.elements(); enx.hasMoreElements();) {
            MirrorDirObject mdo1 = (MirrorDirObject) enx.nextElement();
            //Check local dirs for forward slashes and change them to back slashes if there are any.
            String localFilePath = mdo1._filePath;
            if (localFilePath.indexOf ("/") > 0) {
                localFilePath.replaceAll ("/", "\\");
            }
            localVector.addElement (localFilePath);
        }
        for (Enumeration enxx = remDirVector.elements(); enxx.hasMoreElements();) {
            MirrorDirObject mdo2 = (MirrorDirObject) enxx.nextElement();
            String str = mdo2._filePath;
            if (str.indexOf ("/") > 0) {
                str.replaceAll ("/", "\\");
            }
            remoteVector.addElement (str);
        }

        boolean areDiff = Diff.vectorDiff (localVector, remoteVector, aIntbVector, aMinbVector,
                                           _bMinDiraVector);

        if (areDiff) {
            FileInputStream fis = null;
            String dirPathToSend = null;
            long fileLength = 0;
            if (!aMinbVector.isEmpty()) {
                //Send the new files to remote system
                for (Enumeration en2 = aMinbVector.elements(); en2.hasMoreElements();) {
                    String str2 = (String) en2.nextElement();
                    //System.out.println ("-->MirrorDirObject aMinbVector: " + str2);
                    if (str2 == null) {
                        System.out.println ("-->Object dir is null");
                        return;
                    }
                    try {
                        ch.sendLine ("CREATE_DIR " + str2);
                        //ch.receiveMatch ("OK");
                    }
                    catch (Exception e) {
                        System.out.println ("-->CREATE_DIR Exception: " + e);
                    }
                }
            }
        }
    }
    
    /**
     * Get a row from the table and build a string.
     * Returns a vector of strings
     * 
     * @param	rowsVector vector of rows from the platform table
     */
    protected Vector buildStringFromRow (Vector rowsVector)
    {
        if (rowsVector.isEmpty()) {
            System.out.println ("-->There is no Row in this vector");
            return null;
        }
        else {
            Vector dataVector = new Vector();
            for (Enumeration e = rowsVector.elements(); e.hasMoreElements();) {
                Vector rowList = (Vector) e.nextElement();
                StringBuffer sb = new StringBuffer();
                String addr = (rowList.elementAt (0)).toString();
                sb.append (addr + " ");
                String port = String.valueOf (rowList.elementAt (1));
                sb.append (port + " ");                
                dataVector.addElement (sb.toString().trim());
            }
            return dataVector;
        }
    }
    
    /**
     * Update the table at row with the new PlatformInfo.
     * 
     * @param	pi platform to be updated
     * @param	row the row number
     */
    public void updateRowStatusInTable (JeevesFinder.PlatformInfo pi, int row)
    {
        if (pi == null) {
            System.out.println ("-->JeevesFinder.PlatformInfo is null");
            return;
        }
        else {
            Vector dataVector = new Vector();
            dataVector.addElement (pi.addr);
            dataVector.addElement (String.valueOf (pi.port));
            dataVector.addElement (pi.status);
            _platfTableModel.insertRow (row, dataVector);
            //System.out.println ("-->>Row inserted");
        }
    }
    
    /**
     * Update the table at row with the new PlatformInfo when using ping.
     * 
     * @param	pi platform to be updated
     * @param	row the row number to update
     */
    public void updateStatusInTable (JeevesFinder.PlatformInfo pi, int row)
    {
        if (pi == null) {
            System.out.println ("-->JeevesFinder.PlatformInfo is null");
            return;
        }
        else {
            _platfTableModel.removeRow (row);
            //System.out.println ("-->updateStatusInTable removing row: " + row);
            Vector dataVector = new Vector();
            dataVector.addElement (pi.addr);
            dataVector.addElement (String.valueOf (pi.port));
            dataVector.addElement (pi.status);
            if (_platfTableModel.getRowCount() < row) {
                _platfTableModel.addRow (dataVector);
            }
            else {
                _platfTableModel.insertRow (row, dataVector);
            }
        }
    }
    

    /**
     * Get the MD5 string from a file.
     * 
     * @param	localfile the file which the MD5 is calculated
     */
    protected String getMD5String (File localFile)
    {
        String md5String = null;
        try {
            MessageDigest md5 = MessageDigest.getInstance("MD5");
            FileInputStream fis = new FileInputStream (localFile);
            byte [] buff = new byte [1024];
            int nread;
            while ((nread = fis.read (buff, 0, 1024)) >= 0 ) {
                md5.update (buff, 0, nread);
            }
            fis.close();

            Base64Transcoders b64tc = new Base64Transcoders();
            md5String = b64tc.convertByteArrayToB64String (md5.digest());
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return md5String;
    }

    /**
     * Covert a timestamp (TimeMillis) in a formatted string with the following schema:
     * MM/dd/yy HH:mm:ss
     *
     * @param timeStamp
     * @return formattedString
     */
    protected String formatDate (long timeStamp)
    {
        SimpleDateFormat sdf = new SimpleDateFormat ("MM/dd/yy HH:mm:ss");
        Calendar calendar = Calendar.getInstance();
        Date date = new Date();
        date.setTime (timeStamp);
        calendar.setTime (date);
        return sdf.format (calendar.getTime());
    }

    /**
     * Connect to a platform using mockets or sockets
     * 
     * @param	pi platform to connect
     */
    private CommHelper connectToPlatform (JeevesFinder.PlatformInfo pi)
        throws Exception
    {
        if (_useMockets) {
            return connectToPlatformUsingMockets (pi);
        }
        else {
            return connectToPlatformUsingSockets (pi);
        }
    }

    /**
     * Connect to a platform using sockets
     * 
     * @param	pi platform to connect using sockets
     */
    private CommHelper connectToPlatformUsingSockets (JeevesFinder.PlatformInfo pi)
        throws Exception
    {
        CommHelper ch = new CommHelper();
        //ch.init (new Socket (pi.addr, _controlPort));
        ch.init (new Socket (pi.addr, pi.port));
        return ch;
    }

    /**
     * Connect to a platform using mockets
     * 
     * @param	pi platform to connect using mockets
     */
    private CommHelper connectToPlatformUsingMockets (JeevesFinder.PlatformInfo pi)
        throws Exception
    {
        MocketCommHelper mch = new MocketCommHelper();
        Mocket m = new Mocket();
        //m.connect (pi.addr, _controlPort);
        m.connect (pi.addr, pi.port);
        m.addMocketStatusListener (new MocketStatusHandler (this));
        mch.init (m);
        return mch;
    }
    
    /**
     * Either set the cursor to the "wait for it" cursor,
     * or set it back to the normal cursor.
     * 
     * @param	wait indicates if the cursor wait
     **/
    protected void setCursorToWait (boolean wait) 
    {
        if (wait) {
            this.getContentPane().setCursor (Cursor.getPredefinedCursor (Cursor.WAIT_CURSOR));
        }
        else {
            this.getContentPane().setCursor (Cursor.getPredefinedCursor (Cursor.DEFAULT_CURSOR));
        }
    }
        
    // Sets the frame location at the center of the screen
	public void setFrameLocation()	
	{
	    Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension d = tk.getScreenSize();
        Dimension dd = this.getPreferredSize();
        int width = ((d.width/3) - (dd.width/3)) + 80;
        int height = ((d.height/3) - (dd.height/3)) + 80;
        this.setLocation (width, height);
	}
    
	/**
	 * Get the files available in the remote machine in order to edit one of 
	 * them.
	 * 
	 * @param	pi platform where the file will be updated
	 */
    protected boolean getFileInfoForPopupMenu (JeevesFinder.PlatformInfo pi)
    {
        Vector v = new Vector();
        CommHelper ch = null;
        boolean success = true;
        try {
            ch = connectToPlatform (pi);
            ch.sendLine ("EDIT_LIST");
            ch.receiveMatch ("OK");
            
            while (true) {
                String reply = ch.receiveLine();
                System.out.println ("-->Line received: " + reply);

                if (reply.equalsIgnoreCase ("END_EDIT_LIST")) {
                    break;
                }
                else if (reply == null) {
                    System.err.println ("No frame has been received");
                    return false;
                }
                else {
                    //if (!v.contains (reply)) {
                      //  v.addElement (reply);
                        _platfMi = new JMenuItem (reply);
                        _platfMi.addActionListener (new PlatfMenuItemActionListener());
                        _platformPopupMenu.add (_platfMi);
                        _platformPopupMenu.addSeparator();
                   // }
                    
                }
            }
            //_platfMi.addActionListener (new PlatfMenuItemActionListener());
            //System.out.println ("-->>pi: " + pi);
            //editRemoteFiles (pi);            
        }
        catch (Exception e) {
            success = false;
            e.printStackTrace();
            addLogMsg ("Failed to synchronize remote directory - " + e);
        }
        if (ch != null) {
            ch.closeConnection();
        }
        return success;
    }
    
	/**
	 * Download the selected file in the remote site so it can be edited
	 * 
	 * @param	pi platform where a file will be updated
	 */
    protected void editRemoteFiles (JeevesFinder.PlatformInfo pi)
    {
        JFrame f = new JFrame();
        //System.out.println ("-->In editRemoteFiles:_menuLabel: " + _menuLabel);
        if (_menuLabel == null) {
            JOptionPane.showMessageDialog (f, "Please select an item","Invalid input",
                                           JOptionPane.ERROR_MESSAGE);
            return;
        }
        else {
            String remoteRelativePath = _menuLabel;
            //System.out.println ("-->In editRemoteFiles:remoteRelativePath: " + remoteRelativePath);
            String tmpdir = System.getProperty ("user.dir") + System.getProperty ("file.separator") + "tmpJeeves";
            File tmpFile = new File (tmpdir);
            boolean success = false;
            if (tmpFile.exists()) {
                success = true;
            }
            else {
                success = tmpFile.mkdir();
            }
            if (success) {
                String localFileName = parseFilePath (remoteRelativePath);
                //System.out.println ("-->In editRemoteFiles:localFileName: " + localFileName);
                File localFile = new File (tmpdir + System.getProperty ("file.separator") + localFileName);
                if (localFile.exists()) {
                    System.out.println ("-->The file already exists");
                    /*if (1 == JOptionPane.showConfirmDialog (this, "Overwrite File " + localFileName +"?", "Confirm", JOptionPane.YES_NO_OPTION)) {
                        return;
                    }*/
                }
                FileOutputStream fos = null;
                try {
                    fos = new FileOutputStream (localFile);
                }
                catch (Exception e) {
                    e.printStackTrace();
                    JOptionPane.showMessageDialog (this, "Error opening file " + localFileName, "Error", JOptionPane.ERROR_MESSAGE);
                    return;
                }
                //System.out.println ("-->In editRemoteFiles:remoteRelativePath: " + remoteRelativePath);
                if (!downloadFile (pi, fos, remoteRelativePath)) {
                    JOptionPane.showMessageDialog (this, "Error download file " + remoteRelativePath + " from Jeeves at " + pi.addr.getHostAddress(), "Error", JOptionPane.ERROR_MESSAGE);
                    
                }
                else {
                    addLogMsg ("Successfully downloaded " + getNameFromPath (remoteRelativePath) + " from remote site at " + pi.addr.getHostAddress());
                    try {
                        fos.close();
                    }
                    catch (Exception e) {}
                    
                    JeevesFileEditor editor = new JeevesFileEditor (localFile);
                    editor.setVisible (true);
                    //System.out.println ("-->isSuccess: " + editor.isSuccessful());
                    if (editor.isSuccessful()) {
                        addLogMsg ("Successfully downloaded file to tmpJeeves dir");
                        //System.out.println ("-->In editRemoteFiles before upload:remoteRelativePath: " + remoteRelativePath);
                        uploadEditedFile(localFile, remoteRelativePath, pi);
                    }
                }
            }
        }
    }
     
    /**
     * Upload the edited file
     * 
     * @param   pi the platform to be updated
     * @param   localfile the local file to edit
     * @param	remoteRelativePath the relative path to the remote site 
     */
    protected void uploadEditedFile (File localFile, String remoteRelativePath, JeevesFinder.PlatformInfo pi)
    {
        //System.out.println ("-->uploadEditedFile:localFile: " + localFile);
        long fileLength = localFile.length();
        if (fileLength == 0) {
            return;
        }
        FileInputStream fis = null;
        try {
            fis = new FileInputStream (localFile);
        }
        catch (Exception e) {
            JOptionPane.showMessageDialog (this, "Error opening file " + localFile, "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        
        //System.out.println ("-->Update File: " + pi + " at: " + remoteRelativePath);
        if (pi.status.equalsIgnoreCase ("stopped")) {
            if (!updateFile (pi, fis, fileLength, remoteRelativePath)) {
                JOptionPane.showMessageDialog (this, "Error updating Jeeves at " + pi.addr.getHostAddress(), "Error", JOptionPane.ERROR_MESSAGE);
            }
            else {
                addLogMsg ("Successfully updated " + getNameFromPath (remoteRelativePath) + " at " + pi.addr.getHostAddress());
            }
        }
        else {
            addLogMsg ("Skipping " + pi + " which is not stopped");
        }
        try {
            fis.close();
        }
        catch (Exception e) {}
    }
    
	/**
	 * Parse the file path and returns the filename
	 * 
	 * @param	filePath the absolute path
	 */
    protected String parseFilePath (String filePath)
    {
        int index = filePath.lastIndexOf (System.getProperty ("file.separator"));
        String fileName = filePath.substring (index+1);
        //System.out.println ("-->>parseFilePath:FileName: " + fileName);
        return (fileName);
    }
    
    public PlatformTableModel getTableModel()
    {
        return _platfTableModel;
    }

    /**
     * The MocketStatusListener handler
     */
    public class MocketStatusHandler implements MocketStatusListener
    {
        MocketStatusHandler (JeevesAdminTool at)
        {
            _at = at;
        }

        public boolean peerUnreachableWarning (long timeSinceLastContact)
        {
            if (timeSinceLastContact > 10000) {
                addLogMsg ("Mocket connection timing out");
                return true;
            }
            return false;
        }

        private JeevesAdminTool _at;
    }

    /**
     * The Mirror object that contains the file path and the MD5 string
     */
    public class MirrorDirObject implements Serializable
    {
        public MirrorDirObject (String filePath, String md5value)
        {
            _filePath = filePath;
            _md5value = md5value;
        }

        public String _filePath;
        public String _md5value;
    }
    
    /**
     * Table Model for the Platforms
     */
    public class PlatformTableModel extends DefaultTableModel
    {
        public PlatformTableModel (Vector data, Vector columnNames)
        {
            super (data, columnNames);
        }
        
        // Get the data from each row of the table.
        public Vector getRowElements (int row)
        {        
            Vector rowVector = new Vector();
            InetAddress platform = (InetAddress) getValueAt(row, 0);
            rowVector.addElement (platform);
            String port = (String) getValueAt (row, 1);
            rowVector.addElement (port);
            String status = (String) getValueAt(row, 2);
            rowVector.addElement (status);
            return rowVector;
        }
    }
    
    /**
     * Table selection listener
     */
    private class PlatfTableSelectionHandler implements ListSelectionListener
    {
        public void valueChanged (ListSelectionEvent evt) 
        {
            if (evt.getValueIsAdjusting()) {
                return;
            }
            ListSelectionModel lsm = (ListSelectionModel) evt.getSource();
            if (lsm.isSelectionEmpty()) {
                System.out.println ("No element was selected.");
            } 
            else {
                int row = _platfTable.getSelectedRow();
            }
        }
    }
    
    private class PlatfMenuItemActionListener implements ActionListener
    {
        public void actionPerformed (ActionEvent ae)
        {
            JMenuItem mi = (JMenuItem) ae.getSource();
            //_menuLabel = mi.getLabel();
            _menuLabel = mi.getText();
            //System.out.println ("-->In PlatfMenuItemActionListener:_menuLabel: " + _menuLabel);
			if (_menuLabel == null) {
				System.out.println ("No platform was selected");
			}
            //System.out.println ("-->In PlatfMenuItemActionListener:_selectedPlatform: " + _selectedPlatform.toString());
            editRemoteFiles (_selectedPlatform); 
        }
    }
        
    /**
     * FileFilter for Jeeves purposes.
     */
    public class JeevesFileFilter extends FileFilter
    {
        public JeevesFileFilter() 
        {
            _filters = new Hashtable();
        }
        
        public void addExtension (String extension) 
        {
            if (_filters == null) {
                _filters = new Hashtable(5);
            }
            _filters.put (extension.toLowerCase(), this);
            _fullDescription = null;
        }
        
        public String getExtension (File f) 
        {
            if (f != null) {
                String filename = f.getName();
                int i = filename.lastIndexOf('.');
                if (i>0 && i<filename.length()-1) {
                    return filename.substring(i+1).toLowerCase();
                };
            }
            return null;
        }
        
        public String getDescription() 
        {           
            return _fullDescription;
        }
        
        public void setDescription (String description) 
        {
            _description = description;
            _fullDescription = null;
        }
        
        public boolean accept (File f) 
        {           
            if (f != null) {
                if (f.isDirectory()) {
                    return true;
                }
                String extension = getExtension(f);
                if (extension != null && _filters.get(getExtension(f)) != null) {
                    return true;
                };
            }
            return false;
        }
        
        private Hashtable _filters = null;
        private String _description = null;
        private String _fullDescription = null;
    }
    
    protected class MyMouseListener extends MouseAdapter 
    {
        public void mousePressed (MouseEvent event)
        {
            checkPopup (event);
        }

        public void mouseReleased (MouseEvent event)
        {
            checkPopup (event);
        }

        public void checkPopup (MouseEvent event)
        {
            if (event.isPopupTrigger()) {
                JTable t = (JTable) event.getSource();
                int index = t.rowAtPoint (event.getPoint());
                //int index = t.getSelectedRow();
                //System.out.println ("-->>index: " + index);
                Vector v = (Vector) _platfTableModel.getRowElements (index);
                //System.out.println ("-->>vector: " + v);
                JeevesFinder.PlatformInfo pi = new JeevesFinder.PlatformInfo();
                pi.addr = (InetAddress) v.elementAt (0);
                String portStr = (String) v.elementAt (1);
                pi.port = Integer.parseInt (portStr);
                String status = (String) v.elementAt (2);
                pi.status = status;
                //System.out.println ("-->>Creating pi: " + pi);
                _platformPopupMenu = new JPopupMenu();
                //_platformPopupMenu.add (new JMenuItem("File"));
                if (status.equals ("running")) {
                    JOptionPane.showMessageDialog (new JFrame(), "Please Stop the Service", "Error", JOptionPane.ERROR_MESSAGE);
                    return;
                }
                getFileInfoForPopupMenu (pi);
                _platformPopupMenu.show (t, event.getPoint().x, event.getPoint().y);
                _selectedPlatform = pi;
            }
        }
    }

    
    public static void main (String args[])
    {
        JeevesAdminTool adminTool = new JeevesAdminTool();
        adminTool.setVisible (true);
    }
    
    //Class variables
    private boolean _useMockets = true;
    private boolean _usePingForStatusUpdate = false;
    private int _finderPort = 3278;
    private int _controlPort = 3279;
    private String _serviceName = null;
    
    protected Hashtable _relAbsTable;
    protected JButton _findBtn;
    protected JButton _upStatusBtn;
    protected JButton _upStSelBtn;
    protected JList _jlLog;
    protected JMenuItem _platfMi;
    protected JPopupMenu _platformPopupMenu;
    protected JTable _platfTable;
    protected PlatformTableModel _platfTableModel;
    protected String FILE_BREAK = System.getProperty ("file.separator");
    protected String _menuLabel;
    protected JeevesFinder.PlatformInfo _selectedPlatform;
    protected Vector _downloadFileVector;
    protected Vector _fileRepositoryVector;
    protected Vector _newList;
    protected Vector _loadedPlatformList = new Vector();
    protected Vector _objectVector;
    protected Vector _objectDirVector;
    protected Vector _rowDataVector;
    protected Vector _updateDirsVector;
    protected Vector _bMinDiraVector = new Vector();
}