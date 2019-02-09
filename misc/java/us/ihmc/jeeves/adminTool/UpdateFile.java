package us.ihmc.jeeves.adminTool;

import java.io.File;
import java.util.Enumeration;
import java.util.logging.Logger;
import java.util.Vector;

import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import javax.swing.*;
import javax.swing.border.BevelBorder;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
//import us.ihmc.util.LoggerWrapper;

/**
 * Update files on the remote site.
 *
 * @author  Maggie Breedy
 * @version $Revision$
 * $Date$
 * 
 */
public class UpdateFile extends JDialog 
{
    public UpdateFile()
    {        
        _update = false;
    }
    
    public void init (Vector pathsVector)
    {
        //_logger = Logger.getLogger ("us.ihmc.jeeves.adminTool");
        //System.out.println ("-->pathsVector: " + pathsVector);
        _pathsVector = pathsVector;
        createGui();
    }
    
    protected void createGui()
    {
        setModal (true);
		setTitle ("Upload File");
        setSize (500, 220);
		getContentPane().setLayout (new GridBagLayout());
		GridBagConstraints gbc = new GridBagConstraints();
		setVisible (false);
        setDlgLocation();
        
        // Add upper Panel
        JPanel upPanel = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.3;
        gbc.weighty = 0.3;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        getContentPane().add (upPanel, gbc);
        
        // Add buttons Panel
        JPanel btnPanel = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 0.2;
        gbc.weighty = 0.2;
        gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        getContentPane().add (btnPanel, gbc);
        
        //Up panel components
        gbc = new GridBagConstraints();
        JLabel jLabel1 = new JLabel ("Local File");
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (5,13,5,13);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        upPanel.add (jLabel1, gbc);
        
        //_jtfLocalFile = new JTextField();
        _jtfComboModel = new DefaultComboBoxModel();
        _jtfComboBox = new JComboBox (_jtfComboModel);
        _jtfComboBox.setEditable (true);
        
        Dimension dim = _jtfComboBox.getMinimumSize();
        int w = (int) dim.getWidth();
        int h = (int) dim.getHeight();
        _jtfComboBox.setMaximumSize (new Dimension (290, h));
        _jtfComboBox.setPreferredSize (new Dimension (290, h));
        //System.out.println ("-->_pathsVector: " + _pathsVector);
        if (_jtfComboBox.getItemCount() != 0) {
            _jtfComboModel.removeAllElements();
        }
        if (!_pathsVector.isEmpty()) {
           for (Enumeration e = _pathsVector.elements(); e.hasMoreElements();) {
                _jtfComboModel.addElement ((String) e.nextElement());
            } 
        }
        else {
            _jtfComboModel.addElement ("Edit");
        }
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 0.0;
        gbc.ipady = 2;
        gbc.fill = GridBagConstraints.NONE;
        gbc.insets = new Insets (5,13,5,5);
        upPanel.add (_jtfComboBox, gbc);
        
        gbc = new GridBagConstraints();
        JButton jbBrowse = new JButton ("Browse...");
        jbBrowse.setBorder (BorderFactory.createBevelBorder (BevelBorder.RAISED));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,5,5,13);
        gbc.gridx = 1;
        gbc.gridy = 1;
        gbc.ipady = 2;
        gbc.gridheight = 2;
        gbc.weightx = 0.5;
        upPanel.add (jbBrowse, gbc);
        
        gbc = new GridBagConstraints();
        JLabel jLabel2 = new JLabel ("Relative Path of File To Update on Remote System");
        gbc.gridx = 0;
        gbc.gridy = 2;
        gbc.insets = new Insets (5,13,5,13);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        upPanel.add (jLabel2, gbc);
        
        _jtfRemoteRelativeFile = new JTextField();
        gbc.gridx = 0;
        gbc.gridy = 3;
        gbc.weightx = 0.1;
        //gbc.ipady = 2;
        Dimension dimTf = _jtfComboBox.getMinimumSize();
        int htf = (int) dimTf.getHeight();
        _jtfRemoteRelativeFile.setMaximumSize (new Dimension (290, htf));
        _jtfRemoteRelativeFile.setPreferredSize (new Dimension (290, htf));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,13,13,5);
        upPanel.add (_jtfRemoteRelativeFile, gbc);
        
        //Buttons Panel components
        gbc = new GridBagConstraints();
        JButton jbUpdate = new JButton ("Upload");
        jbUpdate.setBorder (BorderFactory.createBevelBorder (BevelBorder.RAISED));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.insets = new Insets (5,13,13,13);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.ipady = 1;
        gbc.weightx = 0.5;
        btnPanel.add (jbUpdate, gbc);
        
        JButton jbCancel = new JButton ("Cancel");
        jbCancel.setBorder (BorderFactory.createBevelBorder (BevelBorder.RAISED));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.insets = new Insets (5,13,13,13);
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.ipady = 1;
        gbc.weightx = 0.5;
        btnPanel.add (jbCancel, gbc);
        
        jbUpdate.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                updateFiles();
                _jtfComboBox.revalidate();                
            }
        });
        
        _jtfComboBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
        /*_jtfComboBox.addItemListener (new ItemListener() {
             public void itemStateChanged (ItemEvent evt) {*/               
                JComboBox src = (JComboBox) evt.getSource();
                int index = src.getSelectedIndex();
                _localFilePath = (String) src.getSelectedItem();
                _remoteRelativeFilePath = getDefaultFileName (_localFilePath);
                _jtfRemoteRelativeFile.setText (_remoteRelativeFilePath);
            }
        });
        
        jbCancel.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _localFilePath = null;
                _remoteRelativeFilePath = null;
                _update = false;
                dispose();
            }
        });
        
        jbBrowse.addActionListener(new ActionListener() {
            public void actionPerformed (ActionEvent evt) {
                browseDirectories();
            }
        });
        
        this.addWindowListener (new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                setVisible (false);
                dispose();
            }
        });
    }        
    
    protected void updateFiles()
    {
        //_localFilePath = _jtfLocalFile.getText();
        _localFilePath = (String) _jtfComboModel.getSelectedItem();
        //_remoteRelativeFilePath = _jtfRemoteRelativeFile.getText();
        if (_jtfRemoteRelativeFile.getText().length() == 0) {
            _remoteRelativeFilePath = getDefaultFileName (_localFilePath);
            _jtfRemoteRelativeFile.setText (_remoteRelativeFilePath);
            //_logger.info ("_jtfRemoteRelativeFile: " + _jtfRemoteRelativeFile);
        }
        else {
            _remoteRelativeFilePath = _jtfRemoteRelativeFile.getText();
            //_logger.info ("_jtfRemoteRelativeFile: " + _jtfRemoteRelativeFile);
        }

        if (_localFilePath == null) {
            //_logger.fine ("LocalFilePath: " + _localFilePath + " is null");
            JOptionPane.showMessageDialog (this, "Must select a local file", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        
        if (_localFilePath.length() == 0) {
            //_logger.fine ("LocalFilePath: " + _localFilePath + " is zero");
            JOptionPane.showMessageDialog (this, "Must select local file", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        else if (_remoteRelativeFilePath.length() == 0) {
            //_logger.fine ("RemoteRelativeFilePath: " + _remoteRelativeFilePath + " is zero");
            JOptionPane.showMessageDialog (this, "Must specify remote file", "Error", JOptionPane.ERROR_MESSAGE);
        }
        else {
            _update = true;
            dispose();
        }
    }
    
    protected void browseDirectories()
    {
        JFileChooser jfc = new JFileChooser();
        int result = jfc.showDialog (this, "Choose File");
        if (result == JFileChooser.APPROVE_OPTION) {
            File selectedFile = jfc.getSelectedFile();
            //_jtfLocalFile.setText (selectedFile.getAbsolutePath());
            //_jtfComboModel.addElement (selectedFile.getAbsolutePath());
            _jtfComboModel.insertElementAt (selectedFile.getAbsolutePath(), 0);
            _jtfComboBox.setSelectedIndex (0);
            if (_jtfRemoteRelativeFile.getText().length() == 0) {
                _jtfRemoteRelativeFile.setText (selectedFile.getName());
            }
        }
    }
    protected String getDefaultFileName (String path)
    {
        int index = path.lastIndexOf (FILE_BREAK);
        String fileName = path.substring (index+1);
        return fileName;
    }
    
    public boolean update()
    {
        return _update;
    }

    public String getLocalFilePath()
    {
        return _localFilePath;
    }
    
    public String getRemoteRelativePath()
    {
        return _remoteRelativeFilePath;
    }
    
    // Sets the frame location at the center of the screen
	public void setDlgLocation()	
	{
	    Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension d = tk.getScreenSize();
        Dimension dd = this.getPreferredSize();
        int width = ((d.width/3) - (dd.width/3)) + 80;
        int height = ((d.height/3) - (dd.height/3)) + 80;
        this.setLocation (width, height);
	}
    
    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) 
    {
        UpdateFile up = new UpdateFile();
        up.setVisible (true);
    }
    
    //Class Variables
    private boolean _update;
    private String _localFilePath;
    private String _remoteRelativeFilePath;
    protected String FILE_BREAK = System.getProperty ("file.separator");
    protected DefaultComboBoxModel _jtfComboModel;
    protected JComboBox _jtfComboBox;
    //protected JTextField _jtfLocalFile;
    protected JTextField _jtfRemoteRelativeFile;
    protected Vector _pathsVector;
    
    //protected Logger _logger;
}