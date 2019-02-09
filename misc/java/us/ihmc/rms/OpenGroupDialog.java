package us.ihmc.rms;

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
 * OpenGroupDialog allows to get the Group Name and the password 
 * for creating a private managed group. 
 *
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 *
 **/

public class OpenGroupDialog extends JDialog
{
    public OpenGroupDialog()
    {
        setModal (true);
        setResizable (true);
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-450)/2, (screenSize.height-200)/2, 450, 200);

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
        _groupNameField.requestFocus();
        //_groupNameField.setText ("RMSPrivate");
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
                    String password = new String (_passwordField.getPassword());
                    if ((password.equals ("")) || (password == null)){
                        JOptionPane.showMessageDialog (new JFrame(), "Please Enter a Password", "Invalid Password",
                                                       JOptionPane.ERROR_MESSAGE);
                        return;
                    }
                    handleOKButton();
                }
            }
        });

        // Create the Password Label
        _passwordLabel = new JLabel ("Password:");
        _passwordLabel.setLocation (16, 56);
        _passwordLabel.setSize (144, 24);
        _passwordLabel.setEnabled (true);
        contentPanel.add (_passwordLabel);

        // Create the Password Field
        _passwordField = new JPasswordField();
        _passwordField.setText ("");
        _passwordField.setEchoChar ('*');
        _passwordField.setLocation (144, 56);
        _passwordField.setSize (184, 24);
        _passwordField.setEnabled (true);
        //_passwordField.setText ("nomads");
        contentPanel.add (_passwordField);
        
        _passwordField.addKeyListener (new KeyAdapter() {
            public void keyPressed (KeyEvent event) 
            {
                if (event.getKeyCode() == KeyEvent.VK_ENTER) {
                    String password = new String (_passwordField.getPassword());
                    if ((password.equals ("")) || (password == null)){
                        JOptionPane.showMessageDialog (new JFrame(), "Please Enter a Password", "Invalid Password",
                                                       JOptionPane.ERROR_MESSAGE);
                        return;
                    }
                    handleOKButton();
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
            public void actionPerformed (ActionEvent e)
            {
                String password = new String (_passwordField.getPassword());
                if ((password.equals ("")) || (password == null)){
                    JOptionPane.showMessageDialog (new JFrame(), "Please Enter a Password", "Invalid Password",
                                                   JOptionPane.ERROR_MESSAGE);
                    return;
                }
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
        setSize (352, 180);
        
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
    private JTextField _fqgnField;
    private JLabel _passwordLabel;
    private JPasswordField _passwordField;        
}
