/*
 * MonitorForwarder.java
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
 * @author Enrico Casini (ecasini@ihmc.us)
 */

package us.ihmc.aci.netProxy;

import com.google.gson.Gson;
import org.msgpack.MessagePack;
import org.msgpack.unpacker.Unpacker;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Vector;

public class MonitorForwarder
{
    private final int _udpPort;
    final private HashMap<Vector<String>, Vector<HashMap<String, Vector<String>>>> _connectorMap; //Maps the connector
    // (TCP, UDP, mockets)
    // to a vector for the 3 type of connections (TCP, UDP, ICMP)
    private Vector<Object> _connectionsVector;
    private DatagramSocket _datagramSocket;
    private final String[] EMPTY_TCP = {"0", "0", "0", "0", "0", "0"};
    private final String[] EMPTY_GENERIC = {"0", "0", "0", "0"};
    public static final int DEFAULT_PORT = 8755;


    public MonitorForwarder (int udpPort)
    {
        _udpPort = udpPort;
        _connectorMap = new HashMap<Vector<String>, Vector<HashMap<String, Vector<String>>>>();
        _connectionsVector = new Vector<Object>();
        try {
            PacketRetriever pr = new PacketRetriever();
            new Thread(pr).start();
            _datagramSocket = new DatagramSocket();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public class PacketRetriever implements Runnable
    {
        public PacketRetriever ()
        {
        }

        public void run ()
        {
            try {
                // Create a socket to listen on a port and ip only localhost.
                DatagramSocket dsocket = new DatagramSocket(_udpPort);

                // Create a buffer to read datagrams into. If a
                // packet is larger than this buffer
                byte[] buffer = new byte[2048];

                // Create a packet to receive data into the buffer
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);

                while (true) {
                    // Wait to receive a datagram
                    dsocket.receive(packet);
                    System.out.println("NetProxy: Received packet");

                    // Convert the packet contents to a msgpack to display it in the gui

                    MessagePack msgpack = new MessagePack();
                    // Deserialize
                    ByteArrayInputStream in = new ByteArrayInputStream(packet.getData());

                    Unpacker unpacker = msgpack.createUnpacker(in);

                    if (unpacker != null) {
                        String acmnpHeader = unpacker.readString();
                        if (!acmnpHeader.equals("ACMNP")) {
                            System.out.println("NO ACMNP HEADER");
                            return;
                        }
                        System.out.println("Calling update");
                        processPacket(unpacker);
                    }
                }
            }
            catch (Exception e) {
                System.err.println(e);
            }
        }
    }

    private void processPacket (Unpacker unpacker)
    {
        try {
            String sourceIP = intToIp((int) unpacker.readLong());
            System.out.println("Source NetProxy IP is: " + sourceIP);

            for (int i = 0; i < 3; i++) {       //To include TCP, UDP and ICMP
                Byte netConnection = unpacker.readByte();
                System.out.println("here1: " + netConnection);
                int quantityIn = 0;
                float rateIn = 0;
                int quantityOut = 0;
                float rateOut = 0;
                short numTCPConnections = 0;
                if (netConnection.equals((byte) 'T')) {
                    System.out.println("netConnection.equals T");
                    quantityIn = unpacker.readInt();
                    System.out.println("quantityIn: " + quantityIn);
                    rateIn = unpacker.readFloat();
                    System.out.println("rateIn: " + rateIn);
                    quantityOut = unpacker.readInt();
                    System.out.println("quantityOut: " + quantityOut);
                    rateOut = unpacker.readFloat();
                    System.out.println("rateOut: " + rateOut);
                    numTCPConnections = unpacker.readShort();
                    System.out.println("numTCPConnectors: " + numTCPConnections);
                }

                if (netConnection.equals((byte) 'U')) {
                    quantityIn = unpacker.readInt();
                    rateIn = unpacker.readFloat();
                    quantityOut = unpacker.readInt();
                    rateOut = unpacker.readFloat();
                }

                if (netConnection.equals((byte) 'I')) {
                    quantityIn = unpacker.readInt();
                    rateIn = unpacker.readFloat();
                    quantityOut = unpacker.readInt();
                    rateOut = unpacker.readFloat();
                }
            }

            int numConnectors = unpacker.readShort();
            System.out.println("numConnectors: " + numConnectors);
            int virtualIP = 0;
            int physicalIP = 0;
            int port = 0;
            String typeOfConnectorStr = "";
            long duration = 0;

            //for each connector
            for (int i = 0; i < numConnectors; i++) {

                Vector<String> dataVec = new Vector<String>();
                Vector<String> keyVec = new Vector<String>();

                //source IP
                dataVec.addElement(sourceIP);
                keyVec.addElement(sourceIP);

                Byte typeOfConnector = unpacker.readByte();
                if (typeOfConnector.equals((byte) 'T')) {
                    typeOfConnectorStr = "TCP";
                    //virtualIP = unpacker.readInt();
                    virtualIP = (int) unpacker.readLong();

                    System.out.println("virtualIP: " + intToIp(virtualIP));
                    physicalIP = (int) unpacker.readLong();
                    System.out.println("physicalIP: " + intToIp(physicalIP));
                    port = unpacker.readInt();
                    System.out.println("port: " + port);
                    duration = unpacker.readLong();
                    //System.out.println ("duration: " + duration);
                    dataVec.addElement(intToIp(virtualIP));
                    keyVec.addElement(intToIp(virtualIP));
                    //String ipPortStr = String.valueOf (physicalIP) + ":" + String.valueOf (port);
                    String ipPortStr = intToIp(physicalIP) + ":" + String.valueOf(port) + ":" + typeOfConnectorStr;
                    dataVec.addElement(ipPortStr);
                    keyVec.addElement(ipPortStr);
                    System.out.println("ipPortStr: " + ipPortStr);
                    dataVec.addElement(typeOfConnectorStr);
                    dataVec.addElement(String.valueOf(duration));
                    keyVec.addElement(typeOfConnectorStr);
                    if (_connectionsVector.isEmpty()) {
                        _connectionsVector.addElement(dataVec);

                    }
                    else {
                        boolean found = false;
                        for (Object obj : _connectionsVector) {
                            Vector v = (Vector) obj;
                            System.out.println("***typeOfConnector: " + typeOfConnector);
                            System.out.println("***v: " + v);
                            if (v.contains(ipPortStr) && v.contains("TCP")) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            _connectionsVector.addElement(dataVec);
                        }
                    }
                    System.out.println("T dataVec: " + dataVec);
                    getConnectionDetails(unpacker, keyVec, String.valueOf(duration));

                }
                else if (typeOfConnector.equals((byte) 'U')) {
                    typeOfConnectorStr = "UDP";
                    virtualIP = (int) unpacker.readLong();
                    System.out.println("virtualIP: " + intToIp(virtualIP));
                    physicalIP = (int) unpacker.readLong();
                    System.out.println("physicalIP: " + intToIp(physicalIP));
                    port = unpacker.readInt();
                    System.out.println("port: " + port);
                    duration = unpacker.readLong();
                    System.out.println("duration: " + duration);

                    //dataVec.addElement (String.valueOf (virtualIP));
                    //keyVec.addElement (String.valueOf (virtualIP));
                    dataVec.addElement(intToIp(virtualIP));
                    keyVec.addElement(intToIp(virtualIP));
                    //String ipPortStr = String.valueOf (physicalIP) + ":" + String.valueOf (port);
                    String ipPortStr = intToIp(physicalIP) + ":" + String.valueOf(port) + ":" + typeOfConnectorStr;
                    dataVec.addElement(ipPortStr);
                    keyVec.addElement(ipPortStr);
                    System.out.println("ipPortStr: " + ipPortStr);
                    dataVec.addElement(typeOfConnectorStr);
                    dataVec.addElement(String.valueOf(duration));
                    keyVec.addElement(typeOfConnectorStr);
                    System.out.println("***_connectionsVector: " + _connectionsVector);
                    if (_connectionsVector.isEmpty()) {
                        _connectionsVector.addElement(dataVec);
                    }
                    else {
                        boolean found = false;
                        for (Object obj : _connectionsVector) {
                            Vector v = (Vector) obj;
                            System.out.println("***typeOfConnector: " + typeOfConnector);
                            System.out.println("***v: " + v);
                            //if (v.contains (ipPortStr) && v.contains (typeOfConnector)) {
                            if (v.contains(ipPortStr) && v.contains("UDP")) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            _connectionsVector.addElement(dataVec);
                        }
                    }

                    System.out.println("U dataVec: " + dataVec);
                    getConnectionDetails(unpacker, keyVec, String.valueOf(duration));
                }
                else if (typeOfConnector.equals((byte) 'M')) {
                    virtualIP = (int) unpacker.readLong();
                    System.out.println("virtualIP: " + intToIp(virtualIP));
                    physicalIP = (int) unpacker.readLong();
                    System.out.println("physicalIP: " + intToIp(physicalIP));
                    port = unpacker.readInt();
                    System.out.println("port: " + port);
                    duration = unpacker.readLong();
                    System.out.println("duration: " + duration);
                    typeOfConnectorStr = "Mockets";
                    dataVec.addElement(intToIp(virtualIP));
                    keyVec.addElement(intToIp(virtualIP));
                    //String ipPortStr = String.valueOf (physicalIP) + ":" + String.valueOf (port);
                    String ipPortStr = intToIp(physicalIP) + ":" + String.valueOf(port) + ":" + typeOfConnectorStr;
                    dataVec.addElement(ipPortStr);
                    keyVec.addElement(ipPortStr);
                    dataVec.addElement(typeOfConnectorStr);
                    dataVec.addElement(String.valueOf(duration));
                    keyVec.addElement(typeOfConnectorStr);
                    System.out.println("***_connectionsVector: " + _connectionsVector);
                    if (_connectionsVector.isEmpty()) {
                        _connectionsVector.addElement(dataVec);
                    }
                    else {
                        boolean found = false;
                        for (Object obj : _connectionsVector) {
                            Vector v = (Vector) obj;
                            System.out.println("***typeOfConnector: " + typeOfConnector);
                            System.out.println("***v: " + v);
                            //if (v.contains (ipPortStr) && v.contains (typeOfConnector)) {
                            if (v.contains(ipPortStr) && v.contains("Mockets")) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            _connectionsVector.addElement(dataVec);
                        }
                    }
                    //_connectionsVector.addElement (dataVec);

                    System.out.println("M dataVec: " + dataVec);
                    getConnectionDetails(unpacker, keyVec, String.valueOf(duration));
                }
            }

            toJSONDatagram();

        }
        catch (Exception exc) {
            exc.printStackTrace();
        }
    }

    //For each Connector, get the Connections Details
    protected void getConnectionDetails (Unpacker unpacker, Vector<String> dataVec, String duration)
    {
        try {
            //All connections detail vector
            Vector<HashMap<String, Vector<String>>> connDetailVector = new Vector<HashMap<String, Vector<String>>>();
            for (int j = 0; j < 3; j++) {       //To include TCP, UDP and ICMP
                //Maps the connection UDP, TCP or ICMP to a vector of data
                HashMap<String, Vector<String>> connectionMap = new HashMap<String, Vector<String>>();
                Byte netConnection2 = unpacker.readByte();
                System.out.println("Byte value of conn: " + netConnection2.byteValue());
                int quantityIn = 0;
                float rateIn = 0;
                int quantityOut = 0;
                float rateOut = 0;
                short numTCPConnections = 0;

                if (netConnection2.equals((byte) 'T')) {
                    //System.out.println ("_showDataTCP: " + _showData);
                    Vector<String> dataVector = new Vector<String>();
                    dataVector.addElement(duration);
                    quantityIn = unpacker.readInt();
                    dataVector.addElement(String.valueOf(quantityIn));
                    //System.out.println ("quantityIn: " + quantityIn);
                    rateIn = unpacker.readFloat();
                    //System.out.println ("rateIn: " + rateIn);
                    dataVector.addElement(String.valueOf(rateIn));
                    quantityOut = unpacker.readInt();
                    //System.out.println ("quantityOut: " + quantityOut);
                    dataVector.addElement(String.valueOf(quantityOut));
                    rateOut = unpacker.readFloat();
                    //System.out.println ("rateOut: " + rateOut);
                    dataVector.addElement(String.valueOf(rateOut));
                    numTCPConnections = unpacker.readShort();
                    dataVector.addElement(String.valueOf(numTCPConnections));
                    //System.out.println ("numTCPConnectors: " + numTCPConnections);
                    connectionMap.put("TCP", dataVector);
                    connDetailVector.addElement(connectionMap);
                }

                if (netConnection2.equals((byte) 'U')) {
                    //System.out.println ("_showDataUDPP: " + _showData);
                    Vector<String> dataVector = new Vector<String>();
                    quantityIn = unpacker.readInt();
                    dataVector.addElement(String.valueOf(quantityIn));
                    //System.out.println ("quantityIn: " + quantityIn);
                    rateIn = unpacker.readFloat();
                    //System.out.println ("rateIn: " + rateIn);
                    dataVector.addElement(String.valueOf(rateIn));
                    quantityOut = unpacker.readInt();
                    //System.out.println ("quantityOut: " + quantityOut);
                    dataVector.addElement(String.valueOf(quantityOut));
                    rateOut = unpacker.readFloat();
                    //System.out.println ("rateOut: " + rateOut);
                    dataVector.addElement(String.valueOf(rateOut));
                    //numTCPConnections = unpacker.readShort();
                    //dataVector.addElement(String.valueOf(numTCPConnections));
                    //System.out.println ("numTCPConnectors: " + numTCPConnections);
                    connectionMap.put("UDP", dataVector);
                    connDetailVector.addElement(connectionMap);
                }

                if (netConnection2.equals((byte) 'I')) {
                    //System.out.println ("_showDataICMP: " + _showData);
                    Vector<String> dataVector = new Vector<String>();
                    quantityIn = unpacker.readInt();
                    dataVector.addElement(String.valueOf(quantityIn));
                    //System.out.println ("quantityIn: " + quantityIn);
                    rateIn = unpacker.readFloat();
                    //System.out.println ("rateIn: " + rateIn);
                    dataVector.addElement(String.valueOf(rateIn));
                    quantityOut = unpacker.readInt();
                    //System.out.println ("quantityOut: " + quantityOut);
                    dataVector.addElement(String.valueOf(quantityOut));
                    rateOut = unpacker.readFloat();
                    //System.out.println ("rateOut: " + rateOut);
                    dataVector.addElement(String.valueOf(rateOut));
                    //numTCPConnections = unpacker.readShort();
                    //dataVector.addElement(String.valueOf(numTCPConnections));
                    //System.out.println ("numTCPConnectors: " + numTCPConnections);
                    connectionMap.put("ICMP", dataVector);
                    connDetailVector.addElement(connectionMap);
                }
            }
            System.out.println("connDetailVector: " + connDetailVector);
            if (!_connectorMap.containsKey(dataVec)) {
                _connectorMap.put(dataVec, connDetailVector);
            }
            else {
                _connectorMap.remove(dataVec);
                _connectorMap.put(dataVec, connDetailVector);
            }
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private void toJSONDatagram ()
    {
        Gson gson = new Gson();

        HashMap<Vector<String>, Vector<String>> connTable = new HashMap<Vector<String>, Vector<String>>();


        synchronized (_connectorMap) {
            for (Vector<String> conn : _connectorMap.keySet()) {
                Vector<HashMap<String, Vector<String>>> details = _connectorMap.get(conn);
                Vector<Vector<String>> connDetailed = new Vector<Vector<String>>();
                for (int i = 0; i < details.size(); i++) {
                    if (details.get(i).containsKey("TCP")) {
                        connDetailed.add(0, details.get(i).get("TCP"));
                    }
                    else if (details.get(i).containsKey("UDP")) {
                        connDetailed.add(1, details.get(i).get("UDP"));
                    }
                    else if (details.get(i).containsKey("ICMP")) {
                        connDetailed.add(2, details.get(i).get("ICMP"));
                    }
                }

                Vector<String> finalDetails = new Vector<String>();
                finalDetails.addAll(conn); //add general header connection details

                for (int i = 0; i < connDetailed.size(); i++) {
                    if (connDetailed.get(i) != null) {
                        finalDetails.addAll(connDetailed.get(i));
                    }
                    else {
                        finalDetails.addAll((i == 0 ? Arrays.asList(EMPTY_TCP) : Arrays.asList(EMPTY_GENERIC)));
                    }
                }

                connTable.put(conn, finalDetails);

            }
        }

        System.out.println("ConnTable:" + connTable.values());
        String json = gson.toJson(connTable.values());
        byte[] jsonBuf = json.getBytes();
        try {
            DatagramPacket packet = new DatagramPacket(jsonBuf, jsonBuf.length, InetAddress.getLocalHost(),
                    _udpPort + 1);
            _datagramSocket.send(packet);
        }
        catch (UnknownHostException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
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

}
