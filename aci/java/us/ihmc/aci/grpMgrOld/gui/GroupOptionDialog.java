package us.ihmc.aci.grpMgrOld.gui;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Toolkit;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import javax.swing.BorderFactory;
import javax.swing.border.BevelBorder;
import javax.swing.border.EtchedBorder;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JTextField;

import javax.swing.event.CaretEvent;
import javax.swing.event.CaretListener;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

/**
 * GroupOptionDialog allows to get the Group Name and then if the group is Private
 * to get the password for the Create Group Button. 
 *
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 *
 **/

public class GroupOptionDialog extends JDialog
{
    public GroupOptionDialog (JFrame owner, String dialogName)
    {
        super (owner, dialogName, true);
        setModal (true);
        setResizable (true);
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-570)/2, (screenSize.height-500)/2, 570, 500);

        JPanel contentPanel = new JPanel();
        contentPanel.setLayout (null);

        // Create the Group Name Label
        JLabel l = new JLabel ("Group Name:");
        l.setLocation (16, 16);
        l.setSize (144, 24);
        contentPanel.add (l);

        // Create the Group Name Field
        _groupNameField = new JTextField();
        _groupNameField.setLocation (144, 16);
        _groupNameField.setSize (184, 24);
        contentPanel.add (_groupNameField);
        _groupNameField.addCaretListener (new CaretListener() {
            public void caretUpdate (CaretEvent e)
            {           
            }
        });
        
        _groupNameField.addKeyListener (new KeyAdapter() {
            public void keyPressed (KeyEvent event) 
            {
                if (event.getKeyCode() == KeyEvent.VK_ENTER) {
                    if (isPrivateGroup()) {
                        String password = new String (_passwordField.getPassword());
                        if ((password.equals ("")) || (password == null)){
                            JOptionPane.showMessageDialog (new JFrame(), "Please Enter a Password", "Invalid Password",
                                                           JOptionPane.ERROR_MESSAGE);
                            return;
                        }
                    }
                    handleOKButton();
                }
            }
        });

        // Create the Private Group Check Box
        _privateGroupCheckBox = new JCheckBox ("Private Group");
        _privateGroupCheckBox.setBorder (new EtchedBorder());
        _privateGroupCheckBox.setLocation (16, 56);
        _privateGroupCheckBox.setSize (120, 24);
        contentPanel.add (_privateGroupCheckBox);
        _privateGroupCheckBox.addChangeListener (new ChangeListener() {
            public void stateChanged (ChangeEvent e)
            {
                handlePrivateGroupCheckBox();
            }
        });
        
        // Create the Peer Group Check Box
        _peerGroupCheckBox = new JCheckBox ("Peer Group");
        _privateGroupCheckBox.setBorder (new EtchedBorder());
        _peerGroupCheckBox.setLocation (164, 56);
        _peerGroupCheckBox.setSize (120, 24);
        contentPanel.add (_peerGroupCheckBox);
        _peerGroupCheckBox.addChangeListener (new ChangeListener() {
            public void stateChanged (ChangeEvent e)
            {
                _groupNameField.requestFocus();
                handlePeerGroupCheckBox();
            }
        });

        // Create the Password Label
        _passwordLabel = new JLabel ("Password to Join:");
        _passwordLabel.setLocation (16, 88);
        _passwordLabel.setSize (144, 24);
        _passwordLabel.setEnabled (false);
        contentPanel.add (_passwordLabel);

        // Create the Password Field
        _passwordField = new JPasswordField();
        _passwordField.setText ("");
        _passwordField.setEchoChar ('*');
        _passwordField.setLocation (144, 88);
        _passwordField.setSize (184, 24);
        _passwordField.setEnabled (false);
        contentPanel.add (_passwordField);
        
        _passwordField.addKeyListener (new KeyAdapter() {
            public void keyPressed (KeyEvent event) 
            {
                if (event.getKeyCode() == KeyEvent.VK_ENTER) {
                    if (isPrivateGroup()) {
                        String password = new String (_passwordField.getPassword());
                        if ((password.equals ("")) || (password == null)){
                            JOptionPane.showMessageDialog (new JFrame(), "Please Enter a Password", "Invalid Password",
                                                           JOptionPane.ERROR_MESSAGE);
                            return;
                        }
                    }
                    handleOKButton();
                }
            }
        });

        // Create the OK Button
        JButton b = new JButton ("OK");
        b.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        b.setLocation (46, 130);
        b.setSize (100, 28);
        b.setDefaultCapable (true);
        contentPanel.add (b);
        b.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                if (isPrivateGroup()) {
                    String password = new String (_passwordField.getPassword());
                    if ((password.equals ("")) || (password == null)){
                        JOptionPane.showMessageDialog (new JFrame(), "Please Enter a Password", "Invalid Password",
                                                       JOptionPane.ERROR_MESSAGE);
                        return;
                    }
                }
                handleOKButton();
            }
        });
        
        // Create the Cancel Button
        JButton c = new JButton ("Cancel");
        c.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        c.setLocation (198, 130);
        c.setSize (100, 28);
        contentPanel.add (c);
        c.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
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
        setSize (352, 210);
        
        _cancelled = false;
    }

    public void setGroupName (String name)
    {
        _groupNameField.setText (name);
    }

    public String getGroupName()
    {
        return _groupNameField.getText();
    }

    public void setPrivateGroup (boolean flag)
    {
        _privateGroupCheckBox.setSelected (flag);
    }
    
    public boolean isPrivateGroup()
    {
        return _privateGroupCheckBox.isSelected();
    }
    
    public boolean isPeerGroup()
    {
        return _peerGroupCheckBox.isSelected();
    }
    
    public void setGroupNameQualifier (String qualifier)
    {
        _qualifier = qualifier;
    }
    
    public String getGroupNameQualifier()
    {
        return _qualifier;
    }

    public void setPassword (String password)
    {
        _passwordField.setText (password);
    }
    
    public String getPassword()
    {
        String password = new String (_passwordField.getPassword());
        return (password);
    }

    public boolean wasCancelled()
    {
        return _cancelled;
    }
    
    public boolean dialogIsClosing()
    {
        return _windowIsClosing;
    }
    
    private void handlePrivateGroupCheckBox()
    {
        if (_privateGroupCheckBox.isSelected()) {
            _passwordLabel.setEnabled (true);
            _passwordField.setEnabled (true);
            _passwordField.requestFocus();
        }
        else {
            _passwordLabel.setEnabled (false);
            _passwordField.setEnabled (false);
        }
    }
    
    
    private void handlePeerGroupCheckBox()
    {
        if (_peerGroupCheckBox.isSelected() && (_privateGroupCheckBox.isSelected())) {
            _passwordLabel.setEnabled (true);
            _passwordField.setEnabled (true);
            _passwordField.requestFocus();
        }
        else {
            _passwordLabel.setEnabled (false);
            _passwordField.setEnabled (false);
        }
    }
    
    private void handleOKButton()
    {       
        dispose();
    }
    
    private void handleCancelButton()
    {
        _cancelled = true;
        dispose();
    }

    //Class Variables
    private boolean _cancelled;
    private boolean _windowIsClosing = false;
    private JTextField _groupNameField;
    private JCheckBox _privateGroupCheckBox;
    private JCheckBox _peerGroupCheckBox;
    private JTextField _fqgnField;
    private JLabel _passwordLabel;
    private JPasswordField _passwordField;        
    private String _qualifier;
}
