/*
 * NetProxyApplication.java
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
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
 * Net Proxy GUI Application
 *
 * Date: Oct 22, 2012
 * Time: 11:32:29 AM
 * @author  Maggie Breedy <mbreedy@ihmc.us>
 * $Revision$
 */

package us.ihmc.aci.netProxy.gui;

import org.msgpack.unpacker.Unpacker;
import org.msgpack.MessagePack;

import javax.swing.event.*;
import javax.swing.border.EtchedBorder;
import javax.swing.*;
import javax.swing.table.*;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.Font;
import java.awt.Color;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Toolkit;
import java.awt.event.*;
import java.util.Vector;
import java.util.HashMap;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.io.ByteArrayInputStream;
import java.io.IOException;

public class NetProxyApplication extends JFrame
{
    public NetProxyApplication()
    {
        _connectorMap = new HashMap<Vector<String>, Vector<HashMap<String, Vector<String>>>>();
        createGUI();
    }

    protected void updateGui (Unpacker unpacker)
    {
        try {
            System.out.println ("ipAddress: " + unpacker.readInt());
            for (int i=0; i < 3; i++) {       //To include TCP, UDP and ICMP
                Byte netConnection = unpacker.readByte();
                System.out.println ("**netConn: " + netConnection.byteValue());
                int quantityIn = 0;
                float rateIn = 0;
                int quantityOut = 0;
                float rateOut = 0;
                short numTCPConnections = 0;
                if (netConnection.equals((byte) 'T')) {
                    System.out.println ("netConnection.equals T");
                    if (_showData.equals ("showDataInBytes")) {
                        quantityIn = unpacker.readInt();
                        //System.out.println ("quantityIn: " + quantityIn);
                        rateIn = unpacker.readFloat();
                        //System.out.println ("rateIn: " + rateIn);
                        quantityOut = unpacker.readInt();
                        //System.out.println ("quantityOut: " + quantityOut);
                        rateOut = unpacker.readFloat();
                        //System.out.println ("rateOut: " + rateOut);
                        numTCPConnections = unpacker.readShort();
                        System.out.println ("numTCPConnectors: " + numTCPConnections);
                    }
                    else if (_showData.equals ("showDataInKBytes")) {
                        //System.out.println ("_showDataInKBytes: " + _showDataInKBytes);
                        quantityIn = convertIntBytesToKB (unpacker.readInt());
                        rateIn = convertFloatBytesToKB (unpacker.readFloat());
                        quantityOut = convertIntBytesToKB (unpacker.readInt());
                        rateOut = convertFloatBytesToKB (unpacker.readFloat());
                        numTCPConnections =  unpacker.readShort();
                    }
                    else if (_showData.equals ("showDataInMBytes")) {
                        //System.out.println ("_showDataInMBytes: " + _showDataInMBytes);
                        quantityIn = convertIntBytesToMB (unpacker.readInt());
                        rateIn = convertFloatBytesToMB (unpacker.readFloat());
                        quantityOut = convertIntBytesToMB (unpacker.readInt());
                        rateOut = convertFloatBytesToMB (unpacker.readFloat());
                        numTCPConnections =  unpacker.readShort();
                    }

                    if (_allConnBtnPressed) {
                        updateLabels (String.valueOf (quantityIn),  String.valueOf (rateIn), String.valueOf (quantityOut),
                                      String.valueOf (rateOut), String.valueOf (numTCPConnections), "TCP");
                    }
                }

                if (netConnection.equals((byte) 'U')) {
                    //System.out.println ("_showDataUDPP: " + _showData);
                    if (_showData.equals ("showDataInBytes")) {
                        quantityIn = unpacker.readInt();
                        rateIn = unpacker.readFloat();
                        quantityOut = unpacker.readInt();
                        rateOut = unpacker.readFloat();
                    }
                    else if (_showData.equals ("showDataInKBytes")) {
                        quantityIn = convertIntBytesToKB (unpacker.readInt());
                        rateIn = convertFloatBytesToKB (unpacker.readFloat());
                        quantityOut = convertIntBytesToKB (unpacker.readInt());
                        rateOut = convertFloatBytesToKB (unpacker.readFloat());
                    }
                    else if (_showData.equals ("showDataInMBytes")) {
                        quantityIn = convertIntBytesToMB (unpacker.readInt());
                        rateIn = convertFloatBytesToMB (unpacker.readFloat());
                        quantityOut = convertIntBytesToMB (unpacker.readInt());
                        rateOut = convertFloatBytesToMB (unpacker.readFloat());
                    }
                    if (_allConnBtnPressed) {
                        updateLabels (String.valueOf (quantityIn),  String.valueOf (rateIn), String.valueOf (quantityOut),
                                      String.valueOf (rateOut), String.valueOf (numTCPConnections), "UDP");
                    }
                }

                 if (netConnection.equals((byte) 'I')) {
                     //System.out.println ("_showDataICMP: " + _showData);
                     if (_showData.equals ("showDataInBytes")) {
                        quantityIn = unpacker.readInt();
                        rateIn = unpacker.readFloat();
                        quantityOut = unpacker.readInt();
                        rateOut = unpacker.readFloat();
                    }
                    else if (_showData.equals ("showDataInKBytes")) {
                        quantityIn = convertIntBytesToKB (unpacker.readInt());
                        rateIn = convertFloatBytesToKB (unpacker.readFloat());
                        quantityOut = convertIntBytesToKB (unpacker.readInt());
                        rateOut = convertFloatBytesToKB (unpacker.readFloat());
                    }
                    else if (_showData.equals ("showDataInMBytes")) {
                        quantityIn = convertIntBytesToMB (unpacker.readInt());
                        rateIn = convertFloatBytesToMB (unpacker.readFloat());
                        quantityOut = convertIntBytesToMB (unpacker.readInt());
                        rateOut = convertFloatBytesToMB (unpacker.readFloat());
                    }
                    if (_allConnBtnPressed) {
                        updateLabels (String.valueOf (quantityIn),  String.valueOf (rateIn), String.valueOf (quantityOut),
                                      String.valueOf (rateOut), String.valueOf (numTCPConnections), "ICMP");
                    }
                }
            }

            int numConnectors = unpacker.readShort();
            System.out.println ("numConnectors: " + numConnectors);

            int virtualIP = 0;
            int physicalIP = 0;
            int port = 0;
            String typeOfConnectorStr = "";
            long duration = 0;

            //System.out.println ("_allConnBtnPressed2: " + _allConnBtnPressed);
            //for each connector
            for (int i = 0; i<numConnectors; i++) {
                Vector<String> dataVec = new Vector<String>();
                Vector<String> keyVec = new Vector<String>();

                Byte typeOfConnector = unpacker.readByte();
                if (typeOfConnector.equals((byte) 'T')) {
                    typeOfConnectorStr = "TCP";
                    //virtualIP = unpacker.readInt();
                    virtualIP = (int) unpacker.readLong();

                    System.out.println ("virtualIP: " + intToIp  (virtualIP));
                    physicalIP = (int) unpacker.readLong();
                    System.out.println ("physicalIP: " + intToIp (physicalIP));
                    port = unpacker.readInt();
                    System.out.println ("port: " + port);
                    duration = unpacker.readLong();
                    //System.out.println ("duration: " + duration);
                    dataVec.addElement (intToIp (virtualIP));
                    keyVec.addElement (intToIp (virtualIP));
                    //String ipPortStr = String.valueOf (physicalIP) + ":" + String.valueOf (port);
                    String ipPortStr = intToIp (physicalIP) + ":" + String.valueOf (port) + ":" + typeOfConnectorStr;
                    dataVec.addElement (ipPortStr);
                    keyVec.addElement (ipPortStr);
                    System.out.println ("ipPortStr: " + ipPortStr);
                    dataVec.addElement (typeOfConnectorStr);
                    dataVec.addElement (String.valueOf (duration));
                    keyVec.addElement (typeOfConnectorStr);
                    if (_connectionsVector.isEmpty()) {
                        _connectionsVector.addElement (dataVec);

                    }
                    else {
                        boolean found = false;
                        for (Object obj : _connectionsVector) {
                            Vector v = (Vector) obj;
                            System.out.println ("***typeOfConnector: " + typeOfConnector);
                            //System.out.println ("***v: " + v);
                            if (v.contains (ipPortStr) && v.contains ("TCP")) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            _connectionsVector.addElement (dataVec);
                        }
                    }
                    //_connectionsVector.addElement (dataVec);
                    _connectionsTable.revalidate();
                    System.out.println ("T dataVec: " + dataVec);
                    getConnectionDetails (unpacker, keyVec);

                }
                else if (typeOfConnector.equals((byte) 'U')) {
                    typeOfConnectorStr = "UDP";
                    virtualIP = (int) unpacker.readLong();
                    System.out.println ("virtualIP: " + intToIp (virtualIP));
                    physicalIP = (int) unpacker.readLong();
                    System.out.println ("physicalIP: " + intToIp (physicalIP));
                    port = unpacker.readInt();
                    System.out.println ("port: " + port);
                    duration = unpacker.readLong();
                    System.out.println ("duration: " + duration);

                    //dataVec.addElement (String.valueOf (virtualIP));
                    //keyVec.addElement (String.valueOf (virtualIP));
                    dataVec.addElement (intToIp (virtualIP));
                    keyVec.addElement (intToIp (virtualIP));
                    //String ipPortStr = String.valueOf (physicalIP) + ":" + String.valueOf (port);
                    String ipPortStr = intToIp (physicalIP) + ":" + String.valueOf (port) + ":" + typeOfConnectorStr;
                    dataVec.addElement (ipPortStr);
                    keyVec.addElement (ipPortStr);
                    System.out.println ("ipPortStr: " + ipPortStr);
                    dataVec.addElement (typeOfConnectorStr);
                    dataVec.addElement (String.valueOf (duration));
                    keyVec.addElement (typeOfConnectorStr);
                    System.out.println ("***_connectionsVector: " + _connectionsVector);
                    if (_connectionsVector.isEmpty()) {
                        _connectionsVector.addElement (dataVec);
                    }
                    else {
                        boolean found = false;
                        for (Object obj : _connectionsVector) {
                            Vector v = (Vector) obj;
                            System.out.println ("***typeOfConnector: " + typeOfConnector);
                            System.out.println ("***v: " + v);
                            //if (v.contains (ipPortStr) && v.contains (typeOfConnector)) {
                            if (v.contains (ipPortStr) && v.contains ("UDP")) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            _connectionsVector.addElement (dataVec);
                        }
                    }

                    //_connectionsVector.addElement (dataVec);
                    _connectionsTable.revalidate();

                    System.out.println ("U dataVec: " + dataVec);
                    getConnectionDetails (unpacker, keyVec);
                }
                else if (typeOfConnector.equals((byte) 'M')) {
                    virtualIP = (int) unpacker.readLong();
                    System.out.println ("virtualIP: " + intToIp (virtualIP));
                    physicalIP = (int) unpacker.readLong();
                    System.out.println ("physicalIP: " + intToIp (physicalIP));
                    port = unpacker.readInt();
                    System.out.println ("port: " + port);
                    duration = unpacker.readLong();
                    System.out.println ("duration: " + duration);
                    typeOfConnectorStr = "Mockets";
                    dataVec.addElement (intToIp (virtualIP));
                    keyVec.addElement (intToIp (virtualIP));
                    //String ipPortStr = String.valueOf (physicalIP) + ":" + String.valueOf (port);
                    String ipPortStr = intToIp (physicalIP) + ":" + String.valueOf (port) + ":" +typeOfConnectorStr;
                    dataVec.addElement (ipPortStr);
                    keyVec.addElement (ipPortStr);
                    dataVec.addElement (typeOfConnectorStr);
                    dataVec.addElement (String.valueOf (duration));
                    keyVec.addElement (typeOfConnectorStr);
                    System.out.println ("***_connectionsVector: " + _connectionsVector);
                    if (_connectionsVector.isEmpty()) {
                        _connectionsVector.addElement (dataVec);
                    }
                    else {
                        boolean found = false;
                        for (Object obj : _connectionsVector) {
                            Vector v = (Vector) obj;
                            System.out.println ("***typeOfConnector: " + typeOfConnector);
                            System.out.println ("***v: " + v);
                            //if (v.contains (ipPortStr) && v.contains (typeOfConnector)) {
                            if (v.contains (ipPortStr) && v.contains ("Mockets")) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            _connectionsVector.addElement (dataVec);
                        }
                    }
                    //_connectionsVector.addElement (dataVec);
                    _connectionsTable.revalidate();
                    
                    System.out.println ("M dataVec: " + dataVec);
                    getConnectionDetails (unpacker, keyVec);
                }
            }
        }
        catch (Exception exc) {
            exc.printStackTrace();
        }
    }

    //For each Connector, get the Connections Details
    protected void getConnectionDetails (Unpacker unpacker, Vector<String> dataVec)
    {
        try {
            //All connections detail vector
            Vector<HashMap<String, Vector<String>>> connDetailVector = new Vector<HashMap<String, Vector<String>>>();
            for (int j=0; j < 3; j++) {       //To include TCP, UDP and ICMP
                //Maps the connection UDP, TCP or ICMP to a vector of data
                HashMap<String, Vector<String>> connectionMap = new HashMap<String,  Vector<String>>();
                Byte netConnection2 = unpacker.readByte();
                System.out.println ("Byte value of conn: " + netConnection2.byteValue());
                int quantityIn = 0;
                float rateIn = 0;
                int quantityOut = 0;
                float rateOut = 0;
                short numTCPConnections = 0;

                if (netConnection2.equals((byte) 'T')) {
                    //System.out.println ("_showDataTCP: " + _showData);
                    Vector<String> dataVector = new Vector<String>();
                    if (_showData.equals ("showDataInBytes")) {
                        quantityIn = unpacker.readInt();
                        dataVector.addElement (String.valueOf (quantityIn));
                        //System.out.println ("quantityIn: " + quantityIn);
                        rateIn = unpacker.readFloat();
                        //System.out.println ("rateIn: " + rateIn);
                        dataVector.addElement (String.valueOf (rateIn));
                        quantityOut = unpacker.readInt();
                        //System.out.println ("quantityOut: " + quantityOut);
                        dataVector.addElement (String.valueOf (quantityOut));
                        rateOut = unpacker.readFloat();
                        //System.out.println ("rateOut: " + rateOut);
                        dataVector.addElement (String.valueOf (rateOut));
                        numTCPConnections = unpacker.readShort();
                        dataVector.addElement (String.valueOf (numTCPConnections));
                        //System.out.println ("numTCPConnectors: " + numTCPConnections);
                    }
                    else if (_showData.equals ("showDataInKBytes")) {
                        //System.out.println ("_showDataInKBytes: " + _showDataInKBytes);
                        quantityIn = convertIntBytesToKB (unpacker.readInt());
                        dataVector.addElement (String.valueOf (quantityIn));
                        rateIn = convertFloatBytesToKB (unpacker.readFloat());
                        dataVector.addElement (String.valueOf (rateIn));
                        quantityOut = convertIntBytesToKB (unpacker.readInt());
                        dataVector.addElement (String.valueOf (quantityOut));
                        rateOut = convertFloatBytesToKB (unpacker.readFloat());
                        dataVector.addElement (String.valueOf (rateOut));
                        numTCPConnections =  unpacker.readShort();
                        dataVector.addElement (String.valueOf (numTCPConnections));
                    }
                    else if (_showData.equals ("showDataInMBytes")) {
                        //System.out.println ("_showDataInMBytes: " + _showDataInMBytes);
                       quantityIn = convertIntBytesToMB (unpacker.readInt());
                        dataVector.addElement (String.valueOf (quantityIn));
                        rateIn = convertFloatBytesToMB (unpacker.readFloat());
                        dataVector.addElement (String.valueOf (rateIn));
                        quantityOut = convertIntBytesToMB (unpacker.readInt());
                        dataVector.addElement (String.valueOf (quantityOut));
                        rateOut = convertFloatBytesToMB (unpacker.readFloat());
                        dataVector.addElement (String.valueOf (rateOut));
                        numTCPConnections =  unpacker.readShort();
                        dataVector.addElement (String.valueOf (numTCPConnections));
                    }
                    connectionMap.put ("TCP", dataVector);
                    connDetailVector.addElement (connectionMap);
                }

                if (netConnection2.equals((byte) 'U')) {
                    //System.out.println ("_showDataUDPP: " + _showData);
                    Vector<String> dataVector = new Vector<String>();
                    if (_showData.equals ("showDataInBytes")) {
                        quantityIn = unpacker.readInt();
                        dataVector.addElement (String.valueOf (quantityIn));
                        //System.out.println ("quantityIn: " + quantityIn);
                        rateIn = unpacker.readFloat();
                        //System.out.println ("rateIn: " + rateIn);
                        dataVector.addElement (String.valueOf (rateIn));
                        quantityOut = unpacker.readInt();
                        //System.out.println ("quantityOut: " + quantityOut);
                        dataVector.addElement (String.valueOf (quantityOut));
                        rateOut = unpacker.readFloat();
                        //System.out.println ("rateOut: " + rateOut);
                        dataVector.addElement (String.valueOf (rateOut));
                        //numTCPConnections = unpacker.readShort();
                        dataVector.addElement (String.valueOf (numTCPConnections));
                        //System.out.println ("numTCPConnectors: " + numTCPConnections);
                    }
                    else if (_showData.equals ("showDataInKBytes")) {
                        //System.out.println ("_showDataInKBytes: " + _showDataInKBytes);
                        quantityIn = convertIntBytesToKB (unpacker.readInt());
                        dataVector.addElement (String.valueOf (quantityIn));
                        rateIn = convertFloatBytesToKB (unpacker.readFloat());
                        dataVector.addElement (String.valueOf (rateIn));
                        quantityOut = convertIntBytesToKB (unpacker.readInt());
                        dataVector.addElement (String.valueOf (quantityOut));
                        rateOut = convertFloatBytesToKB (unpacker.readFloat());
                        dataVector.addElement (String.valueOf (rateOut));
                        //numTCPConnections =  unpacker.readShort();
                        dataVector.addElement (String.valueOf (numTCPConnections));
                    }
                    else if (_showData.equals ("showDataInMBytes")) {
                        //System.out.println ("_showDataInMBytes: " + _showDataInMBytes);
                       quantityIn = convertIntBytesToMB (unpacker.readInt());
                        dataVector.addElement (String.valueOf (quantityIn));
                        rateIn = convertFloatBytesToMB (unpacker.readFloat());
                        dataVector.addElement (String.valueOf (rateIn));
                        quantityOut = convertIntBytesToMB (unpacker.readInt());
                        dataVector.addElement (String.valueOf (quantityOut));
                        rateOut = convertFloatBytesToMB (unpacker.readFloat());
                        dataVector.addElement (String.valueOf (rateOut));
                        //numTCPConnections =  unpacker.readShort();
                        dataVector.addElement (String.valueOf (numTCPConnections));
                    }
                     connectionMap.put ("UDP", dataVector);
                    connDetailVector.addElement (connectionMap);
                }

                 if (netConnection2.equals((byte) 'I')) {
                     //System.out.println ("_showDataICMP: " + _showData);
                     Vector<String> dataVector = new Vector<String>();
                     if (_showData.equals ("showDataInBytes")) {
                         quantityIn = unpacker.readInt();
                         dataVector.addElement (String.valueOf (quantityIn));
                         //System.out.println ("quantityIn: " + quantityIn);
                         rateIn = unpacker.readFloat();
                         //System.out.println ("rateIn: " + rateIn);
                         dataVector.addElement (String.valueOf (rateIn));
                         quantityOut = unpacker.readInt();
                         //System.out.println ("quantityOut: " + quantityOut);
                         dataVector.addElement (String.valueOf (quantityOut));
                         rateOut = unpacker.readFloat();
                         //System.out.println ("rateOut: " + rateOut);
                         dataVector.addElement (String.valueOf (rateOut));
                         //numTCPConnections = unpacker.readShort();
                         dataVector.addElement (String.valueOf (numTCPConnections));
                         //System.out.println ("numTCPConnectors: " + numTCPConnections);
                     }
                     else if (_showData.equals ("showDataInKBytes")) {
                         //System.out.println ("_showDataInKBytes: " + _showDataInKBytes);
                         quantityIn = convertIntBytesToKB (unpacker.readInt());
                         dataVector.addElement (String.valueOf (quantityIn));
                         rateIn = convertFloatBytesToKB (unpacker.readFloat());
                         dataVector.addElement (String.valueOf (rateIn));
                         quantityOut = convertIntBytesToKB (unpacker.readInt());
                         dataVector.addElement (String.valueOf (quantityOut));
                         rateOut = convertFloatBytesToKB (unpacker.readFloat());
                         dataVector.addElement (String.valueOf (rateOut));
                         //numTCPConnections =  unpacker.readShort();
                         dataVector.addElement (String.valueOf (numTCPConnections));
                     }
                     else if (_showData.equals ("showDataInMBytes")) {
                         //System.out.println ("_showDataInMBytes: " + _showDataInMBytes);
                         quantityIn = convertIntBytesToMB (unpacker.readInt());
                         dataVector.addElement (String.valueOf (quantityIn));
                         rateIn = convertFloatBytesToMB (unpacker.readFloat());
                         dataVector.addElement (String.valueOf (rateIn));
                         quantityOut = convertIntBytesToMB (unpacker.readInt());
                         dataVector.addElement (String.valueOf (quantityOut));
                         rateOut = convertFloatBytesToMB (unpacker.readFloat());
                         dataVector.addElement (String.valueOf (rateOut));
                         //numTCPConnections =  unpacker.readShort();
                         dataVector.addElement (String.valueOf (numTCPConnections));
                     }
                      connectionMap.put ("UDP", dataVector);
                     connDetailVector.addElement (connectionMap);
                }
            }
            System.out.println ("connDetailVector: " + connDetailVector);
            if (!_connectorMap.containsKey (dataVec)) {
                _connectorMap.put (dataVec, connDetailVector);
            }
            else {
                _connectorMap.remove (dataVec);
                _connectorMap.put (dataVec, connDetailVector);
            }
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    protected void updateLabels (String quantityIn, String rateIn, String quantityOut, String rateOut, String numTCPConnections,
                                 String connectionType)
    {
        if (connectionType.equals ("TCP")) {
            _trInQuantityLabelTCP.setText (quantityIn);
            _trOutQuantityLabelTCP.setText (quantityOut);
            _trInRatelabelTCP.setText (rateIn);
            _trOutRatelabelTCP.setText (rateOut);
            _numberOfTCPConLabel.setText (numTCPConnections);
        }
        else if (connectionType.equals ("UDP")) {
                _trInQuantityLabelUDP.setText (quantityIn);
                _trOutQuantityLabelUDP.setText (quantityOut);
                _trInRatelabelUDP.setText (rateIn);
                _trOutRatelabelUDP.setText (rateOut);
        }
        else if (connectionType.equals ("ICMP")) {
            _trInQuantityLabelICMP.setText (quantityIn);
            _trOutQuantityLabelICMP.setText (quantityOut);
            _trInRatelabelICMP.setText (rateIn);
            _trOutRatelabelICMP.setText (rateOut);
        }
    }

    protected int convertIntBytesToKB (int value)
    {
        return value / 1024;
    }

    protected int convertIntBytesToMB (int value)
    {
        return value / 1024 * 1024;
    }

    protected float convertFloatBytesToKB (float value)
    {
        return value / 1024;
    }

    protected float convertFloatBytesToMB (float value)
    {
        return value / 1024 * 1024;
    }

    /**
    * Method for converting an existing IP in Integer format to String format.
    *
    * @param integerIp The IP Address in Integer format
    * @return A String representation of the IP Address
    */
    public static String intToIp (int integerIp)
    {
    //    return ((integerIp >> 24) & 0xFF) + "." + ((integerIp >> 16) & 0xFF)
    //            + "." + ((integerIp >> 8) & 0xFF) + "." + (integerIp & 0xFF);

        return (integerIp & 0xFF) + "." + ((integerIp >> 8) & 0xFF)
                + "." + ((integerIp >> 16) & 0xFF) + "." + ((integerIp >> 24) & 0xFF);
    }

    protected void createGUI()
    {
        setTitle ("Net Proxy Application");
        setSize (800, 600);
        //setMinimumSize (new Dimension (800, 600));
        getContentPane().setLayout (new GridBagLayout());
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-800)/2, (screenSize.height-600)/2, 800, 600);

        //Main Panels
        GridBagConstraints gbc = new GridBagConstraints();
        JPanel leftPanel = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        getContentPane().add (leftPanel, gbc);
        leftPanel.setBorder (new EtchedBorder());
        leftPanel.setBackground (new Color (198,216,226));

        /*JPanel rightPanel = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;
        getContentPane().add (rightPanel, gbc);
        rightPanel.setBorder (new EtchedBorder());
        rightPanel.setBackground (new Color (198,216,226));*/

        JPanel botPanel = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.gridwidth = 2;
        gbc.gridheight = 1;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.3;
        gbc.weighty = 0.3;
        getContentPane().add (botPanel, gbc);
        //botPanel.setBorder (new EtchedBorder());
        //botPanel.setBackground (new Color (191,209,229));

        //LeftPanel components
        gbc = new GridBagConstraints();
        JLabel tlabel = new JLabel ("Net Proxy Connections");
        tlabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,13);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.1;
        leftPanel.add (tlabel,gbc);

        Vector columnNames = new Vector (3);
        columnNames.addElement ("Virtual IP");
        columnNames.addElement ("Physical IP:Port");
        columnNames.addElement ("Connector type");
        columnNames.addElement ("Duration");

        _connectionsVector = new Vector<Object>();

        _connectionsTableModel = new ConnectionsTableModel (_connectionsVector, columnNames);
        _connectionsTable = new JTable (_connectionsTableModel) {
            public boolean isCellEditable (int row, int column) {
                return false;
            }
        };

        JScrollPane ctbScrollPane = new JScrollPane (_connectionsTable);
        ctbScrollPane.setViewportView (_connectionsTable);
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,13);
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        leftPanel.add (ctbScrollPane, gbc);

        _connectionsTable.setCellSelectionEnabled (false);
        _connectionsTable.setShowGrid (true);
        _connectionsTable.setShowHorizontalLines (true);
        _connectionsTable.setShowVerticalLines (true);
        _connectionsTable.setColumnSelectionAllowed (false);
        _connectionsTable.setRowSelectionAllowed (true);

        _connectionsTable.getSelectionModel().addListSelectionListener (new ListSelectionListener() {
            public void valueChanged(ListSelectionEvent e) {
                //TableModel model = (TableModel) e.getSource();
                int row = _connectionsTable.getSelectedRow();
                System.out.println ("->>>>Row Selected: " + row);
                if (row >= 0) {
                    _allConnBtnPressed = false;
                    Vector<String> connVector = _connectionsTableModel.getRowData (row);
                    System.out.println ("->>>>connVector: " + connVector);
                    Vector<String> keyConnVector = _connectionsTableModel.getKeyElements (connVector);
                    System.out.println ("->>>>keyConnVector: " + keyConnVector);
                    if (_connectorMap.size() > 0) {
                        if (_connectorMap.containsKey (keyConnVector)) {
                            Vector<HashMap<String, Vector<String>>> connDetailVector  = _connectorMap.get (keyConnVector);
                            for (HashMap<String, Vector<String>> connMap : connDetailVector) {
                                for (String conn : connMap.keySet()) {
                                    Vector<String> dataVector = connMap.get (conn);
                                    if (dataVector.size() > 4) {
                                        updateLabels (dataVector.elementAt(0), dataVector.elementAt(1), dataVector.elementAt(2),
                                                      dataVector.elementAt(3), dataVector.elementAt(4), conn);
                                    }
                                    else {
                                        System.out.println ("->>>>dataVector is less than 4");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        });

        //RightPanel
/*        gbc = new GridBagConstraints();
        JLabel label = new JLabel ("Connection Details");
        label.setFont (new Font ("Frame", Font.BOLD, 14));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,13,13);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.1;
        rightPanel.add (label,gbc);

        JLabel numlabel = new JLabel ("Number of TCP Connection: ");
        numlabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,5,13);
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 0.1;
        rightPanel.add (numlabel,gbc);

        _numConnLabel = new JLabel ("0");
        numlabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,5,13);
        gbc.gridx = 1;
        gbc.gridy = 1;
        gbc.weightx = 0.2;
        rightPanel.add (_numConnLabel,gbc);

        JLabel trfInlabel = new JLabel ("Traffic in: ");
        numlabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,5,13);
        gbc.gridx = 0;
        gbc.gridy = 2;
        gbc.weightx = 0.1;
        rightPanel.add (trfInlabel,gbc);

        _traffInLabel = new JLabel ("0");
        numlabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,5,13);
        gbc.gridx = 1;
        gbc.gridy = 2;
        gbc.weightx = 0.2;
        rightPanel.add (_traffInLabel,gbc);

        JLabel trfOfflabel = new JLabel ("Traffic out: ");
        numlabel.setFont (new Font ("Frame", Font.PLAIN, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,5,13);
        gbc.gridx = 0;
        gbc.gridy = 3;
        gbc.weightx = 0.1;
        rightPanel.add (trfOfflabel,gbc);

        _traffOutLabel = new JLabel ("0");
        numlabel.setFont (new Font ("Frame", Font.BOLD, 12));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,5,13);
        gbc.gridx = 1;
        gbc.gridy = 3;
        gbc.weightx = 0.2;
        rightPanel.add (_traffOutLabel,gbc);*/

        //Bottom Panel
        gbc = new GridBagConstraints();
        JPanel panel1 = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.3;
        gbc.weighty = 0.3;
        botPanel.add (panel1, gbc);
        //panel1.setBackground (new Color (198,216,226));

        JPanel panel2 = new JPanel (new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 0.09;
        gbc.weighty = 0.09;
        botPanel.add (panel2, gbc);
        panel2.setBackground (new Color (198,216,226));
        //panel2.setBorder (new EtchedBorder());

        //Panel1
        gbc = new GridBagConstraints();
        JLabel sumlabel = new JLabel ("Connection Summary");
        sumlabel.setFont (new Font ("Frame", Font.BOLD, 14));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,13);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.1;
        panel1.add (sumlabel,gbc);

        JLabel conlabel = new JLabel ("Connection type: TCP");
        conlabel.setFont (new Font ("Frame", Font.BOLD, 13));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,0,5);
        gbc.gridx = 0;
        gbc.gridy = 1;
        //gbc.weightx = 0.1;
        panel1.add (conlabel,gbc);

        //number of TCP connectors
        _numberOfTCPConLabel = new JLabel ("0");
        _numberOfTCPConLabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,5,0,5);
        gbc.gridx = 1;
        gbc.gridy = 1;
        //gbc.weightx = 0.1;
        panel1.add (_numberOfTCPConLabel, gbc);

        JLabel trinQuanlabel = new JLabel ("Traffic In  Quantity:");
        trinQuanlabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,5);
        gbc.gridx = 0;
        gbc.gridy = 2;
        //gbc.weightx = 0.1;
        panel1.add (trinQuanlabel,gbc);

        _trInQuantityLabelTCP = new JLabel ("0");
        _trInQuantityLabelTCP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 1;
        gbc.gridy = 2;
        //gbc.weightx = 0.1;
        panel1.add (_trInQuantityLabelTCP,gbc);

        JLabel troutQuanlabel = new JLabel ("Traffic Out  Quantity:");
        troutQuanlabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 2;
        gbc.gridy = 2;
        //gbc.weightx = 0.1;
        panel1.add (troutQuanlabel,gbc);

        _trOutQuantityLabelTCP = new JLabel ("0");
        _trOutQuantityLabelTCP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 3;
        gbc.gridy = 2;
        //gbc.weightx = 0.1;
        panel1.add (_trOutQuantityLabelTCP,gbc);

        //Rate
        JLabel trinRatelabel = new JLabel ("Traffic In  Rate:");
        trinRatelabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,5);
        gbc.gridx = 0;
        gbc.gridy = 3;
        //gbc.weightx = 0.1;
        panel1.add (trinRatelabel,gbc);

        _trInRatelabelTCP = new JLabel ("0");
        _trInRatelabelTCP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 1;
        gbc.gridy = 3;
        //gbc.weightx = 0.1;
        panel1.add (_trInRatelabelTCP,gbc);

        JLabel troutRatelabel = new JLabel ("Traffic Out  Rate:");
        troutRatelabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 2;
        gbc.gridy = 3;
        //gbc.weightx = 0.1;
        panel1.add (troutRatelabel,gbc);

        _trOutRatelabelTCP = new JLabel ("0");
        _trOutRatelabelTCP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 3;
        gbc.gridy = 3;
        //gbc.weightx = 0.1;
        panel1.add (_trOutRatelabelTCP,gbc);

        //For UDP
        JLabel udpconlabel = new JLabel ("Connection type: UDP");
        udpconlabel.setFont (new Font ("Frame", Font.BOLD, 13));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,0,5);
        gbc.gridx = 0;
        gbc.gridy = 4;
        gbc.weightx = 0.1;
        panel1.add (udpconlabel,gbc);

        JLabel trinQuanlabelUDP = new JLabel ("Traffic In  Quantity:");
        trinQuanlabelUDP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,5);
        gbc.gridx = 0;
        gbc.gridy = 5;
        //gbc.weightx = 0.1;
        panel1.add (trinQuanlabelUDP,gbc);

        _trInQuantityLabelUDP = new JLabel ("0");
        _trInQuantityLabelUDP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 1;
        gbc.gridy = 5;
        //gbc.weightx = 0.1;
        panel1.add (_trInQuantityLabelUDP,gbc);

        JLabel troutQuanlabelUDP = new JLabel ("Traffic Out  Quantity:");
        troutQuanlabelUDP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 2;
        gbc.gridy = 5;
        //gbc.weightx = 0.1;
        panel1.add (troutQuanlabelUDP,gbc);

        _trOutQuantityLabelUDP = new JLabel ("0");
        _trOutQuantityLabelUDP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 3;
        gbc.gridy = 5;
        //gbc.weightx = 0.1;
        panel1.add (_trOutQuantityLabelUDP,gbc);

        //Rate
        JLabel udprinRatelabel = new JLabel ("Traffic In  Rate:");
        udprinRatelabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,5);
        gbc.gridx = 0;
        gbc.gridy = 6;
        //gbc.weightx = 0.1;
        panel1.add (udprinRatelabel,gbc);

        _trInRatelabelUDP = new JLabel ("0");
        _trInRatelabelUDP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 1;
        gbc.gridy = 6;
        //gbc.weightx = 0.1;
        panel1.add (_trInRatelabelUDP,gbc);

        JLabel udproutRatelabel = new JLabel ("Traffic Out  Rate:");
        udproutRatelabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 2;
        gbc.gridy = 6;
        //gbc.weightx = 0.1;
        panel1.add (udproutRatelabel,gbc);

        _trOutRatelabelUDP = new JLabel ("0");
        _trOutRatelabelUDP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 3;
        gbc.gridy = 6;
        //gbc.weightx = 0.1;
        panel1.add (_trOutRatelabelUDP,gbc);

        //ICMP Traffic
        JLabel icnpconlabel = new JLabel ("Connection type: ICMP");
        icnpconlabel.setFont (new Font ("Frame", Font.BOLD, 13));
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,13,0,5);
        gbc.gridx = 0;
        gbc.gridy = 7;
        gbc.weightx = 0.1;
        panel1.add (icnpconlabel,gbc);

        JLabel trinQuanlabelICMP = new JLabel ("Traffic In  Quantity:");
        trinQuanlabelICMP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,5);
        gbc.gridx = 0;
        gbc.gridy = 8;
        //gbc.weightx = 0.1;
        panel1.add (trinQuanlabelICMP,gbc);

        _trInQuantityLabelICMP = new JLabel ("0");
        _trInQuantityLabelICMP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 1;
        gbc.gridy = 8;
        //gbc.weightx = 0.1;
        panel1.add (_trInQuantityLabelICMP,gbc);

        JLabel troutQuanlabelICMP = new JLabel ("Traffic Out  Quantity:");
        troutQuanlabelICMP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 2;
        gbc.gridy = 8;
        //gbc.weightx = 0.1;
        panel1.add (troutQuanlabelICMP,gbc);

        _trOutQuantityLabelICMP = new JLabel ("0");
        _trOutQuantityLabelICMP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 3;
        gbc.gridy = 8;
        //gbc.weightx = 0.1;
        panel1.add (_trOutQuantityLabelICMP,gbc);

        //Rate
        JLabel irinRatelabel = new JLabel ("Traffic In  Rate:");
        irinRatelabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,13,5,5);
        gbc.gridx = 0;
        gbc.gridy = 9;
        //gbc.weightx = 0.1;
        panel1.add (irinRatelabel,gbc);

        _trInRatelabelICMP = new JLabel ("0");
        _trInRatelabelICMP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 1;
        gbc.gridy = 9;
        //gbc.weightx = 0.1;
        panel1.add (_trInRatelabelICMP,gbc);

        JLabel iroutRatelabel = new JLabel ("Traffic Out  Rate:");
        iroutRatelabel.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 2;
        gbc.gridy = 9;
        //gbc.weightx = 0.1;
        panel1.add (iroutRatelabel,gbc);

        _trOutRatelabelICMP = new JLabel ("0");
        _trOutRatelabelICMP.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 3;
        gbc.gridy = 9;
        //gbc.weightx = 0.1;
        panel1.add (_trOutRatelabelICMP,gbc);


        //Panel2
        gbc = new GridBagConstraints();
        JButton allConnBtn = new JButton ("All Connections Summary");
        allConnBtn.setFont (new Font ("Frame", Font.BOLD, 12));
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.insets = new Insets (5,5,50,5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        //gbc.weightx = 0.1;
        panel2.add (allConnBtn, gbc);


        JRadioButton bytesRadioButton = new JRadioButton();
        JRadioButton kRadioButton = new JRadioButton();
        JRadioButton mRadioButton = new JRadioButton();

        bytesRadioButton.setText ("Bytes/sec");
        bytesRadioButton.setBackground (new Color (198,216,226));
        gbc.gridx = 0;
        //to align the buttons
        gbc.gridy = GridBagConstraints.RELATIVE;
        gbc.anchor = GridBagConstraints.WEST;
        bytesRadioButton.setSelected (true);
        gbc.gridy = 1;
        gbc.insets = new Insets (0,5,5,5);
        panel2.add (bytesRadioButton, gbc);
        _showData = "showDataInBytes";

        gbc.gridy = 2;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = GridBagConstraints.RELATIVE;
        gbc.anchor = GridBagConstraints.WEST;
        panel2.add (kRadioButton, gbc);
        kRadioButton.setBackground (new Color (198,216,226));
        //kRadioButton.setAlignmentX (SwingConstants.LEFT);
        kRadioButton.setText ("KB/sec");

        gbc.gridy = 3;
        gbc.insets = new Insets (0,5,5,5);
        gbc.gridx = 0;
        gbc.gridy = GridBagConstraints.RELATIVE;
        gbc.anchor = GridBagConstraints.WEST;
        
        panel2.add (mRadioButton, gbc);
        mRadioButton.setBackground (new Color (198,216,226));
        //mRadioButton.setAlignmentX (SwingConstants.LEFT);
        mRadioButton.setText ("MB/sec");

        ButtonGroup buttonGroup = new ButtonGroup();
        buttonGroup.add (bytesRadioButton);
        buttonGroup.add (kRadioButton);
        buttonGroup.add (mRadioButton);

        bytesRadioButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _showData = "showDataInBytes";
            }
        });

        kRadioButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _showData = "showDataInKBytes";
            }
        });
        
        mRadioButton.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                _showData = "showDataInMBytes";
            }
        });

        allConnBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e) {
                System.out.println ("ALL Connections Summary");
                _allConnBtnPressed = true;
                _connectionsTable.getSelectionModel().clearSelection();
            }
        });

        this.addWindowListener (new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                dispose();
                System.exit (1);
            }
        });

        repaint();

        //Starts thread to retrieve a packet
        (new Thread (new PacketRetriever ())).start();
    }


   /**
     * Table Model for the net proxy connections
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


    public static void main (String[] args) throws Exception
    {
        if (args.length == 1) {
           _port = Integer.parseInt (args[0]);
        }
        NetProxyApplication npa = new NetProxyApplication();
        npa.setVisible (true);
    }
    
    public  class PacketRetriever implements Runnable
    {
        public PacketRetriever()
        {
        }

        public void run ()
        {
            try {
                if (_port <= 0) {
                    _port = 8755;
                }
                // Create a socket to listen on a port and ip only localhost.
                //InetAddress inetAdd = InetAddress.getByName ("localhost");
                //DatagramSocket dsocket = new DatagramSocket (_port, inetAdd);
		        DatagramSocket dsocket = new DatagramSocket (_port);
                // Create a buffer to read datagrams into. If a
                // packet is larger than this buffer
                byte[] buffer = new byte [2048];

                // Create a packet to receive data into the buffer
                DatagramPacket packet = new DatagramPacket (buffer, buffer.length);

                while (true) {
                    // Wait to receive a datagram
                    dsocket.receive (packet);

                    // Convert the packet contents to a msgpack to display it in the gui
                    MessagePack msgpack = new MessagePack();
                    // Deserialize
                    ByteArrayInputStream in = new ByteArrayInputStream (packet.getData());
                    Unpacker unpacker = msgpack.createUnpacker (in);

                    if (unpacker != null) {
                        String acmnpHeader = unpacker.readString();
                        if (!acmnpHeader.equals ("ACMNP")) {
                            System.out.println ("NO ACMNP HEADER");
                            return;
                        }
                        System.out.println ("Calling update");
                        updateGui (unpacker);
                    }
                }
            }
            catch (Exception e) {
                System.err.println(e);
            }
        }
    }

    //Variables
    protected String _showData;
    protected static int _port;

    protected HashMap<Vector<String>, Vector<HashMap<String, Vector<String>>>> _connectorMap; //Maps the connector (TCP, UDP, mockets) 
                                                                              // to a vector for the 3 type of connections (TCP, UDP, ICMP)

    protected volatile boolean _allConnBtnPressed = true;
    protected JLabel _numConnLabel;
    protected JLabel _traffInLabel;
    protected JLabel _traffOutLabel;
    protected JLabel _trInQuantityLabelTCP;
    protected JLabel _trInRatelabelTCP;
    protected JLabel _trOutQuantityLabelTCP;
    protected JLabel _trOutRatelabelTCP;
    protected JLabel _numberOfTCPConLabel;
    protected JLabel _trInQuantityLabelUDP;
    protected JLabel _trInRatelabelUDP;
    protected JLabel _trOutQuantityLabelUDP;
    protected JLabel _trOutRatelabelUDP;
    protected JLabel _trInQuantityLabelICMP;
    protected JLabel _trInRatelabelICMP;
    protected JLabel _trOutQuantityLabelICMP;
    protected JLabel _trOutRatelabelICMP;

    protected int _quantityIn = 0;
    protected float _rateIn = 0;
    protected int _quantityOut = 0;
    protected float _rateOut = 0;
    protected short _numTCPConnections = 0;


    protected JTable _connectionsTable;

    protected ConnectionsTableModel _connectionsTableModel;
    protected TableModel _model;

    protected Vector<Object> _connectionsVector;
    protected Vector<Object> _summaryVector;
}
