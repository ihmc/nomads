package us.ihmc.rms;

import java.io.File;
import java.util.Enumeration;
import java.util.Vector;

import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import javax.swing.*;
import javax.swing.border.BevelBorder;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;

/**
 * Download a file from the remote site.
 * 
 * @author  Maggie Breedy
 * @version $Revision$
 * $Date$
 * 
 */
public class DownloadFileDialog extends JDialog 
{
    public DownloadFileDialog()
    {        
        _download = false;
    }
    
    public void init (Vector pathsVector)
    {
        _pathsVector = pathsVector;
        createGui();
    }
    
    protected void createGui()
    {
        setModal (true);
		setTitle ("Download File");
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
        JLabel jLabel1 = new JLabel ("Remote Absolute Path to Download the File");
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (5,13,5,13);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        upPanel.add (jLabel1, gbc);
        
        _jtfRemoteAbsolutePath = new JTextField();
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 0.0;
        gbc.ipady = 1;
        Dimension dimTf = _jtfRemoteAbsolutePath.getMinimumSize();
        int jtfh = (int) dimTf.getHeight();
        _jtfRemoteAbsolutePath.setMaximumSize (new Dimension (290, jtfh));
        _jtfRemoteAbsolutePath.setPreferredSize (new Dimension (290, jtfh));
        gbc.fill = GridBagConstraints.NONE;
        gbc.insets = new Insets (5,13,13,5);
        upPanel.add (_jtfRemoteAbsolutePath, gbc);
                
        gbc = new GridBagConstraints();
        JLabel jLabel2 = new JLabel ("Local File");
        gbc.gridx = 0;
        gbc.gridy = 2;
        gbc.insets = new Insets (5,13,5,13);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        upPanel.add (jLabel2, gbc);
        
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
        if (!_pathsVector.isEmpty()) {
           for (Enumeration e = _pathsVector.elements(); e.hasMoreElements();) {
                _jtfComboModel.addElement ((String) e.nextElement());
            } 
        }
        gbc.gridx = 0;
        gbc.gridy = 3;
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
        gbc.gridy = 3;
        gbc.ipady = 2;
        gbc.gridheight = 2;
        gbc.weightx = 0.5;
        upPanel.add (jbBrowse, gbc);
        
        //Buttons Panel components
        gbc = new GridBagConstraints();
        JButton jbDownload = new JButton ("Download");
        jbDownload.setBorder (BorderFactory.createBevelBorder (BevelBorder.RAISED));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.insets = new Insets (5,13,13,13);
        gbc.gridx = 0;
        gbc.gridy = 0;
        //gbc.ipady = 5;
        gbc.weightx = 0.5;
        btnPanel.add (jbDownload, gbc);
        
        JButton jbCancel = new JButton ("Cancel");
        jbCancel.setBorder (BorderFactory.createBevelBorder (BevelBorder.RAISED));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.insets = new Insets (5,13,13,13);
        gbc.gridx = 1;
        gbc.gridy = 0;
        //gbc.ipady = 5;
        gbc.weightx = 0.5;
        btnPanel.add (jbCancel, gbc);
        
        jbDownload.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                downloadFile();
            }
        });
        
        _jtfComboBox.addItemListener (new ItemListener() {
             public void itemStateChanged (ItemEvent e) {                
                JComboBox src = (JComboBox) e.getSource();
                int index = src.getSelectedIndex();
                _localFilePath = (String) src.getSelectedItem();
                //_jtfRemoteAbsolutePath.setText (_localFilePath);
            }
        });
        
        jbCancel.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _localFilePath = null;
                _remoteFilePath = null;
                _download = false;
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
    
    protected void downloadFile()
    {
        _localFilePath = (String) _jtfComboModel.getSelectedItem();
        _remoteFilePath = _jtfRemoteAbsolutePath.getText();
        /*if (_jtfRemoteAbsolutePath.getText().length() == 0) {
            //_remoteRelativeFilePath = getDefaultFileName (_localFilePath);
            //_jtfRemoteAbsolutePath.setText (_remoteRelativeFilePath);
            _jtfRemoteAbsolutePath.setText (_localFilePath);
        }
        else {
            //_remoteRelativeFilePath = _jtfRemoteAbsolutePath.getText();
            _remoteFilePath = _jtfRemoteAbsolutePath.getText();
            
        }*/
        if (_localFilePath == null) {
            JOptionPane.showMessageDialog (this, "Must select a local file", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        else if (_localFilePath.length() == 0) {
            JOptionPane.showMessageDialog (this, "Must select local file", "Error", JOptionPane.ERROR_MESSAGE);
        }
        else if (_remoteFilePath.length() == 0) {
            JOptionPane.showMessageDialog (this, "Must specify remote file", "Error", JOptionPane.ERROR_MESSAGE);
        }
        else {
            _download = true;
            dispose();
        }
    }
        
    protected void browseDirectories()
    {
        JFileChooser jfc = new JFileChooser();
        int result = jfc.showDialog (this, "Choose File");
        if (result == JFileChooser.APPROVE_OPTION) {
            File selectedFile = jfc.getSelectedFile();
            _jtfComboModel.insertElementAt (selectedFile.getAbsolutePath(), 0);
            _jtfComboBox.setSelectedIndex (0);
        }
    }
    protected String getDefaultFileName (String path)
    {
        int index = path.lastIndexOf (FILE_BREAK);
        //int index = path.lastIndexOf ("\\");
        String fileName = path.substring (index+1);
        //System.out.println ("-->filename: " + fileName);
        return fileName;
    }
    
   public boolean download()
    {
        return _download;
    }

    public String getLocalFilePath()
    {
        return _localFilePath;
    }
    
    public String getRemotePath()
    {
        return _remoteFilePath;
    }    
    
    // Sets the dialog location at the center of the screen
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
    public static void main (String args[]) 
    {
        DownloadFileDialog dl = new DownloadFileDialog();
        dl.setVisible (true);
    }
    
    //Class Variables
    private boolean _download;
    private String _localFilePath;
    private String _remoteFilePath;
    protected String FILE_BREAK = System.getProperty ("file.separator");
    protected DefaultComboBoxModel _jtfComboModel;
    protected JComboBox _jtfComboBox;
    //protected JTextField _jtfLocalFile;
    protected JTextField _jtfRemoteAbsolutePath;
    protected Vector _pathsVector;
}