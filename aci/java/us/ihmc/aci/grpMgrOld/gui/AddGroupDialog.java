package us.ihmc.aci.grpMgrOld.gui;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import javax.swing.event.CaretEvent;
import javax.swing.event.CaretListener;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

public class AddGroupDialog extends JDialog
{
    public AddGroupDialog (JFrame owner)
    {
        super (owner, "Add Group", true);
        setResizable (true);

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
        /*_groupNameField.addCaretListener (new CaretListener() {
            public void caretUpdate (CaretEvent e)
            {           
            }
        });*/

        // Create the Restricted Group Check Box
        _restrictedGroupCheckBox = new JCheckBox ("Restricted Group");
        _restrictedGroupCheckBox.setLocation (16, 56);
        _restrictedGroupCheckBox.setSize (160, 24);
        contentPanel.add (_restrictedGroupCheckBox);
        _restrictedGroupCheckBox.addChangeListener (new ChangeListener() {
            public void stateChanged (ChangeEvent e)
            {
                handleRestrictedGroupCheckBox();
            }
        });

        // Create the Password Label
        _passwordLabel = new JLabel ("Password to Join:");
        _passwordLabel.setLocation (16, 88); //144
        _passwordLabel.setSize (144, 24);
        _passwordLabel.setEnabled (false);
        contentPanel.add (_passwordLabel);

        // Create the Password Field
        _passwordField = new JTextField();
        _passwordField.setLocation (144, 88);
        _passwordField.setSize (184, 24);
        _passwordField.setEnabled (false);
        contentPanel.add (_passwordField);

        // Create the OK Button
        JButton b = new JButton ("OK");
        b.setLocation (46, 130); //200
        b.setSize (100, 36);
        b.setDefaultCapable (true);
        contentPanel.add (b);
        b.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                handleOKButton();
            }
        });

        // Create the Cancel Button
        b = new JButton ("Cancel");
        b.setLocation (198, 130); //200
        b.setSize (100, 36);
        contentPanel.add (b);
        b.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                handleCancelButton();
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

    public void setRestrictedGroup (boolean flag)
    {
        _restrictedGroupCheckBox.setSelected (flag);
    }
    
    public boolean isRestrictedGroup()
    {
        return _restrictedGroupCheckBox.isSelected();
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
        return _passwordField.getText();
    }

    public boolean wasCancelled()
    {
        return _cancelled;
    }
    
    private void handleRestrictedGroupCheckBox()
    {
        if (_restrictedGroupCheckBox.isSelected()) {
            _fqgnLabel.setEnabled (true);
            _passwordLabel.setEnabled (true);
            _passwordField.setEnabled (true);
        }
        else {
            _fqgnLabel.setEnabled (false);
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

    private boolean _cancelled;
    private JCheckBox _restrictedGroupCheckBox;
    private JLabel _fqgnLabel;
    private JLabel _passwordLabel;
    private JTextField _groupNameField;
    private JTextField _fqgnField;
    private JTextField _passwordField;
    private String _qualifier;
}
