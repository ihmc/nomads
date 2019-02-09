package us.ihmc.rms;

import java.io.File;

import java.awt.*;
import javax.swing.*;
import javax.swing.border.EtchedBorder;
import javax.swing.border.BevelBorder;
import javax.swing.filechooser.FileFilter;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;

/**
 * * Similar class to the update files but it only allows to get the
 *   local path that users want to download in the remote machines.
 *
 * @author  Maggie Breedy
 * @version $Revision$
 * $Date$
 * 
 */
public class DownloadDirectoryDialog extends JDialog
{
    public DownloadDirectoryDialog()
    {
        initComponents();
        _download = false;
    }
    
    protected void initComponents()
    {
        setModal (true);
		setTitle ("Download a Directory");
        setSize (500, 220);
		getContentPane().setLayout (new GridBagLayout());
		GridBagConstraints gbc = new GridBagConstraints();
		setVisible (false);
        
        // Add upper Panel
        JPanel upPanel = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.5;
        gbc.weighty = 0.5;
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
        JLabel jLabel1 = new JLabel ("Local Directory");
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (5,13,5,13);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        upPanel.add (jLabel1, gbc);
        
        _jtfLocalFile = new JTextField();
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 0.5;
        gbc.ipady = 3;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,13,13,5);
        upPanel.add (_jtfLocalFile, gbc);
        
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
        JLabel jLabel2 = new JLabel ("Remote Directory Path ");
        jLabel2.setHorizontalAlignment (SwingConstants.RIGHT);
        gbc.gridx = 0;
        gbc.gridy = 2;
        gbc.insets = new Insets (5,13,13,5);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        upPanel.add (jLabel2, gbc);
        
        _jtfRemoteDir = new JTextField();
        gbc.gridx = 1;
        gbc.gridy = 2;
        gbc.weightx = 0.5;
        gbc.ipady = 3;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,13);
        upPanel.add (_jtfRemoteDir, gbc);

        //Buttons Panel components
        gbc = new GridBagConstraints();
        JButton jbUpdate = new JButton ("Download");
        jbUpdate.setBorder (BorderFactory.createBevelBorder (BevelBorder.RAISED));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.insets = new Insets (5,13,13,13);
        gbc.gridx = 0;
        gbc.gridy = 0;
        //gbc.ipady = 5;
        gbc.weightx = 0.5;
        btnPanel.add (jbUpdate, gbc);
        
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
                
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-420)/2, (screenSize.height-224)/2, 420, 224);
        
        jbUpdate.addActionListener (new java.awt.event.ActionListener() {
            public void actionPerformed (java.awt.event.ActionEvent evt) {
                _localFilePath = _jtfLocalFile.getText();
                _remoteFilePath = _jtfRemoteDir.getText();
                if (_localFilePath.length() == 0) {
                    JOptionPane.showMessageDialog (new JFrame(), "Must select local file", "Error", JOptionPane.ERROR_MESSAGE);
                }
                else {
                    _download = true;
                    dispose();
                }
            }
        });
        
        jbCancel.setText("Cancel");
        jbCancel.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                //_localFilePath = null;
                //_remoteFilePath = null;
                _download = false;
               dispose();
            }
        });
        
        jbBrowse.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                String path = null;
                JFileChooser chooser = new JFileChooser();
                chooser.setFileSelectionMode (JFileChooser.DIRECTORIES_ONLY);
                int result = chooser.showDialog (new JFrame(), "Choose Directory");
                if (result == JFileChooser.APPROVE_OPTION) {
                    System.out.println ("..Opening... ");
                    path = chooser.getSelectedFile().getAbsolutePath();
                    System.out.println ("****Path: " + path);
                    if (path.length() == 0) {
                        JOptionPane.showMessageDialog (new JFrame(), path + " is not valid", "Error", JOptionPane.ERROR_MESSAGE);
                        return;
                    }
                    else {
                        System.out.println ("****Opening: " + path);
                        _jtfLocalFile.setText (path);
                    }
                    System.out.println ("Opening " + path + "....");
                }
            }
        });
    }
    
    public boolean download()
    {
        return _download;
    }

    public String getLocalFilePath()
    {
        System.out.println ("_localFilePath: " + _localFilePath);
        return _localFilePath;
    }
    
    public String getRemotePath()
    {
        System.out.println ("_remoteFilePath: " + _remoteFilePath);
        return _remoteFilePath;
    }
    
    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) 
    {
        DownloadDirectoryDialog ddd = new DownloadDirectoryDialog();
        ddd.setVisible (true);
    }
    
    /**
     * The RMS file filter
     */
    public class RMSFileFilter extends FileFilter
    {
        public RMSFileFilter() 
        {
        }
        
        public boolean accept(File f) 
        {
            return f.isDirectory();
        }
        
        public String getDescription() 
        {
            return "";
        }
    }
    
    //Variables
    private boolean _download = false;
    private String _localFilePath;
    private String _remoteFilePath;
    protected JTextField _jtfLocalFile;
    protected JTextField _jtfRemoteDir;
}