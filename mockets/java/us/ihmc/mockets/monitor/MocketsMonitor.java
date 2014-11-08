/*
 * MocketsMonitor.java
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 * 
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 * 
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Date: 9/23/13
 * Time: 10:52 AM
 * @author  Maggie Breedy <mbreedy@ihmc.us>
 * $Revision: 1.10 $
 */

package us.ihmc.mockets.monitor;

import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import java.text.DecimalFormat;
import java.util.HashMap;
import java.util.Map;
import java.util.Vector;

public class MocketsMonitor extends JFrame implements MocketStatusListener
{
    class StatWrapper
    {
        StatWrapper()
        {
            this (null, null);
        }

        StatWrapper (MocketStatus.EndPointsInfo epi, MocketStatus.MocketStatisticsInfo msi)
        {
            _epi = epi;
            _msi = msi;
        }

        MocketStatus.EndPointsInfo _epi;
        MocketStatus.MocketStatisticsInfo _msi;
    }

    public MocketsMonitor()
    {
        createGUI();
        clearLabels();
        try {
            Monitor m = new Monitor();
            m.setListener (this);
            m.start();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        _statsByPeer = new HashMap<String, Map<String, StatWrapper>>();
        _statsDataMap = new HashMap<String, StatWrapper>();
    }

    protected void createGUI()
    {
        setTitle ("MocketsMonitor");
        setSize (800, 500);
        getContentPane().setLayout (new GridBagLayout());
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-800)/2, (screenSize.height-500)/2, 800, 500);

        //LeftPanel
        GridBagConstraints gbc = new GridBagConstraints();
        JPanel leftPanel = new JPanel (new GridBagLayout());
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.2;
        gbc.weighty = 0.7;
        gbc.insets = new Insets (0,20,0,20);
        leftPanel.setMinimumSize (new Dimension (100, 50));
        getContentPane().add (leftPanel, gbc);
        TitledBorder tb = new TitledBorder (new EtchedBorder(), " Process ID ",
                                  TitledBorder.LEFT, TitledBorder.TOP);
        tb.setTitleColor (new Color (48, 103, 84));
        leftPanel.setBorder (tb);

        //Right Panel
        JPanel rightPanel = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.7;
        gbc.weighty = 0.7;
        gbc.insets = new Insets (0,0,0,20);
        getContentPane().add (rightPanel, gbc);
        TitledBorder tb2 = new TitledBorder (new EtchedBorder(), " Connections ",
                                            TitledBorder.LEFT, TitledBorder.TOP);
        //rightPanel.setMaximumSize (new Dimension (600, 400)); //600,250
        rightPanel.setBorder (tb2);
        rightPanel.setMinimumSize (new Dimension (600, 200));
        tb2.setTitleColor (new Color (48, 103, 84));

        //Bottom Panel
        JPanel bottomPanel = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.5;
        gbc.weighty = 0.4;
        gbc.gridwidth = 2;
        gbc.gridheight = 1;
        getContentPane().add (bottomPanel, gbc);
        TitledBorder tb3 = new TitledBorder (new EtchedBorder(), " Statistics ",
                          TitledBorder.LEFT, TitledBorder.TOP);
        tb3.setTitleColor (new Color (48, 103, 84));
        bottomPanel.setBorder (tb3);
        //bottomPanel.setBackground (new Color (239,248,251));

        _pidList = new JList();
        _pidListModel = new DefaultListModel();
        _pidList.setModel (_pidListModel);
        _pidList.setLayoutOrientation (JList.HORIZONTAL_WRAP);

        JScrollPane sp = new JScrollPane (_pidList);
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHEAST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
        //sp.setMinimumSize (new Dimension (100, 50));
        leftPanel.add (sp, gbc);

        _pidList.addListSelectionListener (new ListSelectionListener() {
            public void valueChanged (ListSelectionEvent e) {
                if (!e.getValueIsAdjusting()) {
                    JList list = (JList) e.getSource();
                    int selIndex =  list.getSelectedIndex();
                    if (selIndex < 0) {
                        System.out.println ("Selection error");
                    }
                    else {
                        String pid = list.getSelectedValue().toString();
                        _currentSelection = pid;
                        EventQueue.invokeLater (new Runnable() {
                            public void run() {
                                _connListModel.removeAllElements();
                                System.out.println ("******_currentSelection: " + _currentSelection);
                                updatePID (_currentSelection);
                            }
                        });

//                        _connListModel.removeAllElements();
//                        System.out.println ("******_currentSelection: " + _currentSelection);
////                        _connList.setCellRenderer (new SelectedListCellRenderer());
//                        updatePID (pid);
                    }
                }
             }
         });


        //Right panel components
        _connList = new JList();
        _connListModel = new DefaultListModel();
        _connList.setModel (_connListModel);

        JScrollPane sp2 = new JScrollPane (_connList);

        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHEAST;
        gbc.weightx = 0.7;
        gbc.weighty = 0.7;
        //sp2.setMinimumSize (new Dimension (600, 500));
        rightPanel.add (sp2, gbc);

        _connList.addListSelectionListener (new ListSelectionListener() {
            public void valueChanged (ListSelectionEvent e) {
                if (!e.getValueIsAdjusting()) {
                    JList list = (JList) e.getSource();
                    int selIndex =  list.getSelectedIndex();
                    if (selIndex < 0) {
                       System.out.println ("Selection error");
                    }
                    else {
                        String conn = list.getSelectedValue().toString();
                        //System.out.println ("connection " + conn);
                        updateConnections (conn);
                        _connectionSelected = conn;
                    }
                }
            }
        });

        //Bottom Panel components
        JPanel panel1 = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
        bottomPanel.add(panel1, gbc);
        //panel1.setBackground (new Color (239,248,251));

        //Panel1
        JLabel contTimeLabel = new JLabel ("Last Contact Time (sec): ");
        contTimeLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        panel1.add (contTimeLabel,gbc);

        jfLastContact = new JLabel (" ");
        jfLastContact.setHorizontalAlignment (JLabel.LEFT);
        jfLastContact.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 0;
        //gbc.weightx = 0.1;
        panel1.add (jfLastContact,gbc);

        JLabel rttLabel = new JLabel ("Estimated RTT: ");
        rttLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 2;
        gbc.gridy = 0;
        panel1.add (rttLabel,gbc);

        jfRTT = new JLabel (" ");
        jfRTT.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfRTT.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 3;
        gbc.gridy = 0;
        panel1.add (jfRTT,gbc);

        //Panel2
        JPanel panel2 = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.3;
        bottomPanel.add (panel2, gbc);
        panel2.setBorder (new EtchedBorder());
        //panel2.setBackground (new Color (239,248,251));

        JPanel panelPS = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.weightx = 0.3;
        panel2.add (panelPS, gbc);

        JPanel panelPR = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.3;
        panel2.add (panelPR, gbc);

        JPanel panelPD = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 2;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.WEST;
        gbc.weightx = 0.3;
        panel2.add (panelPD, gbc);

        JLabel sentLabel = new JLabel ("Packets Sent ");
        sentLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        panelPS.add (sentLabel,gbc);

        JLabel sentByLabel = new JLabel ("Bytes: ");
        sentByLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        sentByLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 1;
        panelPS.add (sentByLabel,gbc);

        jfSentBytes = new JLabel (" ");
        jfSentBytes.setHorizontalAlignment (JLabel.LEFT);
        jfSentBytes.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 1;
        panelPS.add (jfSentBytes,gbc);

        JLabel numpackLabel = new JLabel ("Number of Packets: ");
        numpackLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        numpackLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 2;
        panelPS.add (numpackLabel,gbc);

        jfSentPackets = new JLabel (" ");
        jfSentPackets.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfSentPackets.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 2;
        panelPS.add (jfSentPackets,gbc);

        JLabel retrLabel = new JLabel ("Retransmissions: ");
        retrLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        retrLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 3;
        panelPS.add (retrLabel,gbc);

        jfRetr = new JLabel (" ");
        jfRetr.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfRetr.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 3;
        panelPS.add (jfRetr,gbc);

        JLabel recLabel = new JLabel ("Packets Received ");
        recLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        panelPR.add (recLabel,gbc);

        JLabel byLabel = new JLabel ("Bytes: ");
        byLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        byLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 0;
        gbc.gridy = 1;
        panelPR.add (byLabel,gbc);

        jfRecBytes = new JLabel (" ");
        jfRecBytes.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfRecBytes.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 1;
        panelPR.add (jfRecBytes,gbc);

        JLabel packLabel = new JLabel ("Number of Packets: ");
        packLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        packLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 0;
        gbc.gridy = 2;
        panelPR.add (packLabel,gbc);

        jfRecPackets = new JLabel (" ");
        jfRecPackets.setHorizontalAlignment (JLabel.LEFT);
        jfRecPackets.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 2;
        panelPR.add (jfRecPackets,gbc);

        JLabel discLabel = new JLabel ("Packets Discarded ");
        discLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,20,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        panelPD.add (discLabel,gbc);

        JLabel dupLabel = new JLabel ("Duplicated: ");
        dupLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        dupLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,20,5,5);
        gbc.gridx = 0;
        gbc.gridy = 1;
        panelPD.add (dupLabel,gbc);

        jfDiscDup = new JLabel (" ");
        jfDiscDup.setHorizontalAlignment (JLabel.LEFT);
        jfDiscDup.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 1;
        panelPD.add (jfDiscDup,gbc);

        JLabel nroomLabel = new JLabel ("No Room: ");
        nroomLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        nroomLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,20,5,5);
        gbc.gridx = 0;
        gbc.gridy = 2;
        panelPD.add (nroomLabel,gbc);

        jfDiscNoRoom = new JLabel (" ");
        jfDiscNoRoom.setHorizontalAlignment (JLabel.LEFT);
        jfDiscNoRoom.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 2;
        panelPD.add (jfDiscNoRoom,gbc);

        JLabel reasLabel = new JLabel ("Reassembly Skipped: ");
        reasLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        reasLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,20,5,5);
        gbc.gridx = 0;
        gbc.gridy = 3;
        panelPD.add (reasLabel,gbc);

        jfDiscReassSkipped = new JLabel (" ");
        jfDiscReassSkipped.setHorizontalAlignment (JLabel.LEFT);
        jfDiscReassSkipped.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 3;
        panelPD.add (jfDiscReassSkipped,gbc);

        //Panel3
        JPanel panel3 = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 2;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
        bottomPanel.add (panel3, gbc);
        panel3.setBorder (new EtchedBorder());
        //panel3.setBackground (new Color (239,248,251));

        JPanel panel3PQ = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.weightx = 0.3;
        panel3.add (panel3PQ, gbc);

        JPanel panel3URS = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 0.3;
        panel3.add (panel3URS, gbc);

        JPanel panel3URUS = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 2;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.WEST;
        gbc.weightx = 0.3;
        panel3.add (panel3URUS, gbc);

        JLabel pendLabel = new JLabel ("Pending Packet Queue ");
        pendLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        panel3PQ.add (pendLabel,gbc);

        JLabel dataSizeLabel = new JLabel ("Data Size: ");
        dataSizeLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        dataSizeLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 1;
        panel3PQ.add (dataSizeLabel,gbc);

        jfPPQDataSize = new JLabel ("0");
        jfPPQDataSize.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfPPQDataSize.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 1;
        panel3PQ.add (jfPPQDataSize,gbc);

        JLabel queueSizeLabel = new JLabel ("Queue Size: ");
        queueSizeLabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        queueSizeLabel.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 2;
        panel3PQ.add (queueSizeLabel,gbc);

        jfPPQSize = new JLabel ("0");
        jfPPQSize.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfPPQSize.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 2;
        panel3PQ.add (jfPPQSize,gbc);

        JLabel ursqLabel = new JLabel ("Unacknowledged Reliable Sequenced Queue ");
        ursqLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        panel3URS.add (ursqLabel,gbc);

        JLabel dataSizeLabel2 = new JLabel ("Data Size: ");
        dataSizeLabel2.setFont (new Font ("Frame", Font.PLAIN, 12));
        dataSizeLabel2.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 1;
        panel3URS.add (dataSizeLabel2,gbc);

        jfRelSeqDataSize = new JLabel (" ");
        jfRelSeqDataSize.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfRelSeqDataSize.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 1;
        //gbc.weightx = 0.1;
        panel3URS.add (jfRelSeqDataSize,gbc);

        JLabel queueSizeLabel2 = new JLabel ("Queue Size: ");
        queueSizeLabel2.setFont (new Font ("Frame", Font.PLAIN, 12));
        queueSizeLabel2.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 2;
        //gbc.weightx = 0.1;
        panel3URS.add (queueSizeLabel2,gbc);

        jfRelSeqSize = new JLabel ("0");
        jfRelSeqSize.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfRelSeqSize.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 2;
        panel3URS.add (jfRelSeqSize,gbc);


        JLabel uruqLabel = new JLabel ("Unacknowledged Reliable UnSequenced Queue ");
        uruqLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        panel3URUS.add (uruqLabel,gbc);

        JLabel dataSizeLabel3 = new JLabel ("Data Size: ");
        dataSizeLabel3.setFont (new Font ("Frame", Font.PLAIN, 12));
        dataSizeLabel3.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 1;
        //gbc.weightx = 0.1;
        panel3URUS.add (dataSizeLabel3,gbc);

        jfRelUnseqDataSize = new JLabel ("0");
        jfRelUnseqDataSize.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfRelUnseqDataSize.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 1;
        panel3URUS.add (jfRelUnseqDataSize,gbc);

        JLabel queueSizeLabel3 = new JLabel ("Queue Size: ");
        queueSizeLabel3.setFont (new Font ("Frame", Font.PLAIN, 12));
        queueSizeLabel3.setForeground (new Color (37, 84, 199));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = 2;
        panel3URUS.add (queueSizeLabel3,gbc);

        jfRelUnseqSize = new JLabel ("0");
        jfRelUnseqSize.setFont (new Font ("Frame", Font.PLAIN, 12));
        jfRelUnseqSize.setHorizontalAlignment (JLabel.LEFT);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets (5,0,5,5);
        gbc.gridx = 1;
        gbc.gridy = 2;
        panel3URUS.add (jfRelUnseqSize,gbc);
        jfRelUnseqSize.setText("200");


        this.addWindowListener (new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                dispose();
                System.exit (1);
            }
        });
    }

    protected String getNumberFormated (Long value)
    {
        DecimalFormat df = new DecimalFormat("###,###,###");
        //System.out.print (df.format (value));

        return df.format (value);
    }

    protected void clearLabels()
    {
        //_pidListModel.removeAllElements();
        jfLastContact.setText("");
        jfRTT.setText("");
        jfSentBytes.setText("");
        jfSentPackets.setText("");
        jfRetr.setText("");
        jfRecBytes.setText("");
        jfRecPackets.setText("");
        jfDiscDup.setText("");
        jfDiscNoRoom.setText("");
        jfDiscReassSkipped.setText("");
        jfPPQDataSize.setText("");
        jfPPQSize.setText("");
        jfRelSeqDataSize.setText("");
        jfRelSeqSize.setText("");
        jfRelUnseqDataSize.setText("");
        jfRelUnseqSize.setText("");
    }


    protected void updatePID (String pid)
    {
        Map<String, StatWrapper> map = _statsByPeer.get (pid);
        for (String str : map.keySet()) {
            System.out.println ("******connList str: " + str);
            if (!_connListModel.contains (str)) {
                _connListModel.addElement (str);
            }
            else {
                if (str.equals (_connectionSelected)) {
                    updateConnections (str);
                }
            }
        }
    }

    protected void updateConnections (String connection)
    {
        for (String key : _statsDataMap.keySet()) {
            if (key.equalsIgnoreCase (connection)) {
                System.out.println ("******connection: " + connection);
                StatWrapper wr = _statsDataMap.get (connection);
                setStatisticsInfoUpdateArrived (wr._msi);
            }
        }
//        for (String key : _statsByPeer.keySet()) {
//            if (key.equalsIgnoreCase (_currentSelection)) {
//                System.out.println ("******connList key: " + key);
//                Map<String, StatWrapper> map = _statsByPeer.get (key);
//                for (String name : map.keySet()) {
//                    System.out.println ("******connList name: " + name);
//                    if (name.equalsIgnoreCase (connection)) {
//                        StatWrapper wr = map.get (connection);
//                        setStatisticsInfoUpdateArrived (wr._msi);
//                    }
//                }
//            }
//        }
    }

    public void statusUpdateArrived (MocketStatus.EndPointsInfo epi, byte msgType)
    {
        System.out.println ("Status Update <" + MocketStatus.MocketStatusNoticeType.values()[msgType] +
                            "> received from " + epi.PID + " " + epi.identifier);
    }

    protected void setStatisticsInfoUpdateArrived (MocketStatus.MocketStatisticsInfo msi)
    {
        //MocketStatus.MocketStatisticsInfo msi = sw._msi;
        jfLastContact.setText (Long.toString (System.currentTimeMillis() - msi.lastContactTime));
        System.out.println ("****Current time:<" + System.currentTimeMillis() + ">");
        System.out.println ("****last time:<" + msi.lastContactTime + ">");
        //jfSentBytes.setText (Long.toString (msi.sentBytes));
        jfSentBytes.setText (getNumberFormated  (msi.sentBytes));

        jfSentPackets.setText (Long.toString (msi.sentPackets));
        jfRetr.setText (Long.toString (msi.retransmits));    //removed +")"
        //jfRecBytes.setText (Long.toString (msi.receivedBytes));
        jfRecBytes.setText (getNumberFormated (msi.receivedBytes));

        jfRecPackets.setText (Long.toString (msi.receivedPackets));
        jfDiscDup.setText (Long.toString (msi.duplicatedDiscardedPackets));
        jfDiscNoRoom.setText (Long.toString (msi.noRoomDiscardedPackets));
        jfDiscReassSkipped.setText (Long.toString (msi.reassemblySkippedDiscardedPackets));
        jfRTT.setText (Float.toString (msi.estimatedRTT));
        jfPPQDataSize.setText (Long.toString (msi.pendingDataSize));
        jfPPQSize.setText (Long.toString (msi.pendingPacketQueueSize));
        jfRelSeqDataSize.setText (Long.toString (msi.reliableSequencedDataSize));
        jfRelSeqSize.setText (Long.toString (msi.reliableSequencedPacketQueueSize));
        jfRelUnseqDataSize.setText (Long.toString (msi.reliableUnsequencedDataSize));
        jfRelUnseqSize.setText (Long.toString (msi.reliableUnsequencedPacketQueueSize));
        _connList.revalidate();
    }

    public void statisticsInfoUpdateArrived (MocketStatus.EndPointsInfo epi, MocketStatus.MocketStatisticsInfo msi)
    {
        //System.out.println ("****Status Update: id: " + epi.PID + " identif: " + epi.identifier);
        String pid = String.valueOf (epi.PID);
        StringBuffer sb = new StringBuffer();
        sb.append ("Identifier: " + epi.identifier + "  ");
        //System.out.println ("****Identifier:<" + epi.identifier + ">");
        sb.append ("LocalPort: " + Long.toString (epi.localPort) + "  ");
        //System.out.println ("****localPort:<" + epi.localPort + ">");
        sb.append ("RemoteAddress: " + epi.remoteAddress.toString().replace ('/', '\0') + "  ");
        //System.out.println ("****RemoteAddress:<" + epi.remoteAddress.toString().replace ('/', '\0') + ">");
        sb.append ("RemotePort: " + Long.toString (epi.remotePort) + "  ");
        String dataEP = sb.toString();

        System.out.println ("****Status Update: dataEP: " + dataEP);
        StatWrapper wr = new StatWrapper();
        wr._epi = epi;
        wr._msi = msi;
        //Map<String, StatWrapper> statsDataMap = new HashMap<String, StatWrapper>();
//        if (_statsDataMap.containsKey (dataEP)) {
//            _statsDataMap.remove (dataEP);
//        }

        _statsDataMap.put (dataEP, wr);
        _statsByPeer.put (pid, _statsDataMap);

        if (!_pidListModel.contains (pid)) {
            _pidListModel.addElement (pid);
        }

        if (_currentSelection != null && String.valueOf (epi.PID).compareToIgnoreCase(_currentSelection) == 0) {
           EventQueue.invokeLater (new Runnable() {
               public void run() {
                   updatePID (_currentSelection);
               }
           });
        }
    }

    public void statisticsInfoUpdateArrived (MocketStatus.EndPointsInfo epi, MocketStatus.MessageStatisticsInfo messSi)
    {
    }

    public static void main (String[] args) throws Exception
    {
        EventQueue.invokeLater (new Runnable() {
            public void run() {
                new MocketsMonitor().setVisible (true);
            }
        });
    }

    public class SelectedListCellRenderer extends DefaultListCellRenderer
    {
        public Component getListCellRendererComponent(JList list, Object value, int index, boolean isSelected, boolean cellHasFocus)
        {
            Component c = super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
            if (isSelected) {
                c.setBackground (Color.lightGray);
            }
            return c;
        }
    }

    /**
     * Table Model for the mockets connections
     */
     private class ConnectionsTableModel extends DefaultTableModel
     {
         public ConnectionsTableModel (Vector data, Vector columnNames)
         {
           super (data, columnNames);
         }

         // Get the data from each row of the table.
         public Vector<String> getRowData (int row)
         {
           Vector connVector = new Vector();
           String identifier = (String) getValueAt(row, 0);
           connVector.addElement (identifier);
           String localPort = (String) getValueAt(row, 1);
           connVector.addElement (localPort);
           String remoteAddress = (String) getValueAt(row, 2);
           connVector.addElement (remoteAddress);
           String remotePort = (String) getValueAt(row, 3);
           connVector.addElement (remotePort);

           return connVector;
         }

         public Vector<String> getKeyElements (Vector<String> rowData )
         {
           Vector keyConnVector = new Vector();
           keyConnVector.addElement (rowData.elementAt(0));
           keyConnVector.addElement (rowData.elementAt(1));
           keyConnVector.addElement (rowData.elementAt(2));

           return keyConnVector;
         }
     }


    //Variables
    private DefaultListModel _pidListModel;
    private JList _pidList;

    private DefaultListModel _connListModel;
    private JList _connList;

    private Map<String, Map<String, StatWrapper>> _statsByPeer; //PID -> a statsDataMap
    private Map<String, StatWrapper> _statsDataMap; //PID ->statWrapper

    protected Vector<Object> _connectionsVector;

//    protected JLabel jfIdentifier;
//    protected JLabel jfLocalPort;
//    protected JLabel jfRemoteAddress;
//    protected JLabel jfRemotePort;
    protected JLabel jfDiscDup;
    protected JLabel jfDiscNoRoom;
    protected JLabel jfDiscReassSkipped;
    protected JLabel jfLastContact;
    protected JLabel jfPPQDataSize;
    protected JLabel jfPPQSize;
    protected JLabel jfRTT;
    protected JLabel jfRecBytes;
    protected JLabel jfRecPackets;
    protected JLabel jfRelSeqDataSize;
    protected JLabel jfRelSeqSize;
    protected JLabel jfRelUnseqDataSize;
    protected JLabel jfRelUnseqSize;
    protected JLabel jfRetr;
    protected JLabel jfSentBytes;
    protected JLabel jfSentPackets;

    private String _currentSelection = "";
    private String _connectionSelected = "";
    private long _stream[];
    private int _numStreams;
    private MocketStatus.Update _mostRecUpdate[];
    private long _selectedStream;
    private static int MAX_NUM_STREAMS = 15;

}
