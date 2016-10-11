/*
 * MonitorConsole.java
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 */

package us.ihmc.aci.disService.monitor;

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import java.util.HashMap;
import java.util.Map;
import java.util.Vector;

/**
 * Monitor Console GUI
 * Date: 5/21/13
 * @author  Maggie Breedy <mbreedy@ihmc.us>
 * $Revision: 1.10 $
 *
 */
public final class MonitorConsole extends JFrame implements DisServiceStatusListener
{
    class StatWrapper
    {
        StatWrapper()
        {
            this (null, null, null, null);
        }

        StatWrapper (DisServiceStatus.DisServiceBasicStatisticsInfo dsbsi,
                     DisServiceStatus.DisServiceStatsInfo dssi,
                     DisServiceStatus.DisServiceDuplicateTrafficInfo dsdti,
                     DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsibp)
        {
            _dsbsi = dsbsi;
            _dssi = dssi;
            _dsdti = dsdti;
            _dsbsibps = new HashMap<String, DisServiceStatus.DisServiceBasicStatisticsInfoByPeer>();
        }

        DisServiceStatus.DisServiceBasicStatisticsInfo _dsbsi;
        DisServiceStatus.DisServiceStatsInfo _dssi;
        DisServiceStatus.DisServiceDuplicateTrafficInfo _dsdti;
        Map<String, DisServiceStatus.DisServiceBasicStatisticsInfoByPeer> _dsbsibps;
    }

    public MonitorConsole (int udpPort)
    {
        initComponents();
        try {
            DisServiceMonitor m = new DisServiceMonitor (udpPort);
            m.setListener (this);
            m.start();
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit (-1);
        }
        _statsByPeer = new HashMap<String, StatWrapper>();
    }

    public void basicStatisticsInfoUpdateArrived (DisServiceStatus.DisServiceBasicStatisticsInfo dsbsi)
    {
        StatWrapper wr = _statsByPeer.get (dsbsi.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put (dsbsi.peerId, wr);
        }
        wr._dsbsi = dsbsi;
 
        if (!_peerListModel.contains (dsbsi.peerId)) {
            _peerListModel.addElement (dsbsi.peerId);
        }
        if (_currentSelection != null && dsbsi.peerId.compareToIgnoreCase(_currentSelection) == 0) {
            EventQueue.invokeLater (new Runnable() {
                public void run() {
                    updateForPeer(_currentSelection);
                }
            });
        }
    }

    public void overallStatsInfoUpdateArrived (DisServiceStatus.DisServiceStatsInfo dssi)
    {
        StatWrapper wr = _statsByPeer.get (dssi.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put (dssi.peerId, wr);
        }
        wr._dssi = dssi;

        if (!_peerListModel.contains (dssi.peerId)) {
            _peerListModel.addElement (dssi.peerId);
        }
        if (_currentSelection != null && dssi.peerId.compareToIgnoreCase(_currentSelection) == 0) {
            EventQueue.invokeLater (new Runnable() {
                public void run() {
                    updateForPeer(_currentSelection);
                }
            });
        }
    }

    public void duplicateTrafficStatisticInfoUpdateArrived (DisServiceStatus.DisServiceDuplicateTrafficInfo dsdti)
    {
        StatWrapper wr = _statsByPeer.get (dsdti.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put (dsdti.peerId, wr);
        }
        wr._dsdti = dsdti;

        if (!_peerListModel.contains (dsdti.peerId)) {
            _peerListModel.addElement (dsdti.peerId);
        }
        if (_currentSelection != null && dsdti.peerId.compareToIgnoreCase(_currentSelection) == 0) {
            EventQueue.invokeLater (new Runnable() {
                public void run() {
                    updateForPeer(_currentSelection);
                }
            });
        }
    }

    public void peerGroupStatisticInfoUpdateArrived (DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsip)
    {
        StatWrapper wr = _statsByPeer.get (dsbsip.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put (dsbsip.peerId, wr);
        }
        wr._dsbsibps.put (dsbsip.remotePeerId, dsbsip);

        if (!_peerListModel.contains (dsbsip.peerId)) {
            _peerListModel.addElement (dsbsip.peerId);
        }
        if (_currentSelection != null && dsbsip.peerId.compareToIgnoreCase(_currentSelection) == 0) {
            EventQueue.invokeLater (new Runnable() {
                public void run() {
                    updateForPeer(_currentSelection);
                }
            });
        }
    }

    private void initComponents()
    {
        JLabel jLabel1;
        JLabel jLabel11;
        JLabel jLabel13;
        JLabel jLabel15;
        JLabel jLabel17;
        JLabel jLabel18;
        JLabel jLabel19;
        JLabel jLabel2;
        JLabel jLabel21;
        JLabel jLabel22;
        JLabel jLabel3;
        JLabel jLabel4;
        JLabel jLabel5;
        JLabel jLabel6;
        JLabel jLabel7;
        JLabel jLabel9;
        JLabel jLabelDT;
        JLabel jLabelDT2;

        setTitle ("DisService Statistics Monitor");
        setSize (800, 600);
        //setMinimumSize (new Dimension (800, 600));
        getContentPane().setLayout (new GridBagLayout());
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-800)/2, (screenSize.height-600)/2, 800, 600);

        //Main Panels
        GridBagConstraints gbc = new GridBagConstraints();
        JPanel leftTopPanel = new JPanel (new GridBagLayout());
        GroupLayout jPanel2Layout = new GroupLayout (leftTopPanel);
        leftTopPanel.setLayout(jPanel2Layout);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHEAST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
        getContentPane().add (leftTopPanel, gbc);
        leftTopPanel.setBorder(BorderFactory.createTitledBorder ("Keep Alive Message KAM"));

        JPanel leftCenterPanel = new JPanel();
        GroupLayout jPanel1Layout = new GroupLayout (leftCenterPanel);
        leftCenterPanel.setLayout(jPanel1Layout);
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHEAST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
        getContentPane().add (leftCenterPanel, gbc);
        leftCenterPanel.setBorder(BorderFactory.createTitledBorder ("Missing Fragments Request MFR"));
        //leftCenterPanel.setBackground (new Color (198,216,226));

        JPanel leftBtmPanel = new JPanel();
        GroupLayout jPanel5Layout = new GroupLayout (leftBtmPanel);
        leftBtmPanel.setLayout(jPanel5Layout);
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 2;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.EAST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
        getContentPane().add (leftBtmPanel, gbc);
        leftBtmPanel.setBorder(BorderFactory.createTitledBorder ("Duplicate Traffic"));

        JPanel rightCenterPanel = new JPanel();
        GroupLayout jPanel3Layout = new GroupLayout(rightCenterPanel);
        rightCenterPanel.setLayout (jPanel3Layout);
        gbc = new GridBagConstraints();
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
        gbc.gridheight = 3;
        gbc.gridwidth = 1;
        getContentPane().add (rightCenterPanel, gbc);
        rightCenterPanel.setBorder(BorderFactory.createTitledBorder ("All Clients, All Groups, All Tags"));
        //rightCenterPanel.setBackground (new Color (198,216,226));

        JPanel rightPanel = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 2;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 0.3;
        gbc.weighty = 0.1;
        gbc.gridheight = 3;
        gbc.gridwidth = 1;
        getContentPane().add (rightPanel, gbc);
        rightPanel.setBorder(BorderFactory.createTitledBorder ("List of Peers"));

        JPanel bottomPanel = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 3;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        gbc.gridheight = 1;
        gbc.gridwidth = 3;
        getContentPane().add (bottomPanel, gbc);
        bottomPanel.setBorder(BorderFactory.createTitledBorder ("Detail by Peer"));
        //bottomPanel.setBackground (new Color (198,216,226));

        //LeftTopPanel
        jLabel2 = new JLabel();
        jfKeepAliveMessagesSent = new JLabel();
        jLabel2.setText("Sent:");
        jfKeepAliveMessagesSent.setText("0");
        jLabel6 = new JLabel();
        jfKeepAliveMessagesReceived = new JLabel();
        jLabel6.setText("Received:");
        jfKeepAliveMessagesReceived.setText("0");

        jPanel2Layout.setHorizontalGroup(
            jPanel2Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel2Layout.createSequentialGroup()
            .addContainerGap()
            .addGroup(jPanel2Layout.createParallelGroup(GroupLayout.Alignment.LEADING, false)
                    .addGroup(jPanel2Layout.createSequentialGroup()
                            .addComponent(jLabel2)
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED, GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                            .addComponent(jfKeepAliveMessagesSent, GroupLayout.PREFERRED_SIZE, 80, GroupLayout.PREFERRED_SIZE))
                    .addGroup(jPanel2Layout.createSequentialGroup()
                            .addComponent(jLabel6)
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                            .addComponent(jfKeepAliveMessagesReceived, GroupLayout.PREFERRED_SIZE, 80, GroupLayout.PREFERRED_SIZE)))
            .addContainerGap(50, Short.MAX_VALUE))
        );

        jPanel2Layout.setVerticalGroup(
            jPanel2Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel2Layout.createSequentialGroup()
                    .addGroup(jPanel2Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                            .addComponent(jLabel2)
                            .addComponent(jfKeepAliveMessagesSent))
                    .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                    .addGroup(jPanel2Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                            .addComponent(jLabel6)
                            .addComponent(jfKeepAliveMessagesReceived))
                    .addContainerGap(24, Short.MAX_VALUE))
        );

        //LeftCenterPanel
        jLabel1 = new JLabel();
        jLabel1.setText("Messages Sent:");
        jfMissingFragmentRequestMessagesSent = new JLabel();
        jfMissingFragmentRequestMessagesSent.setText("0");
        jLabel3 = new JLabel();
        jfMissingFragmentRequestBytesSent = new JLabel();
        jLabel3.setText("Bytes Sent:");
        jfMissingFragmentRequestBytesSent.setText("0");
        jLabel5 = new JLabel();
        jfMissingFragmentRequestMessagesReceived = new JLabel();
        jLabel5.setText("Messages Received:");
        jfMissingFragmentRequestMessagesReceived.setText("0");
        jLabel7 = new JLabel();
        jfMissingFragmentRequestBytesReceived = new JLabel();
        jLabel7.setText("Bytes Received:");
        jfMissingFragmentRequestBytesReceived.setText("0");

        jPanel1Layout.setHorizontalGroup (
            jPanel1Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel1Layout.createSequentialGroup()
            .addContainerGap()
            .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel1Layout.createSequentialGroup()
                    .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
                            .addComponent(jLabel1)
                            .addComponent(jLabel3))
                    .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                    .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.LEADING, false)
                            .addComponent(jfMissingFragmentRequestBytesSent, GroupLayout.DEFAULT_SIZE, GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                            .addComponent(jfMissingFragmentRequestMessagesSent, GroupLayout.DEFAULT_SIZE, 80, Short.MAX_VALUE)))
            .addGroup(jPanel1Layout.createSequentialGroup()
                    .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
                            .addComponent(jLabel5)
                            .addComponent(jLabel7))
                    .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                    .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.LEADING, false)
                            .addComponent(jfMissingFragmentRequestBytesReceived, GroupLayout.DEFAULT_SIZE, GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                            .addComponent(jfMissingFragmentRequestMessagesReceived, GroupLayout.DEFAULT_SIZE, 80, Short.MAX_VALUE)))))
        );

        jPanel1Layout.setVerticalGroup (
            jPanel1Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel1Layout.createSequentialGroup()
            .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel1)
                    .addComponent(jfMissingFragmentRequestMessagesSent))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel3)
                    .addComponent(jfMissingFragmentRequestBytesSent))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel5)
                    .addComponent(jfMissingFragmentRequestMessagesReceived))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel1Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel7)
                    .addComponent(jfMissingFragmentRequestBytesReceived))
            .addContainerGap(GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
        );

        //LeftBottomPanel
        jLabelDT = new JLabel();
        jfDTNoTarget = new JLabel();
        jLabelDT.setText("DT Received no Target:");
        jfDTNoTarget.setText("0");
        jLabelDT2 = new JLabel();
        jfDTTargeted = new JLabel();
        jLabelDT2.setText("DT Received Targeted to a Node:");
        jfDTTargeted.setText("0");

        jPanel5Layout.setHorizontalGroup (
            jPanel5Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel5Layout.createSequentialGroup()
            .addContainerGap()
            .addGroup(jPanel5Layout.createParallelGroup(GroupLayout.Alignment.LEADING, false)
                    .addGroup(jPanel5Layout.createSequentialGroup()
                            .addComponent(jLabelDT)
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED, GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                            .addComponent(jfDTNoTarget, GroupLayout.DEFAULT_SIZE, 80, GroupLayout.DEFAULT_SIZE))
                    .addGroup(jPanel5Layout.createSequentialGroup()
                            .addComponent(jLabelDT2)
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                            .addComponent(jfDTTargeted, GroupLayout.DEFAULT_SIZE, 80, GroupLayout.DEFAULT_SIZE)))
            .addContainerGap(GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
        );

        jPanel5Layout.setVerticalGroup (
            jPanel5Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel5Layout.createSequentialGroup()
                    .addGroup(jPanel5Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                            .addComponent(jLabelDT)
                            .addComponent(jfDTNoTarget))
                    .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                    .addGroup(jPanel5Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                            .addComponent(jLabelDT2)
                            .addComponent(jfDTTargeted))
                    .addContainerGap(GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
        );

        //RightPanel
        JList peerList = new JList();
        _peerListModel = new DefaultListModel();
        peerList.setModel (_peerListModel);
        _peerListModel.addElement (ALL_PEER_LABEL);
        //peerList.setSelectedIndex (0);

        JScrollPane jlsp = new JScrollPane (peerList);
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTHEAST;
        gbc.weightx = 0.4;
        gbc.weighty = 0.6;
        rightPanel.add (jlsp, gbc);

        peerList.addListSelectionListener (new ListSelectionListener() {
            public void valueChanged (ListSelectionEvent e) {
                if (!e.getValueIsAdjusting()) {
                    JList list = (JList) e.getSource();
                    int selIndex =  list.getSelectedIndex();
                    if (selIndex < 0) {
                        System.out.println ("Selection error");
                    }
                    else {
                        String peerName = list.getSelectedValue().toString();
                        if (peerName.equals (ALL_PEER_LABEL)) {
                            System.out.println (ALL_PEER_LABEL);
                            updateOverall();
                            _currentSelection = ALL_PEER_LABEL;
                        }
                        else {
                            System.out.println ("peerName: " + peerName);
                            updateForPeer (peerName);
                            _currentSelection = peerName;
                        }
                    }
                }
             }
         });

        //RightCenterPanel
        jLabel4 = new JLabel();
        jfOverallClientMessagesPushed = new JLabel();
        jLabel4.setText("Messages Pushed:");
        jfOverallClientMessagesPushed.setText("0");
        jLabel9 = new JLabel();
        jfOverallClientBytesPushed = new JLabel();
        jLabel9.setText("Bytes Pushed:");
        jfOverallClientBytesPushed.setText("0");
        jLabel11 = new JLabel();
        jfOverallFragmentsPushed = new JLabel();
        jLabel11.setText("Fragments Pushed:");
        jfOverallFragmentsPushed.setText("0");
        jLabel13 = new JLabel();
        jfOverallFragmentBytesPushed = new JLabel();
        jLabel13.setText("Fragment Bytes Pushed:");
        jfOverallFragmentBytesPushed.setText("0");
        jLabel15 = new JLabel();
        jfOverallOnDemandFragmentsSent = new JLabel();
        jLabel15.setText("On-Demand Fragments Sent:");
        jfOverallOnDemandFragmentsSent.setText("0");
        jLabel17 = new JLabel();
        jfOverallOnDemandFragmentBytesSent = new JLabel();
        jLabel17.setText("On-Demand Fragment Bytes Sent:");
        jfOverallOnDemandFragmentBytesSent.setText("0");
        jLabel18 = new JLabel();
        jLabel18.setText("Messages Received:");
        jfOverallMessagesReceived = new JLabel();
        jfOverallMessagesReceived.setText("0");
        jLabel19 = new JLabel();
        jLabel19.setText("Bytes Received:");
        jLabel21 = new JLabel();
        jLabel21.setText("Fragments Received:");
        jLabel22 = new JLabel();
        jLabel22.setText("Fragment Bytes Received:");
        jfOverallBytesReceived = new JLabel();
        jfOverallBytesReceived.setText("0");
        jfOverallFragmentBytesReceived = new JLabel();
        jfOverallFragmentBytesReceived.setText("0");
        jfOverallFragmentsReceived = new JLabel();
        jfOverallFragmentsReceived.setText("0");

        jPanel3Layout.setHorizontalGroup (
            jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel3Layout.createSequentialGroup()
            .addContainerGap()
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
                    .addGroup(jPanel3Layout.createSequentialGroup()
                            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
                                    .addComponent(jLabel17)
                                    .addComponent(jLabel15))
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING, false)
                                    .addComponent(jfOverallOnDemandFragmentsSent, GroupLayout.DEFAULT_SIZE, GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                                    .addComponent(jfOverallOnDemandFragmentBytesSent, GroupLayout.DEFAULT_SIZE, 80, Short.MAX_VALUE)))
                    .addGroup(jPanel3Layout.createSequentialGroup()
                            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
                                    .addComponent(jLabel13)
                                    .addComponent(jLabel11))
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING, false)
                                    .addComponent(jfOverallFragmentsPushed, GroupLayout.DEFAULT_SIZE, GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                                    .addComponent(jfOverallFragmentBytesPushed, GroupLayout.DEFAULT_SIZE, 80, Short.MAX_VALUE)))
                    .addGroup(jPanel3Layout.createSequentialGroup()
                            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
                                    .addComponent(jLabel4)
                                    .addComponent(jLabel9))
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING, false)
                                    .addComponent(jfOverallClientBytesPushed, GroupLayout.DEFAULT_SIZE, GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                                    .addComponent(jfOverallClientMessagesPushed, GroupLayout.DEFAULT_SIZE, 80, Short.MAX_VALUE)))
                    .addGroup(jPanel3Layout.createSequentialGroup()
                            .addComponent(jLabel18)
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                            .addComponent(jfOverallMessagesReceived, GroupLayout.DEFAULT_SIZE, 176, Short.MAX_VALUE))
                    .addGroup(jPanel3Layout.createSequentialGroup()
                            .addComponent(jLabel19)
                            .addGap(26, 26, 26)
                            .addComponent(jfOverallBytesReceived, GroupLayout.DEFAULT_SIZE, 176, Short.MAX_VALUE))
                    .addGroup(jPanel3Layout.createSequentialGroup()
                            .addComponent(jLabel21)
                            .addGap(31, 31, 31)
                            .addComponent(jfOverallFragmentsReceived, GroupLayout.DEFAULT_SIZE, 147, Short.MAX_VALUE))
                    .addGroup(jPanel3Layout.createSequentialGroup()
                            .addComponent(jLabel22)
                            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
                            .addComponent(jfOverallFragmentBytesReceived, GroupLayout.DEFAULT_SIZE, 147, Short.MAX_VALUE)))
            .addContainerGap())
        );

        jPanel3Layout.setVerticalGroup(
            jPanel3Layout.createParallelGroup(GroupLayout.Alignment.LEADING)
            .addGroup(jPanel3Layout.createSequentialGroup()
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
            .addComponent(jLabel4)
            .addComponent(jfOverallClientMessagesPushed))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel9)
                    .addComponent(jfOverallClientBytesPushed))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel11)
                    .addComponent(jfOverallFragmentsPushed))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel13)
                    .addComponent(jfOverallFragmentBytesPushed))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel15)
                    .addComponent(jfOverallOnDemandFragmentsSent))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel17)
                    .addComponent(jfOverallOnDemandFragmentBytesSent))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel18)
                    .addComponent(jfOverallMessagesReceived))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel19)
                    .addComponent(jfOverallBytesReceived))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel21)
                    .addComponent(jfOverallFragmentsReceived))
            .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
            .addGroup(jPanel3Layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
                    .addComponent(jLabel22)
                    .addComponent(jfOverallFragmentBytesReceived))
            .addContainerGap(21, Short.MAX_VALUE))
        );

        //Bottom Panel
        Vector columnNames = new Vector (8);
        columnNames.addElement ("Peer/Group");
        columnNames.addElement ("KAMRec");
        columnNames.addElement ("MFRMsgRec");
        columnNames.addElement ("MFRBytRec");
        columnNames.addElement ("TotMsgRec");
        columnNames.addElement ("TotBytRec");
        columnNames.addElement ("TotFragRec");
        columnNames.addElement ("TotFragBytRec");

        _summaryVector = new Vector<Object>();

        _sumTableModel = new SummaryTableModel (_summaryVector, columnNames);
        _sumTable = new JTable (_sumTableModel) {
            public boolean isCellEditable (int row, int column) {
                return false;
            }
        };

        JScrollPane ctbScrollPane = new JScrollPane (_sumTable);
        ctbScrollPane.setViewportView (_sumTable);
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,3,3,3);
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        bottomPanel.add (ctbScrollPane, gbc);

        _sumTable.setCellSelectionEnabled (false);
        _sumTable.setShowGrid (true);
        _sumTable.setShowHorizontalLines (true);
        _sumTable.setShowVerticalLines (true);
        _sumTable.setColumnSelectionAllowed (false);
        _sumTable.setRowSelectionAllowed (true);
        _sumTable.setDefaultRenderer (String.class, new DetailTableCellRenderer());

        _sumTable.getModel().addTableModelListener (new TableModelListener() {
            public void tableChanged (TableModelEvent e) {
                TableModel model = (TableModel) e.getSource();
                int row = _sumTable.getSelectedRow();
            }
        });

        this.addWindowListener (new WindowAdapter() {
             public void windowClosing (WindowEvent e) {
                 dispose();
                 System.exit (1);
             }
        });
       // pack();
    }

    private void updateForPeer (String peerName)
    {
        updateValues (_statsByPeer.get (peerName));
    }

    private void updateOverall()
    {        
        StatWrapper overallWr = new StatWrapper();
        overallWr._dsbsi = new DisServiceStatus.DisServiceBasicStatisticsInfo (ALL_PEER_LABEL);
        overallWr._dssi = new DisServiceStatus.DisServiceStatsInfo (ALL_PEER_LABEL);
        overallWr._dsdti = new DisServiceStatus.DisServiceDuplicateTrafficInfo (ALL_PEER_LABEL);
        overallWr._dsbsi = new DisServiceStatus.DisServiceBasicStatisticsInfo (ALL_PEER_LABEL);

        for (StatWrapper wr : _statsByPeer.values()) {
            if (wr._dsbsi != null) {
                overallWr._dsbsi.dataMessagesReceived += wr._dsbsi.dataMessagesReceived;
                overallWr._dsbsi.dataBytesReceived += wr._dsbsi.dataMessagesReceived;
                overallWr._dsbsi.dataFragmentsReceived += wr._dsbsi.dataFragmentsReceived;
                overallWr._dsbsi.dataFragmentBytesReceived += wr._dsbsi.dataFragmentBytesReceived;
                overallWr._dsbsi.missingFragmentRequestMessagesSent += wr._dsbsi.missingFragmentRequestMessagesSent;
                overallWr._dsbsi.missingFragmentRequestBytesSent += wr._dsbsi.missingFragmentRequestBytesSent;
                overallWr._dsbsi.missingFragmentRequestMessagesReceived += wr._dsbsi.missingFragmentRequestMessagesReceived;
                overallWr._dsbsi.missingFragmentRequestBytesReceived += wr._dsbsi.missingFragmentRequestBytesReceived;
                overallWr._dsbsi.dataCacheQueryMessagesSent += wr._dsbsi.dataCacheQueryMessagesSent;
                overallWr._dsbsi.dataCacheQueryBytesSent += wr._dsbsi.dataCacheQueryBytesSent;
                overallWr._dsbsi.dataCacheQueryMessagesReceived += wr._dsbsi.dataCacheQueryMessagesReceived;
                overallWr._dsbsi.dataCacheQueryBytesReceived += wr._dsbsi.dataCacheQueryBytesReceived;
                overallWr._dsbsi.topologyStateMessagesSent += wr._dsbsi.topologyStateMessagesSent;
                overallWr._dsbsi.topologyStateBytesSent += wr._dsbsi.topologyStateBytesSent;
                overallWr._dsbsi.topologyStateMessagesReceived += wr._dsbsi.topologyStateMessagesReceived;
                overallWr._dsbsi.topologyStateBytesReceived += wr._dsbsi.topologyStateBytesReceived;
                overallWr._dsbsi.keepAliveMessagesSent += wr._dsbsi.keepAliveMessagesSent;
                overallWr._dsbsi.keepAliveMessagesReceived += wr._dsbsi.keepAliveMessagesReceived;
                overallWr._dsbsi.queryMessagesSent += wr._dsbsi.queryMessagesSent;
                overallWr._dsbsi.queryMessagesReceived += wr._dsbsi.queryMessagesReceived;
                overallWr._dsbsi.queryHitsMessagesSent += wr._dsbsi.queryHitsMessagesSent;
                overallWr._dsbsi.queryHitsMessagesReceived += wr._dsbsi.queryHitsMessagesReceived;
            }
            if (wr._dsdti != null) {
                overallWr._dssi.clientMessagesPushed += wr._dssi.clientMessagesPushed;
                overallWr._dssi.clientBytesPushed += wr._dssi.clientBytesPushed;
                overallWr._dssi.clientMessagesMadeAvailable += wr._dssi.clientMessagesMadeAvailable;
                overallWr._dssi.clientBytesMadeAvailable += wr._dssi.clientBytesMadeAvailable;
                overallWr._dssi.fragmentsPushed += wr._dssi.fragmentsPushed;
                overallWr._dssi.fragmentBytesPushed += wr._dssi.fragmentBytesPushed;
                overallWr._dssi.onDemandFragmentsSent += wr._dssi.onDemandFragmentsSent;
                overallWr._dssi.onDemandFragmentBytesSent += wr._dssi.onDemandFragmentBytesSent;
            }
            if (wr._dssi != null) {
                overallWr._dsdti.targetedDuplicateTraffic = wr._dsdti.targetedDuplicateTraffic;
                overallWr._dsdti.overheardDuplicateTraffic = wr._dsdti.overheardDuplicateTraffic;
            }
        }

        updateValues (overallWr);
    }

    private void updateValues (StatWrapper wr)
    {
        setBasicStatisticsInfoUpdateArrived (wr == null ? null : wr._dsbsi);
        setOverallStatsInfoUpdateArrived (wr == null ? null : wr._dssi);
        setDuplicateTrafficStatisticInfoUpdateArrived (wr == null ? null : wr._dsdti);
        setPeerGroupStatisticInfoUpdateArrived (wr == null ? null : wr._dsbsibps);
    }

    private void setBasicStatisticsInfoUpdateArrived (DisServiceStatus.DisServiceBasicStatisticsInfo dsbsi)
    {
        jfKeepAliveMessagesSent.setText (dsbsi == null ? "0" : Long.toString (dsbsi.keepAliveMessagesSent));
        jfKeepAliveMessagesReceived.setText (dsbsi == null ? "0" : Long.toString (dsbsi.keepAliveMessagesReceived));
        jfMissingFragmentRequestMessagesSent.setText (dsbsi == null ? "0" : Long.toString (dsbsi.missingFragmentRequestMessagesSent));
        jfMissingFragmentRequestBytesSent.setText (dsbsi == null ? "0" : Long.toString (dsbsi.missingFragmentRequestBytesSent));
        jfMissingFragmentRequestMessagesReceived.setText (dsbsi == null ? "0" : Long.toString (dsbsi.missingFragmentRequestMessagesReceived));
        jfMissingFragmentRequestBytesReceived.setText (dsbsi == null ? "0" : Long.toString (dsbsi.missingFragmentRequestBytesReceived));
        jfOverallMessagesReceived.setText (dsbsi == null ? "0" : Long.toString (dsbsi.dataMessagesReceived));
        jfOverallBytesReceived.setText (dsbsi == null ? "0" : Long.toString (dsbsi.dataBytesReceived));
        jfOverallFragmentsReceived.setText (dsbsi == null ? "0" : Long.toString (dsbsi.dataFragmentsReceived));
        jfOverallFragmentBytesReceived.setText (dsbsi == null ? "0" : Long.toString (dsbsi.dataFragmentBytesReceived));
    }

    private void setOverallStatsInfoUpdateArrived (DisServiceStatus.DisServiceStatsInfo dssi)
    {
        jfOverallClientMessagesPushed.setText (dssi == null ? "0" : Long.toString (dssi.clientMessagesPushed));
        jfOverallClientBytesPushed.setText (dssi == null ? "0" : Long.toString (dssi.clientBytesPushed));
        jfOverallFragmentsPushed.setText (dssi == null ? "0" : Long.toString (dssi.fragmentsPushed));
        jfOverallFragmentBytesPushed.setText (dssi == null ? "0" : Long.toString (dssi.fragmentBytesPushed));
        jfOverallOnDemandFragmentsSent.setText (dssi == null ? "0" : Long.toString (dssi.onDemandFragmentsSent));
        jfOverallOnDemandFragmentBytesSent.setText (dssi == null ? "0" : Long.toString (dssi.onDemandFragmentBytesSent));
    }

    private void setDuplicateTrafficStatisticInfoUpdateArrived (DisServiceStatus.DisServiceDuplicateTrafficInfo dsdti)
    {
        jfDTNoTarget.setText (dsdti == null ? "0" : Long.toString (dsdti.overheardDuplicateTraffic));
        jfDTTargeted.setText (dsdti == null ? "0" : Long.toString (dsdti.targetedDuplicateTraffic));
    }

    private void setPeerGroupStatisticInfoUpdateArrived (Map<String, DisServiceStatus.DisServiceBasicStatisticsInfoByPeer> dsbsips)
    {
        _summaryVector.clear();

        if (dsbsips != null) {
            for (DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsip : dsbsips.values()) {
                Vector<String> sumVector = new Vector<String>();
                sumVector.add (dsbsip.remotePeerId);
                sumVector.add (Long.toString(dsbsip.keepAliveMessagesReceived));
                sumVector.add (Long.toString (dsbsip.missingFragmentRequestMessagesReceived));
                sumVector.add (Long.toString (dsbsip.missingFragmentRequestBytesReceived));
                sumVector.add (Long.toString (dsbsip.dataMessagesReceived));
                sumVector.add (Long.toString (dsbsip.dataBytesReceived));
                sumVector.add (Long.toString (dsbsip.dataFragmentsReceived));
                sumVector.add (Long.toString (dsbsip.dataFragmentBytesReceived));
                //System.out.println ("****sumVector:peerGroup: " + sumVector);
                _summaryVector.addElement (sumVector);
            }
        }

        _sumTableModel.fireTableDataChanged();
    }

    public static void main (String args[])
    {
        EventQueue.invokeLater (new Runnable() {
            public void run() {
                new MonitorConsole (2400).setVisible(true);
            }
        });
    }

    // Variables declaration
    private JLabel jfKeepAliveMessagesReceived;
    private JLabel jfKeepAliveMessagesSent;
    private JLabel jfMissingFragmentRequestBytesReceived;
    private JLabel jfMissingFragmentRequestBytesSent;
    private JLabel jfMissingFragmentRequestMessagesReceived;
    private JLabel jfMissingFragmentRequestMessagesSent;
    private JLabel jfOverallBytesReceived;
    private JLabel jfOverallClientBytesPushed;
    private JLabel jfOverallClientMessagesPushed;
    private JLabel jfOverallFragmentBytesPushed;
    private JLabel jfOverallFragmentBytesReceived;
    private JLabel jfOverallFragmentsPushed;
    private JLabel jfOverallFragmentsReceived;
    private JLabel jfOverallMessagesReceived;
    private JLabel jfOverallOnDemandFragmentBytesSent;
    private JLabel jfOverallOnDemandFragmentsSent;
    private JLabel jfDTNoTarget;
    private JLabel jfDTTargeted;

    private static final String ALL_PEER_LABEL = "All Peers";
    private String _currentSelection = ALL_PEER_LABEL;
    private Vector<Object> _summaryVector;
    private JTable _sumTable;
    private SummaryTableModel _sumTableModel;
    private DefaultListModel _peerListModel;
    private final Map<String, StatWrapper> _statsByPeer; // peerId -> StatWrapper

    private class SummaryTableModel extends DefaultTableModel
    {
        public SummaryTableModel (Vector data, Vector columnNames)
        {
            super (data, columnNames);
        }

        // Get the data from each row of the table.
        public Vector<String> getRowData (int row)
        {
            Vector connVector = new Vector();
            String virtualIP = (String) getValueAt(row, 0);
            connVector.addElement (virtualIP);
            String physIPPort = (String) getValueAt(row, 1);
            connVector.addElement (physIPPort);
            String connType = (String) getValueAt(row, 2);
            connVector.addElement (connType);
            String duration = (String) getValueAt(row, 3);
            connVector.addElement (duration);

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

    public class DetailTableCellRenderer extends JLabel implements TableCellRenderer
    {
        public DetailTableCellRenderer() {
            setOpaque(true);
        }
        public Component getTableCellRendererComponent (JTable table, Object value,
                                                        boolean isSelected, boolean hasFocus,
                                                        int row, int column)
        {
            setText (value.toString());

            if (isSelected) {
                setBackground (table.getSelectionBackground());
                setForeground (table.getSelectionForeground());
            }
            else {
                setBackground (table.getBackground());
                setForeground (table.getForeground());
            }
            setFont (new Font("Dialog", Font.PLAIN, 11));

            return this;
        }
    }
}
