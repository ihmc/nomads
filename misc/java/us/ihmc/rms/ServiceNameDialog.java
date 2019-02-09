package us.ihmc.rms;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Toolkit;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import javax.swing.BorderFactory;
import javax.swing.border.BevelBorder;
import javax.swing.border.EtchedBorder;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;

import javax.swing.event.CaretEvent;
import javax.swing.event.CaretListener;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.util.Enumeration;
import java.util.Vector;

/**
 * ServiceNameDialog allows to get the Service Name.
 *
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 *
 **/

public class ServiceNameDialog extends JDialog
{
    public ServiceNameDialog()
    {
        setTitle ("Get Service Name Dialog");
        setModal (true);
        setResizable (true);
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-450)/2, (screenSize.height-200)/2, 450, 200);
        
        Vector _namesVector = null;
        ServiceNameDialog snd = null;
        _filename = new File (System.getProperty ("user.dir") + FILE_BREAK + "serviceName.txt");
        //System.out.println ("-->startService:_filename: " + _filename);
        if (_filename.exists()) {            
            _namesVector = loadFromFile();
            //System.out.println ("-->_namesVectorFrom File!!: " + _namesVector);
        }

        JPanel contentPanel = new JPanel();
        contentPanel.setLayout (null);

        // Create the Group Name Label
        JLabel l = new JLabel ("Service Name:");
        l.setLocation (16, 16);
        l.setSize (144, 24);
        contentPanel.add (l);
        
        _servComboModel = new DefaultComboBoxModel();
        _servComboBox = new JComboBox (_servComboModel);
        _servComboBox.setEditable (true);
        
        Dimension dim = _servComboBox.getMinimumSize();
        int w = (int) dim.getWidth();
        int h = (int) dim.getHeight();
        _servComboBox.setMaximumSize (new Dimension (290, h));
        _servComboBox.setPreferredSize (new Dimension (290, h));
        //System.out.println ("-->_namesVector: " + _namesVector);
        if ((_namesVector == null) || (_namesVector.isEmpty())) {
        	_servComboModel.addElement ("Edit Service Name");
        }
        else {
            for (Enumeration e = _namesVector.elements(); e.hasMoreElements();) {
                _servComboModel.addElement ((String) e.nextElement());
            }
        }
        
        _servComboBox.setLocation (144, 16);
        _servComboBox.setSize (184, 24);
        contentPanel.add (_servComboBox);
  
        _servComboBox.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                JComboBox src = (JComboBox) e.getSource();
               int index = src.getSelectedIndex();
               _serviceName = (String) src.getSelectedItem();
               if (_serviceName == null) {
                   JOptionPane.showMessageDialog (new JFrame(), "Please Enter a Service Name", "Invalid Service Name",
                                                  JOptionPane.ERROR_MESSAGE);
                   return;
               }
            }
        });
        
        // Create the OK Button
        JButton okButton = new JButton ("OK");
        okButton.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        okButton.setLocation (46, 100);
        okButton.setSize (100, 28);
        okButton.setDefaultCapable (true);
        contentPanel.add (okButton);
        okButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                handleOKButton();
            }
        });
        
        // Create the Cancel Button
        JButton cancelButton = new JButton ("Cancel");
        cancelButton.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        cancelButton.setLocation (198, 100);
        cancelButton.setSize (100, 28);
        contentPanel.add (cancelButton);
        cancelButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                handleCancelButton();
            }
        });
        
        this.addWindowListener (new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                _windowIsClosing = true;
                dispose();
            }
        });

        setContentPane (contentPanel);
        setSize (352, 180);
        
        _cancelled = false;
        
        //Has to be done at the end so it gets selected before repainted.
        _servComboBox.setSelectedIndex (0);
    }

    public void setServiceName (String name)
    {
    	_servComboModel.insertElementAt (name, 0);
    	_servComboBox.setSelectedIndex (0);
    }

    public String getServiceName()
    {
        return _serviceName;
    }

    public boolean wasCancelled()
    {
        return _cancelled;
    }
    
    public boolean dialogIsClosing()
    {
        return _windowIsClosing;
    }
    
    protected void handleOKButton()
    {
        if (_namesVector == null) {
            _namesVector = new Vector();            
            _namesVector.addElement (_serviceName);
        }
        else {
            _namesVector.removeAllElements();
            _namesVector.addElement (_serviceName);
        }
        boolean isSaved = saveToAFile();
        //System.out.println ("-->startService:isSaved: " + isSaved);
        if (!isSaved) {
            System.out.println ("Unable to save service name to a file");
        }
        setVisible (false);
        dispose();
    }
    
    protected void handleCancelButton()
    {
        _cancelled = true;
        dispose();
    }
    
    protected boolean saveToAFile()
    {
        boolean success = false;
        try {
            if (!_filename.exists()) {
                _filename.createNewFile();
            }
            if (_filename.length() < 0) {
                JOptionPane.showMessageDialog (this, _filename + " is not valid", "Error", JOptionPane.ERROR_MESSAGE);
                return false;
            }
            //System.out.println ("Saving file to: " + _filename.getAbsolutePath());
            FileOutputStream fos = new FileOutputStream (_filename);
            ObjectOutputStream oos = new ObjectOutputStream (fos);
            if (_namesVector.isEmpty()) {
                System.out.println ("<<Error:No Service Names>>");
            }
            else {
                System.out.println ("Saving to file _namesVector: " + _namesVector);
                oos.writeObject (_namesVector);
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
    
    protected Vector loadFromFile()
    {
        ObjectInputStream ois = null;
        Vector vec = null;
        if (_filename.length() <= 0) {
            vec = null;
        }
        else {
            try {
                try {
                    ois = new ObjectInputStream (new FileInputStream (_filename));
                    vec = (Vector) ois.readObject();
                    for (Enumeration en = vec.elements(); en.hasMoreElements();) {
                        String str = (String) en.nextElement();
                        //System.out.println ("->Service: " + str);
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

    //Class Variables
    protected boolean _cancelled;
    protected boolean _windowIsClosing = false;
    protected File _filename;
    protected JComboBox _servComboBox;
    protected DefaultComboBoxModel _servComboModel;
    protected String _serviceName;
    protected String FILE_BREAK = System.getProperty ("file.separator");
    protected Vector _namesVector;
}
