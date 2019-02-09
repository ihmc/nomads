package us.ihmc.rms;

import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.*;
import java.awt.Insets;
import java.awt.Toolkit;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.EtchedBorder;
import javax.swing.DefaultListModel;
import javax.swing.JList;
import javax.swing.JTable;
import javax.swing.table.*;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.Serializable;
import java.net.InetAddress;
import java.net.Socket;
import java.security.*;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Vector;

import us.ihmc.aci.grpMgr.GroupManager;
import us.ihmc.aci.grpMgr.GroupManagerException;
import us.ihmc.aci.grpMgr.GroupManagerListener;
import us.ihmc.aci.grpMgr.NetworkInterfaceConf;
import us.ihmc.aci.grpMgr.PeerInfo;
import us.ihmc.comm.CommHelper;
import us.ihmc.ds.Diff;
import us.ihmc.util.Base64Transcoders;
import us.ihmc.util.ConfigLoader;
import us.ihmc.util.NSLookup;

/**
 * The Remote Management Service Console
 * 
 * @author  Maggie Breedy <Nomads team>
 * @version $Revision$
 *
 **/

public class RMSConsole extends JFrame implements GroupManagerListener
{
    public RMSConsole()
    {
       try {
            // Initialize the ConfigLoader
            String rmsHome = (String) System.getProperties().get ("rms.home");
            String cfgFilePath = File.separator + "conf" + File.separator + "rms.cfg";
            _cfgLoader = ConfigLoader.initDefaultConfigLoader (rmsHome, cfgFilePath);

            if (_cfgLoader.hasProperty("RMSPortNum")) {
                _rmsPort = _cfgLoader.getPropertyAsInt ("RMSPortNum");
            }
            else {
                _rmsPort = DEFAULT_RMS_PORT_NUM;
            }

            if (_cfgLoader.hasProperty("RMSGrpMgrPort")) {
                _grpMgrPort = _cfgLoader.getPropertyAsInt ("RMSGrpMgrPort");
            }
            else {
                _grpMgrPort = DEFAULT_GROUP_MANAGER_PORT_NUM;
            }

            // Initialize the Group Manager
            if (_cfgLoader.hasProperty("RMSGrpMgrNetIF")) {
                Vector netIFConfs = new Vector(); //Vector<NetworkInterfaceConf>
                StringTokenizer st = new StringTokenizer (_cfgLoader.getProperty("RMSGrpMgrNetIF")," :;");
                while (st.hasMoreTokens()) {
                    StringTokenizer st2 = new StringTokenizer (st.nextToken(), "/");
                    InetAddress ipAddr = InetAddress.getByName (st2.nextToken());
                    String advMode = st2.nextToken();
                    netIFConfs.addElement (new NetworkInterfaceConf(ipAddr, advMode));
                }
                _grpMgr = new GroupManager (_grpMgrPort, netIFConfs);
            }
            else {
                _grpMgr = new GroupManager (_grpMgrPort);
            }
            String nodeName = "RMS-" + InetAddress.getLocalHost().getHostName();
            _grpMgr.setNodeName (nodeName);
            _grpMgr.setNodeUUID (nodeName);
            _grpMgr.setListener (this);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    public void init()
    {
        try {
            System.out.println ("RMSConsole:init:creating the group");
            _grpMgr.start();
            _fileRepositoryVector = new Vector();
            _rowDataVector = new Vector();
            createGui();
            setFrameLocation();
            _downloadFileVector = new Vector();
            _objectVector = new Vector();
            _objectDirVector = new Vector();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("Peer Search Request Received with peer search UUID " + searchUUID);
    }
    
    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String peerSearchUUID) {
    	System.out.println ("Persistent Peer Search with search UUID " + peerSearchUUID + " terminated");
    }
    
    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("Peer Search Result Received with peer search UUID " + searchUUID);
    }

    public void conflictWithPrivatePeerGroup (String groupName, String peerUUID)
    {
        System.out.println ("Conflicting group " + groupName + " on peer " + peerUUID);
    }

    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data)
    {
        System.out.println ("Peer Message Received from group " + groupName);
    }
    
    public void deadPeer (String uuid)
    {
        System.out.println ("Peer - " + uuid + " died");
        //Vector dataVector = (Vector) _rmsTableModel.getDataVector();
        for (int i=0; i < _rowDataVector.size(); i++) {
            //System.out.println ("-->>deadPeer i: " + i);
            Vector v = (Vector) _rowDataVector.get (i);
            //System.out.println ("-->>deadPeer vector: " + v);
            if (!v.get(2).equals (uuid)) {
                continue;
            }
            else {
                //System.out.println ("-->>groupMemberLeft removing row: " + i);
                v.removeAllElements();
                //_rmsTableModel.removeRow (i);
                _rowDataVector.removeElementAt (i);
            }
        }
        _rmsTable.revalidate();
    }

    public void groupListChange (String uuid)
    {
        System.out.println ("Peer - " + uuid + " changed it's group list");
    }

    public void groupMemberLeft (String groupName, String memberUUID)
    {
        //System.out.println ("Group Member " + memberUUID + " left group " + groupName);
        //Vector dataVector = (Vector) _rmsTableModel.getDataVector();
        for (int i=0; i < _rowDataVector.size(); i++) {
            //System.out.println ("-->>groupMemberLeft i: " + i);
            Vector v = (Vector) _rowDataVector.get (i);
            //System.out.println ("-->>groupMemberLeft vector: " + v);
            if (!v.get(2).equals (memberUUID)) {
                continue;
            }
            else {
                //System.out.println ("-->>groupMemberLeft removing row: " + i);
                v.removeAllElements();
                //_rmsTableModel.removeRow (i);
                _rowDataVector.removeElementAt (i);
            }
        }
        _rmsTable.revalidate();
    }

    public void newGroupMember (String remoteGroupName, String memberUUID, byte[] joinData)
    {
        //System.out.println ("RMS:New Group Member in group:<" + remoteGroupName + ">" + ": " + memberUUID);
        PeerInfo pgi = _grpMgr.getPeerInfo (memberUUID);
        //System.out.println ("RMSConsole:newGroupMember:pgi: " + pgi);
        InetAddress ia = pgi.addr;
        //System.out.println ("RMSConsole:newGroupMember:addr: " + pgi.addr.getHostAddress());
        String nodeName = pgi.nodeName;
        //System.out.println ("RMSConsole:nodeName:<" + nodeName + ">");
        if (nodeName.equals ("") || nodeName.equals (" ")) {
            nodeName = ia.getHostAddress();
        }        
        int port = pgi.port;
        //System.out.println ("RMSConsole:newGroupMember:port: " + port);
        Vector v = new Vector();
        //System.out.println ("RMSConsole:nodeName: " + nodeName);
        v.addElement (nodeName);
        //System.out.println ("RMSConsole:ip: " + ia.getHostAddress());
        v.addElement (ia.getHostAddress());
        //System.out.println ("RMSConsole:memberUUID: " + memberUUID);
        v.addElement (memberUUID);
        //Add to table if it is not already there.
        //Vector dataVector = (Vector) _rmsTableModel.getDataVector();
        if (_rowDataVector.isEmpty ()) {
            //System.out.println ("-->>the data vecotr is empty!!!!");
            _rowDataVector.addElement (v);
            //_rmsTableModel.addRow (v);
        }
        else {
            boolean ok = false;
            for (int i=0; !ok && i < _rowDataVector.size(); i++) {
                Vector vec = (Vector) _rowDataVector.elementAt (i);
                //System.out.println ("-->>Vector: " + vec + " at row: " + i);
                if (vec.get(2).equals (memberUUID)) {
                   ok = true;
                }
            }
            if (!ok) {
                   // System.out.println ("-->> new member vector: " + vec);
                    //_rmsTableModel.addRow (v);
                    _rowDataVector.addElement (v);
                }
        }
        _rmsTable.revalidate();
        //_rmsTableModel.addRow (v);
    }

    public void newPeer (String uuid)
    {
        System.out.println ("RMS:Detected new peer - " + uuid);
    }
    
    public void peerGroupDataChanged (String groupName, String peerUUID, byte[] param)
    {
        System.out.println ("GroupDataChanged:GroupName: " + groupName + "\tpeerUUID: " + peerUUID);
    }
    
   
    /**
     * Build the Gui components
     */
    protected void createGui()
    {
        setResizable (true);
		setTitle ("RMS Console");
		getContentPane().setLayout (new GridBagLayout());
		setSize (700, 500);
        setVisible (false);
        GridBagConstraints gbc = new GridBagConstraints();
        
        //Panel1
        JPanel panel1 = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
		gbc.gridy = 0;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 0.8;
        gbc.weighty = 0.3;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (panel1,gbc);
        
        //Panel2
        JPanel panel2 = new JPanel (new GridBagLayout());
        gbc.gridx = 1;
		gbc.gridy = 0;
        gbc.gridwidth = 1;
        gbc.gridheight = 2;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.3;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (panel2,gbc);
        
        //Panel3
        JPanel panel3 = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
		gbc.gridy = 1;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
		gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (panel3,gbc);
        
        //Panel1 component
        gbc = new GridBagConstraints();
        Vector columnNames = new Vector (3);
        columnNames.addElement ("NodeName");
        columnNames.addElement ("IP");
        columnNames.addElement ("UUID");
        
        _rmsTableModel = new RMSTableModel (_rowDataVector, columnNames);
        _rmsTable = new JTable (_rmsTableModel) {
            public boolean isCellEditable (int row, int column) {
                return false;
            }                        
        };
        
        for (int i = 0; i < 2; i++) {
            TableColumn tableMod = _rmsTable.getColumnModel().getColumn(i);
            if (i == 0) {
                tableMod.setPreferredWidth (65);
            }
            else if (i == 1) {
                tableMod.setPreferredWidth (15);
            }
            else {
                tableMod.setPreferredWidth (190);
                
            }
        }
        
        JScrollPane ctbScrollPane = new JScrollPane (_rmsTable);
        ctbScrollPane.setViewportView (_rmsTable);
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (13,13,13,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        panel1.add (ctbScrollPane, gbc);
        
        _rmsTable.setCellSelectionEnabled (false);
        _rmsTable.setShowGrid (true);
        _rmsTable.setShowHorizontalLines (true);
        _rmsTable.setShowVerticalLines (true);
        _rmsTable.setColumnSelectionAllowed (false);
        _rmsTable.setRowSelectionAllowed (true);
        JList list = new JList();
        ListSelectionModel lsm = list.getSelectionModel();
        lsm.setSelectionMode (ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
        lsm.addListSelectionListener (new RMSTableSelectionListener());
        _rmsTable.setSelectionModel(lsm);
        
        //Panel2 components
        _startBtn = new JButton ("Start Service");
        _startBtn.setEnabled (true);
        _startBtn.setToolTipText ("Start Selected Groups");
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (_startBtn, gbc);
        _startBtn.setEnabled (false);
        
        _stopBtn = new JButton ("Stop Service");
        _stopBtn.setEnabled (true);
        _stopBtn.setToolTipText ("Stop Selected Groups");
        gbc.gridx = 1;
        gbc.gridy = 1;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (_stopBtn, gbc);
        _stopBtn.setEnabled (false);        
        
        JButton openGroupBtn = new JButton ("Open Group");
        openGroupBtn.setEnabled (true);
        openGroupBtn.setToolTipText ("Open a Private Managed Group");
        gbc.gridx = 1;
        gbc.gridy = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (openGroupBtn, gbc);
        
        _updateFileBtn = new JButton ("Update File...");
        _updateFileBtn.setEnabled (true);
        _updateFileBtn.setToolTipText ("Update Files on Joined Private Groups");
        gbc.gridx = 1;
        gbc.gridy = 3;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (_updateFileBtn, gbc);
        _updateFileBtn.setEnabled (false);
        
        _downFileBtn = new JButton ("Download File...");
        _downFileBtn.setEnabled (true);
        _downFileBtn.setToolTipText ("Download Files on Joined Private Groups");
        gbc.gridx = 1;
        gbc.gridy = 4;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (_downFileBtn, gbc);
        _downFileBtn.setEnabled (false);
        
        _updateDirBtn = new JButton ("Update Directories...");
        _updateDirBtn.setEnabled (true);
        _updateDirBtn.setToolTipText ("Update Directories on Platforms");
        gbc.gridx = 1;
        gbc.gridy = 5;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (_updateDirBtn, gbc);
        _updateDirBtn.setEnabled (false);

        _downloadDirBtn = new JButton("Download Directory");
        _downloadDirBtn.setEnabled (true);
        _downloadDirBtn.setToolTipText ("Download a Directory");
        gbc.gridx = 1;
        gbc.gridy = 6;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (_downloadDirBtn, gbc);
        _downloadDirBtn.setEnabled (false);
        
        _killBtn = new JButton ("Kill Process");
        _killBtn.setEnabled (true);
        _killBtn.setToolTipText ("Kill a process");
        gbc.gridx = 1;
        gbc.gridy = 7;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.1;
        gbc.insets = new Insets (5, 13, 5, 13);
        panel2.add (_killBtn, gbc);
        _killBtn.setEnabled (false);
        
        //Panel3 component
        gbc = new GridBagConstraints();
        _logList = new JList();
        _logListModel = (new DefaultListModel());
        _logList.setModel (_logListModel);
        JScrollPane sp = new JScrollPane (_logList);
        _logList.setBorder (new EtchedBorder());
        sp.setViewportView (_logList);
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets (5,13,13,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.2;
        gbc.weighty = 0.2;
        panel3.add (sp, gbc);
        
        _startBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                if (startService()) {
                    addLogMsg ("Successfully started service");
                }
                else {
                    addLogMsg ("The service was cancel or did not start");
                }
            }
        });
        
        _stopBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                if (stopService()) {
                    addLogMsg ("Successfully stopped service");                    
                }
                else {
                    addLogMsg ("The service was cancel or did not stop");
                }
            }
        });
        
        openGroupBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               openGroup();
            }
        });
        
        _updateFileBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               updateFileAction();
            }
        });
        
        _downFileBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               downloadFileAction();
            }
        });
        
        _updateDirBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               _objectVector.removeAllElements();
               _objectDirVector.removeAllElements();
               updateDirectoryAction();
            }
        });

        _downloadDirBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
               //_objectVector.removeAllElements();
               //_objectDirVector.removeAllElements();
                //JOptionPane.showMessageDialog (new JFrame(), "Not ready yet", "Coming Soon", JOptionPane.ERROR_MESSAGE);
               downloadDirectoryAction();
            }
        });
        
        _killBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                if (killProcess()) {
                    addLogMsg ("Successfully killed process");
                }
                else {
                    addLogMsg ("Error killing the process");
                }
            }
        });
        
        this.addWindowListener(new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                _fileRepositoryVector.removeAllElements();
                _downloadFileVector.removeAllElements();
                _rowDataVector.removeAllElements ();
                dispose();
                System.exit (1);
            }
         });
    }
    
    protected void openGroup()
    {
        Vector v = null;
        OpenGroupDialog ogd = new OpenGroupDialog();
        File filename = new File (System.getProperty ("user.dir") + FILE_BREAK + "groupName.txt");
        //System.out.println ("-->openGroup:filename: " + filename);
        if (filename.exists()) {
            v = loadFromFile (filename);
            if (v != null) {
                for (Enumeration en = v.elements(); en.hasMoreElements();) {
                    String str = (String) en.nextElement();
                    ogd.setGroupName (str);
                }
            }
        }
        
        ogd.setVisible (true);
        String reply = null;
        boolean dialogClosed = ogd.dialogIsClosing();
        if ((!ogd.wasCancelled()) && (dialogClosed == false)) {
            String groupName = ogd.getGroupName().trim();
            if (groupName == "") {
                JOptionPane.showMessageDialog (_mainWnd, "Please Enter the Group Name", "Group Error",
                                               JOptionPane.ERROR_MESSAGE);
                return;
            }
            if (v == null) {
                v = new Vector();
                v.addElement (groupName);
            }
            else {
                v.removeAllElements();
                v.addElement (groupName);
            }
            boolean isSaved = saveToAFile (v, filename);
            //System.out.println ("-->openGroup:isSaved: " + isSaved);
            if (!isSaved) {
                addLogMsg ("Unable to save group " + groupName + " to a file");
            }
            
            String password = ogd.getPassword().trim();
            if ((password.equals ("")) || (password == null)){
                JOptionPane.showMessageDialog (new JFrame(), "Please Enter a Password", "Invalid Password",
                                               JOptionPane.ERROR_MESSAGE);
                return;
            }
            createManagedGroup (groupName, password, v);
        }
    }
    
    protected void createManagedGroup (String groupName, String password, Vector v)
    {
        try {
            System.out.println ("->>RMSConsole:creating a PrivateManagedGroup: " + groupName);
            _grpMgr.createPrivateManagedGroup (groupName, password);
        }
        catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog (new JFrame(), "The group already exists or the password is invalid", "GroupManager Error",
                                           JOptionPane.ERROR_MESSAGE);
            v.removeAllElements();
            return;
        }
    }
    
    protected void updateFileAction()
    {
        int[] selNum = _rmsTable.getSelectedRows();
        //System.out.println ("-->>Num of selections: " + selNum.length);
        Object[] selGrps = new Object [selNum.length];
        
        for (int j = 0; j < selNum.length; j++) {            
            Vector v = (Vector) _rmsTableModel.getRowElements (selNum [j]);
            InfoObject infoObj = new InfoObject();
            infoObj.grpName = (String) v.elementAt (0);
            infoObj.grpIP = (String) v.elementAt (1);
            infoObj.memberUUID = (String) v.elementAt (2);
            selGrps [j] = infoObj;
        }
        
        UpdateFileDialog ufd = new UpdateFileDialog();
        ufd.init (_fileRepositoryVector);
        ufd.setVisible (true);
        if (ufd.update()) {            
            String localFilePath = ufd.getLocalFilePath().trim();
            String remoteFilePath = ufd.getRemoteAbsolutePath().trim();
            //System.out.println ("-->localFilePath in update: " + localFilePath);
            if (_fileRepositoryVector.isEmpty()) {
                _fileRepositoryVector.addElement (localFilePath);
            }
            else {
                if (!_fileRepositoryVector.contains (localFilePath)) {
                    _fileRepositoryVector.addElement (localFilePath);
                }
            }
            
            File localFile = new File (localFilePath);
            long fileLength = localFile.length();
            if (fileLength == 0) {
                JOptionPane.showMessageDialog (this, localFilePath + " is not valid", "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            for (int i = 0; i < selGrps.length; i++) {
                InfoObject grpObj = (InfoObject) selGrps[i];
                UpdateFileThread fileThread = new UpdateFileThread (grpObj, localFile, fileLength, remoteFilePath);
                fileThread.start();
            }
        }
    }
    
    
    /**
     * Update files in a remote platform
     * 
     * @param   fis the file input stream for transfering the file
     * @param	fileSize the size of the file to be transfered
     */
    private boolean updateFile (String grpIP, FileInputStream fis, long fileSize, String remotePath)
    {
        CommHelper ch = null;
        boolean success = true;
        try {
            ch = connectToPlatform (grpIP);
            //System.out.println ("-->UpdateFile: at RemotePath: " + remotePath);
            if (transferFile (ch, fis, fileSize, remotePath)) {
                success = true;
            }
            else {
                success = false;
            }
            
        }
        catch (Exception e) {
            success = false;
            addLogMsg ("Failed to update RMS - " + "\r\n" + e);
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
     */
    private boolean transferFile (CommHelper ch, FileInputStream fis, long fileSize, String remoteRelativePath)
    {
        boolean success = true;
        try {
            //System.out.println ("-->***Transfer File at RemotePath:UPDATE " + remoteRelativePath);
            ch.sendLine ("UPDATE " + remoteRelativePath + " " + fileSize);
            ch.receiveMatch ("OK");
            byte[] buf = new byte [1024];
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
            addLogMsg ("Failed to update file - " + "\r\n" + e);
            e.printStackTrace();
        }
        return success;
    }

    /**
     * Connect to a platform using sockets
     * 
     * @param	grpIP the group IP
     */
    private CommHelper connectToPlatform (String grpIP)
        throws Exception
    {
        CommHelper ch = new CommHelper();
        ch.init (new Socket (grpIP, _rmsPort));
        return ch;
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
    
    protected void downloadFileAction()
    {
        int[] selNum = _rmsTable.getSelectedRows();
        Object[] selGrps = new Object [selNum.length];
        String grpIP = null;
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _rmsTableModel.getRowElements (selNum [j]);
            String grpName = (String) v.elementAt (0);
            grpIP = (String) v.elementAt (1);
            String memberUUID = (String) v.elementAt (2);
        }
        if (selGrps.length != 1) {
            JOptionPane.showMessageDialog (this, "Must select one group from which to download file", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        DownloadFileDialog dfg = new DownloadFileDialog();
        dfg.init (_downloadFileVector);
        dfg.setVisible (true);
        
        if (dfg.download()) {
            String localFilePath = dfg.getLocalFilePath();
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
            
            String remotePath = dfg.getRemotePath();
            //System.out.println ("-->******The localFilePath: " + localFilePath);
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
            if (!downloadFile (grpIP, fos, remotePath)) {
                JOptionPane.showMessageDialog (this, "Error download file " + remotePath + " from RMS at " + grpIP, "Error", JOptionPane.ERROR_MESSAGE);
            }
            else {
                addLogMsg ("Successfully downloaded " + remotePath + " from remote site at " + grpIP);
            }
            try {
                fos.close();
            }
            catch (Exception e) {}
        }
    }
    
     /**
     * Download a file from a remote platform
     * 
     * @param   grpIP the group IP
     * @param   fos the file output stream for downloading the file
     */
    protected boolean downloadFile (String grpIP, FileOutputStream fos, String remotePath)
    {
        System.out.println ("-->downloadFile:remotePath: " + remotePath);
        CommHelper ch = null;
        boolean success = true;
        try {
            ch = connectToPlatform (grpIP);
            System.out.println ("-->downloadFile:sending DOWNLOAD cmd");
            ch.sendLine ("DOWNLOAD " + remotePath);
            ch.receiveMatch ("OK");
            System.out.println ("-->downloadFile:after receiving OK");
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
            addLogMsg ("Failed to download file - " + "\r\n" + e);
        }
        if (ch != null) {
            ch.closeConnection();
        }
        return success;
    }

         /**
     * Download a file from a remote platform
     *
     * @param   grpIP the group IP
     * @param   fos the file output stream for downloading the file
     */
    protected boolean downloadFile (CommHelper ch, String grpIP, FileOutputStream fos, String remotePath)
    {
        //System.out.println ("-->downloadFile:remotePath: " + remotePath);
        boolean success = true;
        try {
            ch.sendLine ("DOWNLOAD " + remotePath);
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
            addLogMsg ("Failed to download file - " + "\r\n" + e);
        }
        return success;
    }
    
    /**
     * Synchronize directories in local and remote machines.
     */
    protected void updateDirectoryAction()
    {
        int[] selNum = _rmsTable.getSelectedRows();
        Object[] selGrps = new Object [selNum.length];
        String grpIP = null;
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _rmsTableModel.getRowElements (selNum [j]);
            InfoObject infoObj = new InfoObject();
            infoObj.grpName = (String) v.elementAt (0);
            infoObj.grpIP = (String) v.elementAt (1);
            infoObj.memberUUID = (String) v.elementAt (2);
            selGrps [j] = infoObj;
        }
        
        UpdateDirectoryDialog udd = new UpdateDirectoryDialog();
        udd.setVisible (true);
        if (udd.update()) {
            setCursorToWait (true);
            String localFilePath = udd.getLocalFilePath();
            String remotePath = udd.getRemotePath();
            _objectVector.removeAllElements();
            _objectDirVector.removeAllElements();
            //for (int i = 0; i < selGrps.length; i++) {
                //Get list of directories from the console
            String currRelPath = null;
            getDirStructure (localFilePath, currRelPath);

            //InfoObject grpObj = (InfoObject) selGrps[i];
            boolean updateIsStrict = udd.updateStrict();
            if (updateIsStrict) {
                if (1 == JOptionPane.showConfirmDialog (this, "Do you want to delete all directories and files in " +
                                                              remotePath + "?", "Confirm", JOptionPane.YES_NO_OPTION)) {
                    return;
                }
            }
            for (int i = 0; i < selGrps.length; i++) {
                InfoObject grpObj = (InfoObject) selGrps[i];
                SynchDirectoriesThread synchDirThread = new SynchDirectoriesThread (grpObj, updateIsStrict, localFilePath, remotePath);
                synchDirThread.start();
            }
            setCursorToWait (false);
        }
    }
    
    /**
     * Recursively find files and directories in the specified initial path and
     * add entries into two vectors, one a list of directories and the other a
     * list of files.
     * 
     * @param initialPath    the starting path for the directory listing
     * @param currRelPath    used during recursion - caller should pass null
     */
    private void getDirStructure (String initialPath, String currRelPath)
    {
        //System.out.println ("-->getDirStructure:initialPath is: " + initialPath);
        //System.out.println ("-->getDirStructure:currRelPath is: " + currRelPath);
        String currDir = initialPath;
        if ((currRelPath != null) && (currRelPath.length() > 0)) {
            if (!currDir.endsWith (File.separator)) {
                currDir += File.separator;
            }
            currDir += currRelPath;
        }
        File dirToSearch = new File (currDir);
        File[] dirContents = dirToSearch.listFiles();
        for (int i = 0; i < dirContents.length; i++) {
            String relPathToEntry = currRelPath;
            if (relPathToEntry == null) {
                relPathToEntry = dirContents[i].getName();
            }
            else {
                if (!relPathToEntry.endsWith (File.separator)) {
                    relPathToEntry += File.separator;
                }
                relPathToEntry += dirContents[i].getName();
            }

            if (dirContents[i].isDirectory()) {
                //System.out.println ("-->getDirStructure:is a directory: " + initialPath);
                MirrorDirObject mdo = new MirrorDirObject (relPathToEntry, null);
                _objectDirVector.addElement (mdo);
                //System.out.println ("-->getDirStructure recurse calling:currRelPath is:relPathToEntry: " + relPathToEntry);
                getDirStructure (initialPath, relPathToEntry);
            }
            else if (dirContents[i].isFile()) {
                //System.out.println ("-->getDirStructure:is file: " + dirContents[i].getAbsoluteFile());
                String md5HashFile = getMD5String (dirContents[i].getAbsoluteFile());
                MirrorDirObject mdo = new MirrorDirObject (relPathToEntry, md5HashFile);
                _objectVector.addElement (mdo);
            }
        }
    }
        
     /**
     * This method allows to mirror a directory on the remote site and update it
     *
     * @param	isStrict indicates if a mirror copy is desired
     */
    private boolean updateDirectories (String grpIP, boolean isStrict, String localPath, String remFilePath)
    {
        Vector resultsVector = new Vector();
        Vector resultsDirVector = new Vector();
        Vector aIntbVector = new Vector();
        Vector aMinbVector = new Vector();
        Vector bMinaVector = new Vector();
        Vector localVector = new Vector();
        Vector remoteVector = new Vector();
        Hashtable ht = new Hashtable();
        
        CommHelper ch = null;
        boolean success = true;
        try {
            ch = connectToPlatform (grpIP);
            ch.sendLine ("SEND_DIRS " + remFilePath);
            ch.receiveMatch ("OK");

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
                    else if (fileNames[0].equals ("FILE")) {
                        //Store the MD5 string and file name into a hashtable
                        String remFileName = fileNames[1];
                        ht.put (fileNames[2], remFileName);
                        resultsVector.addElement (remFileName);
                    }
                }
            }

            checkDirectories (resultsDirVector, ch, isStrict, remFilePath);
            
            for (Enumeration enx1 = _objectVector.elements(); enx1.hasMoreElements();) {
                MirrorDirObject localMdo = (MirrorDirObject) enx1.nextElement();
                String localFilePath = localMdo._filePath;
                if (localFilePath.indexOf ("/") > 0) {
                    localFilePath.replaceAll ("/", "\\");
                }
                localVector.addElement (localFilePath);
            }
            
            boolean areDiff = Diff.vectorDiff (localVector, resultsVector, aIntbVector, aMinbVector,
                                               bMinaVector);
            
            FileInputStream fis = null;
            long fileLength = 0;
            if (!aMinbVector.isEmpty()) {
                //Send the new files to remote system
                for (Enumeration en2 = aMinbVector.elements(); en2.hasMoreElements();) {
                    String fileStr = (String) en2.nextElement();
                    if (fileStr == null) {
                        System.out.println ("-->No files to send");
                    }
                    fileStr = fileStr.trim();
                    try {
                        String localFileStr = localPath + File.separator + fileStr;
                        File localFile = new File (localFileStr);
                        fileLength = localFile.length();
                        fis = new FileInputStream (localFile);
                    }
                    catch (Exception ex) {
                        JOptionPane.showMessageDialog (this, "Error opening file1 " + fileStr, "Error", JOptionPane.ERROR_MESSAGE);
                        return false;
                    }
                    if (fileLength <= 0) {
                        System.out.println ("-->The file is empty!");
                    }
                    //System.out.println ("-->****Transfering at: " + remFilePath + File.separator + fileStr);
                    if (!transferFile (ch, fis, fileLength, remFilePath + File.separator + fileStr)) {
                        JOptionPane.showMessageDialog (this, "Error updating directory at " + grpIP, "Error", JOptionPane.ERROR_MESSAGE);
                        return false;
                    }
                    else {
                        addLogMsg ("Successfully updated file1 " + getNameFromPath (fileStr) + " at " + grpIP);
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
                    //System.out.println ("-->UpdateDirectories3:bMinaVector:DELETE_FILE: " + remFilePath+ File.separator + relPath2);
                    ch.sendLine ("DELETE_FILE " + remFilePath + File.separator + relPath2);
                }
            }
            else {
                bMinaVector.removeAllElements();
            }
            
            //Delete directories recursively after delete the files.
            if ((!_bMinDiraVector.isEmpty()) && (isStrict)) {
                Comparator myComp = new Comparator() {
                    public int compare (Object objA, Object objB)
                    {
                        if ((objA instanceof String) && (objB instanceof String)) {
                            String strA = (String) objA;
                            String strB = (String) objB;
                            
                            int countA = countSlashes (strA);
                            int countB = countSlashes (strB);
                            
                            return (countA - countB);
                        }
                        else {
                            return 0;
                        }
                    }
                    
                    private int countSlashes (String str)
                    {
                        char fileSepChar = File.separatorChar;
                        int count = 0;
                        
                        for (int i = 0; i < str.length(); i++) {
                            if (str.charAt (i) == fileSepChar) {
                                count++;
                            }
                        }
                        
                        return count;
                    }
                }; // comp.
                
                Collections.sort (_bMinDiraVector, myComp);
                //Delete the files in remote system if the strict button is selected.
                for (Enumeration en3 = _bMinDiraVector.elements(); en3.hasMoreElements();) {
                    String strDir2 = (String) en3.nextElement();
                    try {
                        //System.out.println ("-->updateDirectories5:_bMinDiraVector:DELETE_DIR: " + remFilePath+ File.separator +strDir2);
                        ch.sendLine ("DELETE_DIR " + remFilePath+ File.separator+ strDir2);
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
                    //System.out.println ("-->updateDirectories6:mdo3._filePath: " + mdo3._filePath);
                    if (mdo3._filePath.endsWith ("rms.exe")) {
                        break;
                    }
                    for (Enumeration enun = aIntbVector.elements(); enun.hasMoreElements();) {
                        String strPath = (String) enun.nextElement();
                        String remValue = (String) ht.get (strPath);
                        if (strPath.equals (mdo3._filePath)) {
                            String value = mdo3._md5value;
                            if (!value.equals (remValue)) {
                                String pathStrToSend = mdo3._filePath;
                                long lengthFileToSend = 0;
                                try {
                                    String localFileStrToSend = localPath + File.separator + pathStrToSend;
                                    File localFileToSend = new File (localFileStrToSend);
                                    lengthFileToSend = localFileToSend.length();
                                    fisRem = new FileInputStream (localFileToSend);
                                }
                                catch (Exception ex) {
                                    JOptionPane.showMessageDialog (this, "Error opening file2 " + pathStrToSend, "Error", JOptionPane.ERROR_MESSAGE);
                                    return false;
                                }
                                if (!transferFile (ch, fisRem, lengthFileToSend, remFilePath + File.separator + pathStrToSend)) {
                                    JOptionPane.showMessageDialog (this, "Error updating directory at " + grpIP, "Error", JOptionPane.ERROR_MESSAGE);
                                    return false;
                                }
                                else {
                                    addLogMsg ("Successfully updated file2 " + getNameFromPath (pathStrToSend) + " at " + grpIP);
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
        resultsVector.removeAllElements();
        resultsDirVector.removeAllElements();
        aIntbVector.removeAllElements();
        aMinbVector.removeAllElements();
        bMinaVector.removeAllElements();
        localVector.removeAllElements();
        remoteVector.removeAllElements();
        _bMinDiraVector.removeAllElements();
        ht.clear();
        
        return success;
    }
    
    /**
     * Check if the directories exists in the remote site. Otherwise it creates them
     * 
     * @param   ch a handle to the Nomads CommHelper
     * @param   remDirVector vector of remote directories
     * @param	isStrict indicates if a mirror copy is desired 
     */
    protected void checkDirectories (Vector remDirVector, CommHelper ch, boolean isStrict, String remFilePath)
    {
        Vector aIntbVector = new Vector();
        Vector aMinbVector = new Vector();
        Vector localVector = new Vector();
        Vector remoteVector = new Vector();
        _bMinDiraVector.removeAllElements();

        for (Enumeration enx = _objectDirVector.elements(); enx.hasMoreElements();) {
            MirrorDirObject mdo1 = (MirrorDirObject) enx.nextElement();
            //Check local dirs for forward slashes and change them to back slashes if there are any.
            String localFilePath = mdo1._filePath;
            //System.out.println ("-->checkDirectories:localFilePath1: " + localFilePath);
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
                    //System.out.println ("-->checkDirectories:aMinbVector: " + remFilePath + File.separator + str2);
                    if (str2 == null) {
                        System.out.println ("-->Object dir is null");
                        return;
                    }
                    try {
                        ch.sendLine ("CREATE_DIR " + remFilePath + File.separator + str2);
                    }
                    catch (Exception e) {
                        System.out.println ("-->CREATE_DIR Exception: " + e);
                    }
                }
            }
            //Delete directories.
            if ((!_bMinDiraVector.isEmpty()) && (isStrict)) {
                //Delete the files in remote system if the strict button is selected.
                for (Enumeration en3 = _bMinDiraVector.elements(); en3.hasMoreElements();) {
                    String strDir2 = (String) en3.nextElement();
                    try {
                        //System.out.println ("-->checkDirectories:_bMinDiraVector2:DELETE_DIR: " + remFilePath + File.separator + strDir2);
                        ch.sendLine ("DELETE_DIR " + remFilePath + File.separator + strDir2);
                    }
                    catch (Exception ex) {
                        System.out.println ("-->DELETE_FILE Exception: " + ex);
                    }
                }
            }
            else {
                _bMinDiraVector.removeAllElements();
            }
        }
        aIntbVector.removeAllElements();
        aMinbVector.removeAllElements();
        localVector.removeAllElements();
        remoteVector.removeAllElements();
    }

    protected void downloadDirectoryAction ()
    {
        int[] selNum = _rmsTable.getSelectedRows();
        Object[] selGrps = new Object [selNum.length];
        String grpIP = null;
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _rmsTableModel.getRowElements (selNum [j]);
            InfoObject infoObj = new InfoObject();
            infoObj.grpName = (String) v.elementAt (0);
            infoObj.grpIP = (String) v.elementAt (1);
            infoObj.memberUUID = (String) v.elementAt (2);
            selGrps [j] = infoObj;
        }

        DownloadDirectoryDialog ddd = new DownloadDirectoryDialog();
        ddd.setVisible (true);
        String localFilePath = null;
        String remotePath = null;
        if (ddd.download ()) {
            setCursorToWait (true);
            localFilePath = ddd.getLocalFilePath();
            remotePath = ddd.getRemotePath();
        }

        for (int i = 0; i < selGrps.length; i++) {
            InfoObject grpObj = (InfoObject) selGrps[i];
            if (!downloadDirectory (grpObj.grpIP, remotePath, localFilePath)) {
                JOptionPane.showMessageDialog (new JFrame(), "Error updating File at " + grpObj.grpIP, "Error", JOptionPane.ERROR_MESSAGE);
            }
            else {
                addLogMsg ("Successfully downloaded directories for: " + grpObj.grpIP);
            }
        }
        setCursorToWait (false);
    }

    private boolean downloadDirectory (String grpIP, String remFilePath, String localFilePath)
    {
        Vector resultsFileVector = new Vector();
        Vector resultsDirVector = new Vector();
        CommHelper ch = null;
        boolean success = true;
        
        try {
            //System.out.println ("-->**downloadDirectory:remFilePath: " + remFilePath);
            //System.out.println ("-->**downloadDirectory:localFilePath: " + localFilePath);
            ch = connectToPlatform (grpIP);
            ch.sendLine ("SEND_DIRS " + remFilePath);
            ch.receiveMatch ("OK");

            if (localFilePath.indexOf ("/") > 0) {
                localFilePath.replaceAll ("/", "\\");
            }            
                      
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
                    if (fileNames[0].equals ("DIR")) {
                        resultsDirVector.addElement (fileNames[1]);
                    }
                    else if (fileNames[0].equals ("FILE")) {
                        String remFileName = fileNames[1];
                        resultsFileVector.addElement (remFilePath + FILE_BREAK + remFileName);
                    }
                }
            }
            //Checks for directories
            if (!resultsDirVector.isEmpty()) {
                for (Enumeration en = resultsDirVector.elements(); en.hasMoreElements();) {
                    String remDirPath = (String) en.nextElement();
                    File f = new File (localFilePath + FILE_BREAK + remDirPath);
                    f.mkdirs ();
                }
            }
            else {
                System.out.println ("-->No directories to download");
            }
            //Checks for files
            if (!resultsFileVector.isEmpty()) {
                for (Enumeration en2 = resultsFileVector.elements(); en2.hasMoreElements();) {
                    String remFile = (String) en2.nextElement();
                    File localFile = new File (remFile);
                    if (localFile.exists()) {
                        if (1 == JOptionPane.showConfirmDialog (this, "Overwrite File " + localFilePath +"?", "Confirm", JOptionPane.YES_NO_OPTION)) {
                            return false;
                        }
                    }
                    FileOutputStream fos = null;
                    try {
                        fos = new FileOutputStream (localFile);
                    }
                    catch (Exception e) {
                        JOptionPane.showMessageDialog (this, "Error opening file " + remFile, "Error", JOptionPane.ERROR_MESSAGE);
                    }
                    if (!downloadFile (ch, grpIP, fos, remFile)) {
                        //System.out.println ("-->calling downloadFile at remFile: " + remFile);
                        JOptionPane.showMessageDialog (this, "Error download file " + remFile + " from RMS at " + grpIP, "Error", JOptionPane.ERROR_MESSAGE);
                    }
                    else {
                        addLogMsg ("Successfully downloaded " + remFile + " from remote site at " + grpIP);
                    }
                    try {
                        fos.close();
                    }
                    catch (Exception e) {}
                 }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        if (ch != null) {
            ch.closeConnection();
        }
        return success;
    }
        
     /**
     * Get the MD5 string from a file.
     * 
     * @param	localFile the file which the MD5 is calculated
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
    
    protected boolean startService()
    {
        boolean success = false;
        int[] selNum = _rmsTable.getSelectedRows();
        //System.out.println ("-->>Num of selections: " + selNum.length);
        Object[] selGrps = new Object [selNum.length];
        String grpIP = null;
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _rmsTableModel.getRowElements (selNum [j]);
            InfoObject infoObj = new InfoObject();
            infoObj.grpName = (String) v.elementAt (0);
            infoObj.grpIP = (String) v.elementAt (1);
            infoObj.memberUUID = (String) v.elementAt (2);
            selGrps [j] = infoObj;
        }
        
        ServiceNameDialog snd = new ServiceNameDialog();
        snd.setVisible (true);
        if ((!snd.wasCancelled()) && (snd.dialogIsClosing() == false)) {
            String serviceName = snd.getServiceName();
            CommHelper ch = null;
            
            for (int i = 0; i < selGrps.length; i++) {
                InfoObject grpObj = (InfoObject) selGrps[i];
                System.out.println ("-->startService at:grpIP: " + grpObj.grpIP);
                
                try {
                    ch = connectToPlatform (grpObj.grpIP);
                    //System.out.println ("-->startService at:connected to: " + grpObj.grpIP);
                    ch.sendLine ("START " + serviceName);
                    ch.receiveMatch ("OK");
                    success = true;
                }
                catch (Exception e) {
                    success = false;
                    addLogMsg ("Failed to START service - " + "\r\n" + e);
                    e.printStackTrace();
                }
                if (ch != null) {
                    ch.closeConnection();
                }
            }
        }
        
        return success;
    }
    
    protected boolean stopService()
    {
        int[] selNum = _rmsTable.getSelectedRows();
        Object[] selGrps = new Object [selNum.length];
        String grpIP = null;
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _rmsTableModel.getRowElements (selNum [j]);
            InfoObject infoObj = new InfoObject();
            infoObj.grpName = (String) v.elementAt (0);
            infoObj.grpIP = (String) v.elementAt (1);
            infoObj.memberUUID = (String) v.elementAt (2);
            selGrps [j] = infoObj;
        }
        
        ServiceNameDialog snd = new ServiceNameDialog ();
        snd.setVisible (true);
        boolean success = false;
        if ((!snd.wasCancelled()) && (snd.dialogIsClosing() == false)) {
            String serviceName = snd.getServiceName();
            CommHelper ch = null;            
            for (int i = 0; i < selGrps.length; i++) {
                InfoObject grpObj = (InfoObject) selGrps[i];
                System.out.println ("-->Stop service at:grpIP: " + grpObj.grpIP);
                try {
                    ch = connectToPlatform (grpObj.grpIP);
                    System.out.println ("-->Stop service sending:serviceName: " + serviceName);
                    ch.sendLine ("STOP " + serviceName);
                    System.out.println ("-->Stop service sending:waiting for OK");
                    ch.receiveMatch ("OK");
                    success = true;
                }
                catch (Exception e) {
                    success = false;
                    addLogMsg ("Failed to STOP service - " + "\r\n" + e);
                    e.printStackTrace();
                }
                if (ch != null) {
                    ch.closeConnection();
                }
            }
        }
        return success;
    }
    
    protected boolean killProcess()
    {
        boolean success = false;
        int[] selNum = _rmsTable.getSelectedRows();
        //System.out.println ("-->>Num of selections: " + selNum.length);
        Object[] selGrps = new Object [selNum.length];
        String grpIP = null;
        for (int j = 0; j < selNum.length; j++) {
            Vector v = (Vector) _rmsTableModel.getRowElements (selNum [j]);
            InfoObject infoObj = new InfoObject();
            infoObj.grpName = (String) v.elementAt (0);
            infoObj.grpIP = (String) v.elementAt (1);
            infoObj.memberUUID = (String) v.elementAt (2);
            selGrps [j] = infoObj;
        }
        
        ProcessNameDialog pnd = new ProcessNameDialog();
        pnd.setVisible (true);
        if ((!pnd.wasCancelled()) && (pnd.dialogIsClosing() == false)) {
            String processName = pnd.getProcessName();
            
            CommHelper ch = null;           
            for (int i = 0; i < selGrps.length; i++) {
                InfoObject grpObj = (InfoObject) selGrps[i];
                System.out.println ("-->startService at:grpIP: " + grpObj.grpIP);
                
                try {
                    ch = connectToPlatform (grpObj.grpIP);
                    //System.out.println ("-->startService at:connected to: " + grpObj.grpIP);
                    ch.sendLine ("KILL " + processName);
                    ch.receiveMatch ("OK");
                    success = true;
                }
                catch (Exception e) {
                    success = false;
                    addLogMsg ("Failed to KILL process - " + "\r\n" + e);
                    e.printStackTrace();
                }
                if (ch != null) {
                    ch.closeConnection();
                }
            }
        }
        
        return success;
    }
    
    protected boolean saveToAFile (Vector v, File filename)
    {
        boolean success = false;
        try {
            if (!filename.exists()) {
                filename.createNewFile();
            }
            if (filename.length() < 0) {
                JOptionPane.showMessageDialog (_rmsConsole, filename + " is not valid", "Error", JOptionPane.ERROR_MESSAGE);
                return false;
            }
            System.out.println ("Saving file to: " + filename.getAbsolutePath());
            FileOutputStream fos = new FileOutputStream (filename);
            ObjectOutputStream oos = new ObjectOutputStream (fos);
            if (v.isEmpty()) {
                System.out.println ("<<Error:No Service Names>>");
            }
            else {
                oos.writeObject (v);
                success = true;
            }
            oos.close();
        }
        catch (Exception e) {
            success = false;
            e.printStackTrace();
        }
        
        return success;
    }
    
    protected Vector loadFromFile (File filename)
    {
        ObjectInputStream ois = null;
        Vector vec = null;
        if (filename.length() <= 0) {
            vec = null;
        }
        else {
            try {
                try {
                    ois = new ObjectInputStream (new FileInputStream (filename));
                    vec = (Vector) ois.readObject();
                    for (Enumeration en = vec.elements(); en.hasMoreElements();) {
                        String str = (String) en.nextElement();
                        System.out.println ("Service: " + str);
                    }
                }
                finally {
                    if (ois != null) {
                        ois.close();
                    }
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
        return vec;
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
     * Show the messages in the Log list
     * 
     * @param    msg the message to show in the list
     */
    protected void addLogMsg (String msg)
    {
        String time = formatDate (System.currentTimeMillis());
        _logListModel.addElement ("[" + time + "] " + msg);
        int position = _logListModel.getSize();
        _logList.ensureIndexIsVisible (position-1); //Show last row of the list
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
    
    public static void main (String args[])
    {
        RMSConsole rmsConsole = new RMSConsole();
        rmsConsole.init();
        rmsConsole.setVisible (true);
    }
    
    //Inner Classes
    
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
     * The Info object that contains the group name, member UUID and the ip of
     * the selected group
     */
    public class InfoObject implements Serializable
    {
        public InfoObject()
        {
        }

        public String grpName;
        public String grpIP;
        public String memberUUID;
    }
    
    protected class UpdateFileThread extends Thread
    {
        public UpdateFileThread (InfoObject grpObj, File localFile, long fileLength, String remFilePath)
        {
            //System.out.println ("<<---->>>UpdateFileThread: " + remFilePath);
            _grpObj = grpObj;
            _localFile = localFile;
            _fileLength = fileLength;
            _remFilePath = remFilePath;
        }
        
        public void run()
        {
            FileInputStream fis = null;
            try {
               fis = new FileInputStream (_localFile);
            }
            catch (Exception e) {
                JOptionPane.showMessageDialog (new JFrame(), "Error opening file " + _remFilePath, "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            //System.out.println ("<<---->>>UpdateFileThread: " + _remFilePath);
            if (!updateFile (_grpObj.grpIP, fis, _fileLength, _remFilePath)) {
                JOptionPane.showMessageDialog (new JFrame(), "Error updating RMS at " + _grpObj.grpIP, "Error", JOptionPane.ERROR_MESSAGE);
            }
            else {
                addLogMsg ("Successfully updated " + _remFilePath + " at " + _grpObj.grpIP);
            }
            try {
                fis.close();
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
        
        protected InfoObject _grpObj;
        protected File _localFile;
        protected long _fileLength;
        protected String _remFilePath;
    }
    
    protected class SynchDirectoriesThread extends Thread
    {
        public SynchDirectoriesThread (InfoObject grpObj, boolean updateIsStrict, String localFilePath, String remoteFilePath)
        {
            _grpObj = grpObj;
            _updateIsStrict = updateIsStrict;
            _localFilePath = localFilePath;
            _remoteFilePath = remoteFilePath;
        }
        
        public void run()
        {
            if (!updateDirectories (_grpObj.grpIP, _updateIsStrict, _localFilePath, _remoteFilePath)) {
                JOptionPane.showMessageDialog (new JFrame(), "Error updating directory at " + _grpObj.grpIP, "Error", JOptionPane.ERROR_MESSAGE);
            }
            else {
                addLogMsg ("Successfully updated all directories at: " + _grpObj.grpIP);
            }
        }
        
        protected InfoObject _grpObj;
        protected boolean _updateIsStrict;
        protected String _localFilePath;
        protected String _remoteFilePath;
    }
    
    /**
     * Table Model for the Platforms
     */
    public class RMSTableModel extends DefaultTableModel
    {
        public RMSTableModel (Vector data, Vector columnNames)
        {
            super (data, columnNames);
        }
        
        // Get the data from each row of the table.
        public Vector getRowElements (int row)
        {        
            Vector rowVector = new Vector();
            String nodeName = (String) getValueAt(row, 0);
            rowVector.addElement (nodeName);
            String ip = (String) getValueAt (row, 1);
            rowVector.addElement (ip);
            String uuid = (String) getValueAt (row, 2);
            rowVector.addElement (uuid);
            
            return rowVector;
        }
    }
    
     /**
     *  List selection Listener for policy list
     */
    private class RMSTableSelectionListener implements ListSelectionListener
    {
        public void valueChanged (ListSelectionEvent e) 
        {
            int[] rowCount = _rmsTable.getSelectedRows();
            if (rowCount.length > 0) {
                _startBtn.setEnabled (true);
                _stopBtn.setEnabled (true);
                _downFileBtn.setEnabled (true);
                _updateDirBtn.setEnabled (true);
                _updateFileBtn.setEnabled (true);
                _downloadDirBtn.setEnabled (true);
                _killBtn.setEnabled (true);
            } 
            else {
                _startBtn.setEnabled (false);
                _stopBtn.setEnabled (false);
                _updateDirBtn.setEnabled (false);
                _downFileBtn.setEnabled (false);
                _updateFileBtn.setEnabled (false);
                _downloadDirBtn.setEnabled(false);
                _killBtn.setEnabled (false);
            }
        }
    }
    
    //Class variables
    public static final int DEFAULT_GROUP_MANAGER_PORT_NUM = 1126;
    public static final int DEFAULT_RMS_PORT_NUM = 1125;
    private ConfigLoader _cfgLoader;
    private GroupManager _grpMgr;
    private int _grpMgrPort;
    private int _rmsPort;
    protected DefaultListModel _logListModel;
    protected JButton _downFileBtn;
    protected JButton _killBtn;
    protected JButton _startBtn;
    protected JButton _stopBtn;
    protected JButton _updateDirBtn;
    protected JButton _updateFileBtn;
    protected JButton _downloadDirBtn;
    protected JFrame _mainWnd;
    protected JList _logList;
    protected JTable _rmsTable;
    protected RMSTableModel _rmsTableModel;
    protected RMSConsole _rmsConsole = this;
    protected String FILE_BREAK = System.getProperty ("file.separator");
    protected Vector _bMinDiraVector = new Vector();
    protected Vector _fileRepositoryVector;
    protected Vector _downloadFileVector;
    protected Vector _objectVector;
    protected Vector _objectDirVector;
    protected Vector _rowDataVector;
}