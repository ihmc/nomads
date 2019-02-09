package us.ihmc.jeeves.adminTool;

import java.awt.Dimension;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.Insets;
import java.awt.event.*;
import java.awt.Toolkit;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.SwingConstants;

/**
 * AddPlatform dialog for adding platforms.
 *
 * @author  Maggie Breedy
 * @version $Revision$
 * $Date$
 * 
 */


public class AddPlatform extends JDialog 
{
    /** Creates new form AddPlatform */
    public AddPlatform() 
    {
        initComponents();
        _update = false;
    }
    
    protected void initComponents()
    {
        setModal (true);
        setTitle ("Add New Platform");
        setSize (400, 150);
        getContentPane().setLayout (new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        
        // Add upper Panel
        JPanel upPanel = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.5;
        gbc.weighty = 0.5;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        getContentPane().add (upPanel, gbc);
        
        gbc = new GridBagConstraints();
        JLabel jLabel1 = new JLabel ("Enter IP Address or Hostname:", SwingConstants.RIGHT);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (5,13,5,5);
        gbc.anchor = GridBagConstraints.EAST;
        upPanel.add (jLabel1,gbc);
        jLabel1.setLabelFor (_hostTextField);
        
        _hostTextField = new JTextField();
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.weightx = 1.0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets (5,5,5,13);
        upPanel.add (_hostTextField, gbc);
        
        gbc = new GridBagConstraints();
        JLabel jLabel2 = new JLabel ("Enter Port Number:", SwingConstants.RIGHT);
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.insets = new Insets (5,13,5,5);
        gbc.anchor = GridBagConstraints.EAST;
        upPanel.add (jLabel2, gbc);
        jLabel2.setLabelFor (_portTextField);
        
        _portTextField = new JTextField();
        gbc.gridx = 1;
        gbc.gridy = 1;
        gbc.weightx = 1.0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets (5,5,5,13);
        upPanel.add (_portTextField, gbc);
        
        //Add buttons Panel
        gbc = new GridBagConstraints();
        JPanel bPanel = new JPanel();
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 0.2;
        gbc.weighty = 0.2;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.CENTER;
        getContentPane().add (bPanel, gbc);
        
        _btnAdd = new JButton ("Add");
        bPanel.add (_btnAdd, gbc);
        _btnAdd.setMnemonic ('a');
        _btnAdd.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                if (_hostTextField.getText().length() > 0) {
                    _update = true;
                    dispose();
                }
                else {
                    JOptionPane.showMessageDialog (new JFrame(), "Must Enter a Value", "Error", JOptionPane.ERROR_MESSAGE);
                }
            }
        });
        
        JButton btnClear = new JButton ("Clear");
        bPanel.add (btnClear, gbc);
        btnClear.setMnemonic ('c');
        btnClear.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _hostTextField.setText ("");
                _portTextField.setText ("");
                _hostTextField.requestFocus (true);
            }
         });
        
        _hostTextField.addKeyListener(new java.awt.event.KeyAdapter() {
            public void keyTyped(java.awt.event.KeyEvent evt) {
                if (evt.getKeyChar() == evt.VK_ENTER) {
                    _btnAdd.doClick();
                }
            }
        });
        
        _portTextField.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                if (_portTextField.getText().length() > 0) {
                    _update = true;
                    dispose();
                }
                else {                    
                    JOptionPane.showMessageDialog (new JFrame(), "Must Enter a Port", "Error", JOptionPane.ERROR_MESSAGE);
                }
            }
        });
        
        this.addWindowListener(new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                dispose();
                setVisible (false);
            }
        });
        
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds((screenSize.width-428)/2, (screenSize.height-172)/2, 428, 172);
    }
    
    public boolean update()
    {
        return _update;
    }
    
    public String getValue()
    {
        return _hostTextField.getText();
    }
    
    public String getPort()
    {
        return _portTextField.getText();
    }

    /**
     * @param args the command line arguments
     */
    public static void main (String args[]) {
        AddPlatform ap = new AddPlatform ();
        ap.setVisible (true);
    }
    
    //Variables
    private boolean _update;
    protected JButton _btnAdd;
    protected JTextField _hostTextField;
    protected JTextField _portTextField;
}