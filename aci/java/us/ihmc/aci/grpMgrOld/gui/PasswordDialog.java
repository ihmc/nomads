package us.ihmc.aci.grpMgrOld.gui;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JPasswordField;

/**
 * Password Dialog prompt for a password when join a Private group is 
 * requested
 *
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 *
 **/
public class PasswordDialog extends JDialog
{
	public PasswordDialog (JFrame owner)
	{
        super (owner, "Password Dialog", true);
        setResizable (true);
	    Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-570)/2, (screenSize.height-500)/2, 570, 500);
        
        JPanel contentPanel = new JPanel();
        contentPanel.setLayout (null);
       
        JLabel passwLabel = new JLabel ("Enter Password:");
        contentPanel.add(passwLabel);
		passwLabel.setBounds (12,24,96,24);
        
        _passwdField = new JPasswordField();
        _passwdField.setEchoChar ('*');
        contentPanel.add (_passwdField);
		_passwdField.setBounds (120,24,240,24);
        
        JButton okButton = new JButton();
        okButton.setText("OK");
		contentPanel.add(okButton);
		okButton.setBounds(72,72,108,24);
        
        JButton clearButton = new JButton();
		clearButton.setText("clear");
		contentPanel.add(clearButton);
		clearButton.setBounds(228,72,108,24);
        
        okButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                _passwdStr = new String (_passwdField.getPassword());
                setVisible (false);
            }
        });
        
        clearButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                _passwdField.setText ("");
            }
        });
        
        this.addWindowListener (new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                dispose();
            }
        });
        
         setContentPane (contentPanel);
         setSize (380, 150);
    }
    
    public String getPassword()
    {
        return (_passwdStr);
    }
	
    protected JPasswordField _passwdField;
    protected String _passwdStr = null;
}