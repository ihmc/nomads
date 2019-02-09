package us.ihmc.jeeves.adminTool;

import java.io.*;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.*;
import java.awt.Font;
import java.awt.Insets;
import java.awt.Toolkit;
import javax.swing.*;
import javax.swing.text.Document;
import javax.swing.border.BevelBorder;

/**
 * File editor dialog for Jeeves
 *
 * @author  Maggie Breedy
 * @version $Revision$
 *
 **/

public class JeevesFileEditor extends JDialog
{
    public JeevesFileEditor (File filename)
    {
        _filename = filename;
        createFrame();
        _editor.setContentType ("text/plain");
        Document doc = _editor.getEditorKit().createDefaultDocument();
        _editor.setDocument (doc);
        try {
            InputStream is = new BufferedInputStream (new FileInputStream (filename));
            _editor.getEditorKit().read (is, doc, 0);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        _editor.setEditable (true);
    }
    
    protected void createFrame()
    {
        setModal (true);
        setTitle ("Jeeves File Editor");
        setResizable (true);
        getContentPane().setLayout (new BorderLayout());
        setSize (570, 500);
        setFont (new Font("Dialog", Font.BOLD, 12));
        //setVisible(false);
        
        //Main Panel
        JPanel mainPanel = new JPanel (new GridBagLayout());
        getContentPane().add (mainPanel);
        
        // Create Panel components
        GridBagConstraints gbc = new GridBagConstraints();
        JPanel upPanel = new JPanel (new GridBagLayout());
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.weightx = 0.95;
        gbc.weighty = 0.95;
        mainPanel.add (upPanel, gbc);
        
        JPanel lowPanel = new JPanel (new GridBagLayout());
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.weightx = 0.05;
        gbc.weighty = 0.05;
        mainPanel.add (lowPanel, gbc);
        
        // Up Panel component
        _editor = new JEditorPane();
        JScrollPane sp = new JScrollPane (_editor);
        sp.setVerticalScrollBar (new JScrollBar(JScrollBar.VERTICAL));
        //_editor.setEditable (true);
        gbc = new GridBagConstraints();
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        upPanel.add (sp, gbc);
        
        // Low Panel components
        JButton commitButton = new JButton ("Commit Changes");
        commitButton.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        gbc = new GridBagConstraints();
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets (0,40,5,10);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.5;
        gbc.ipady = 5;
        lowPanel.add (commitButton, gbc);
        
        JButton cancelButton = new JButton ("Cancel");
        cancelButton.setBorder (BorderFactory.createBevelBorder(BevelBorder.RAISED));
        gbc = new GridBagConstraints();
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets (0,5,5,40);
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.weightx = 0.5;
        gbc.ipady = 5;
        lowPanel.add (cancelButton, gbc);
        
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-570)/2, (screenSize.height-500)/2, 570, 500);
        
        /**
         * Commit the changes, and save the file again.
         */
        commitButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                try {
                    Writer w = new FileWriter (_filename);
                    _editor.write (w);                    
                    _success = true;
                    //System.out.println ("-->>The file was saved: " + _filename);
                    w.close();
                    setVisible (false);
                }
                catch (Exception ex) {
                    _success = false;
                    ex.printStackTrace();
                }
            }
        });
        
        /**
         * Cancel the operation         
         */
        cancelButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                setVisible (false);
                dispose();
            }
        });
        
        this.addWindowListener(new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                setVisible (false);
            }
        });
    }
    
    public boolean isSuccessful()
    {
        return _success;
    }
    
    //Class Variables
    protected boolean _success = false;
    protected JEditorPane _editor;
    protected File _filename;
}