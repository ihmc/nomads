package us.ihmc.rms;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
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
import javax.swing.SwingConstants;

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
 * ProcessNameDialog allows to get the process name that whant to be killed.
 *
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 *
 **/

public class ProcessNameDialog extends JDialog
{
    public ProcessNameDialog()
    {
        setTitle ("Kill Process Dialog");
        setModal (true);
        setResizable (true);
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-350)/2, (screenSize.height-200)/2, 350, 200);
        getContentPane().setLayout (new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        
        JPanel contentPanel = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.weightx = 0.8;
        gbc.weighty = 0.8;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (contentPanel, gbc);
        
        JPanel bottonsPanel = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.weightx = 0.2;
        gbc.weighty = 0.2;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.insets = new Insets (0,0,0,0);
        getContentPane().add (bottonsPanel, gbc);

        // Create the Process Name Label
        gbc = new GridBagConstraints();
        JLabel l = new JLabel ("Process Name:");
        l.setHorizontalAlignment (SwingConstants.LEFT);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (5,13,13,13);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        contentPanel.add (l, gbc);
        
        _jtfProcName = new JTextField();
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 0.5;
        gbc.ipady = 3;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (13,13,5,13);
        contentPanel.add (_jtfProcName, gbc);
        
        // Create the OK Button
        gbc = new GridBagConstraints();
        JButton okButton = new JButton ("OK");
        okButton.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.5;
        gbc.weighty = 0.5;
        gbc.insets = new Insets (13,13,13,13);
        //gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        bottonsPanel.add (okButton, gbc);
 
        okButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _processName = _jtfProcName.getText();
                if (_processName == null) {
                    JOptionPane.showMessageDialog (new JFrame(), "Please write a process name", "Error", JOptionPane.ERROR_MESSAGE);
                }
                else {
                    setVisible (false);
                }
            }
        });
        
        // Create the Cancel Button
        JButton cancelButton = new JButton ("Cancel");
        cancelButton.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.weightx = 0.5;
        gbc.weighty = 0.5;
        //gbc.gridwidth = 2;
        gbc.insets = new Insets (13,13,13,13);
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        bottonsPanel.add (cancelButton, gbc);
        
        cancelButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _cancelled = true;
                setVisible (false);
                dispose();
            }
        });
        
        this.addWindowListener (new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                _windowIsClosing = true;
                setVisible (false);
                dispose();
            }
        });

        _cancelled = false;
    }

    public String getProcessName()
    {
        System.out.println ("->>Process Name is: " + _processName);
        return _processName;
    }

    public boolean wasCancelled()
    {
        return _cancelled;
    }
    
    public boolean dialogIsClosing()
    {
        return _windowIsClosing;
    }

    //Class Variables
    protected boolean _cancelled;
    protected boolean _windowIsClosing = false;
    protected JTextField _jtfProcName;
    protected String _processName;
}
