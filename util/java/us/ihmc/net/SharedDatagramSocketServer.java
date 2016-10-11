/*
 * SharedDatagramSocketServer.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

package us.ihmc.net;

import java.io.InterruptedIOException;
import java.io.IOException;

import java.net.DatagramSocket;
import java.net.DatagramPacket;

import java.util.Vector;

import us.ihmc.ipc.CoordinatorListener;
import us.ihmc.ipc.MasterSlaveCoordinator;

/*******************************************************
 * @author Marco Carvalho <mcarvalho@ai.uwf.edu>
 * @version $Revision: 1.5 $
 * $Date: 2016/06/09 20:02:46 $
 * 
 * SharedDatagramSocket 
 *****************************************************/
public class SharedDatagramSocketServer implements CoordinatorListener
{
    /**
     * Creates a shared datagram socket on port (port).
     * A MasterSlaveCoordinator is created on the next (port+1)
     * port number. 
     */
    public SharedDatagramSocketServer (int port) throws Exception 
    {
        _port = port;
        _inPacketBuffer = new Vector(10);
        if (_debug) {
            System.out.println ("[SharedDatagramSocket] Creating MSCoord");
        }
        _msCoord = new MasterSlaveCoordinator(_port+1);
        _msCoord.addListener (this);
        _msCoord.waitForElectionToComplete (_electionTimeout);  
        if (!_msCoord.isMaster() && !_msCoord.isSlave()) {
            throw new Exception ("Failed to create SharedSocket. Election has timed out.");
        }
        
        if (_msCoord.isMaster()) {
            startServer();
        } 
    }
    
    //////////////////////////////////////////////////////
    // DatagramSocet emulation methods
    public void send (DatagramPacket dpack) 
    {
        if (isServerAlive()) {
            try {
                if (_msCoord.isMaster()) {
                    _privServer.sendMessage (dpack);
                } else if (_msCoord.isSlave()) {
                    DatagramPacketMessage objMsg = getMessageFromPacket (dpack);
                    _msCoord.sendMsg (objMsg);
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public DatagramPacket receive () throws IOException
    {
        DatagramPacket dpack = null;
        if (_localSoTimeout > 0) {
            if (_debug) {
                System.out.println ("_localSoTimeout=" + _localSoTimeout + ". Calling getPacket(timeout)");
            }
            dpack = getPacket (_localSoTimeout);
            if (dpack == null) {
                throw new InterruptedIOException();
            }
        }
        else {
            dpack = getPacket (0);
            if (_debug) {
                System.out.println ("Inside Synchronized - getting buffer++++++++++++++++++++++");
                System.out.println ("From Buffer: " + dpack.getLength());
                byte[] buf3 = dpack.getData();
                System.out.println ((new String(buf3)).trim());
                System.out.println ("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
            }
        }
        return (dpack);
    }
    
    //////////////////////////////////////////////////////  
    //Synchronized Datagram Buffer operations
    public synchronized DatagramPacket getPacket(long ltimeout)
    {
        try {
            while (_inPacketBuffer.size()==0) {
                if (ltimeout > 0) {
                    if (_debug) {
                        System.out.println ("Will wait on semaphore for " + ltimeout + " ms.");
                    }
                    wait (ltimeout);
                }
                else {
                    wait();
                }
            }
        }
        catch (Exception e) {
            return null;
        }
        DatagramPacket dpack = (DatagramPacket) _inPacketBuffer.firstElement();
        if (_debug) {
            System.out.println ("Inside Synchronized - getting buffer======================");
            System.out.println ("From Buffer: " + dpack.getLength());
            byte[] buf3 = dpack.getData();
            System.out.println ((new String(buf3)).trim());
            System.out.println ("-----------===============================================");
        }
        _inPacketBuffer.removeElementAt(0);
        
        return (dpack);
    }
    
    public synchronized void addPacket (DatagramPacket dpack)
    {
        _inPacketBuffer.addElement(dpack);
        notifyAll();
    }
    //////////////////////////////////////////////////////    
    
    private void startServer()
    {
        System.out.println ("Private Server is being created on port " + _port);
        _privServer = new PrivateDatagramServer (_port, this);
        _privServer.start();
    }
    
    public void setSoTimeout (int timeout)
    {
        if (_debug) {
            System.out.println ("[SharedDatagramSocketServer] setSoTimeout=" + timeout);
        }
        _localSoTimeout = timeout;    
    }
    
    public void StopServer()
    {
        _running = false;
    }
        
        
    /**
     * Invoked when a message arrives from the master
     * 
     * @param msg   the message that was received
     */
    public void msgFromMaster (Object msg)
    {
        DatagramPacketMessage objMsg = (DatagramPacketMessage) msg;
        DatagramPacket dpack = getPacketFromMessage (objMsg);
        addPacket (dpack);
    }
    
    /**
     * Invoked when a message arrives from a slave
     * 
     * @param msg   the message that was received
     */
    public void msgFromSlave (Object msg)
    {
        DatagramPacketMessage dpackMsg = (DatagramPacketMessage) msg;
        DatagramPacket dpack = getPacketFromMessage (dpackMsg);
        _privServer.sendMessage (dpack);
    }
    
    /**
     * Invoked when the master died
     */
    public void masterDied()
    {
        if (_debug) {
            System.out.println ("Master Died - elections are going to be called.");
        }
        
        try {
            Thread.currentThread().sleep(1500);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        
        if (_debug) {
            System.out.println ("Waiting for election to complete (will wait for " + _electionTimeout + " ms)");
        }
        _msCoord.waitForElectionToComplete (_electionTimeout);  
        if (_debug) {
            System.out.println ("Done waiting for elections");
        }
        if (!_msCoord.isMaster() && !_msCoord.isSlave()) {
            if (_debug) {
                System.out.println ("Master Died and the time for an election has expired.");
                System.out.println ("Server is terminating.");
            }
            _running = false;
        }
        
        if (_msCoord.isSlave()) {
            if (_debug) {
                System.out.println ("Election has ended and I'm a Slave");
            }
        }
        
        if (_msCoord.isMaster()) {
            if (_debug) {
                System.out.println ("Election has ended and I'm the Master");
            }
            startServer();
        }
    }
    
    public boolean isServerAlive()
    {
        return (_running);
    }
    
    /**
     * Invoked when one of the slaves died
     */
    public void slaveDied()
    {
        if (_debug) {
            System.out.println ("Slave died. Tough luck! There's nothing to do");
        }
    }
    
    public void handleMessage (DatagramPacket packet)
    {
        try {
            addPacket (packet);
            DatagramPacketMessage objMsg = getMessageFromPacket (packet);
            _msCoord.sendMsg (objMsg);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /////////////////////////////////////////////////////////////
    private class PrivateDatagramServer extends Thread
    {
        public PrivateDatagramServer (int port, SharedDatagramSocketServer sdServer)
        {
            try {
                _sdServer = sdServer;
                _port = port;
                _socket = new DatagramSocket (_port);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
        
        public void sendMessage (DatagramPacket dpack)
        {
            if (_socket != null) {
                try {
                    _socket.send(dpack);
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        
        public void run()
        {
            while (true) 
            {
                if (_socket == null) {
                    break;
                }
                try {
                    byte[] buf = new byte[_msgBufSize]; 
                    DatagramPacket packet = new DatagramPacket(buf, buf.length); 
                    _socket.receive(packet);
         
                    if (_debug) {
                        System.out.println ("Shared-JustGotThePacket-------------");
                        byte[] buf2 = packet.getData();
                        String str = new String(buf2);
                        System.out.println ("Data: " + str.trim());
                        System.out.println ("Shared-Done--------------------------");
                    }
                    _sdServer.handleMessage (packet);
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        
        private int _port;
        private DatagramSocket _socket = null;
        private SharedDatagramSocketServer _sdServer;
    }
    
    public int getMaxPacketSize()
    {
        return (_msgBufSize);
    }
        
        
    /////////////////////////////////////////////////////////////
    private DatagramPacket getPacketFromMessage (DatagramPacketMessage msg)
    {
        DatagramPacket dpack = new DatagramPacket(msg.getData(), msg.getLength());
        dpack.setAddress (msg.getInetAddress());
        dpack.setPort (msg.getPort());
        return (dpack);
    }
    
    private DatagramPacketMessage getMessageFromPacket (DatagramPacket packet)
    {
        DatagramPacketMessage msg = new DatagramPacketMessage();
        msg.setData(packet.getData());
        msg.setInetAddress(packet.getAddress());
        msg.setLength(packet.getLength());
        msg.setPort(packet.getPort());
        
        return (msg);
    }
    
    private MasterSlaveCoordinator _msCoord;
    private PrivateDatagramServer _privServer;
    private int _minSoTimeout = 100;        //minimum SoTimeout
    private int _electionTimeout = 5000;
    private int _msgBufSize = 9086;
    private Vector _inPacketBuffer;
    private int _localSoTimeout = 0;        //blocking
    private int _port;
    private boolean _running = true;
    private boolean _debug = false;
}
