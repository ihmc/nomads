/*
 * MasterSlaveCoordinator.java
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

package us.ihmc.ipc;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;

import java.net.ServerSocket;
import java.net.Socket;

import java.util.Enumeration;
import java.util.Random;
import java.util.Vector;

/**
 * The MasterSlaveCoordinator class is used to coordinate among multiple processes
 * running on the same host. An instance of the MasterSlaveCoordinator may flexibly
 * behave in the role of a Master or a Slave and switch between them as necessary.
 * If, upon startup, another Master is found, the current instance will behave as a
 * slave. If the Master dies, then the instance will try to take over as the Master.
 */
public class MasterSlaveCoordinator implements Runnable
{
    /**
     * Creates a MasterSlaveCoordinator using the specified port
     *
     * @param port the TCP port number to use
     */
    public MasterSlaveCoordinator (int port)
    {
        // Initialize
        _debug = false;
        _isMaster = false;
        _isSlave = false;
        _port = port;
        _slaves = new Vector();
        _randNumGen = new Random();
        _listeners = new Vector();

        if (_debug) {
            System.out.println ("MasterSlaveCoordinator: starting up on port " + _port);
        }

        // Start an election
        Thread t = new Thread (this);
        t.start();
    }
    
    /**
     * Add a listener that is notified upon msg arrival or death of master or slave
     * 
     * @param cl   the listener
     */
    public void addListener (CoordinatorListener cl)
    {
        _listeners.addElement (cl);
    }
    
    /**
     * Removes a previously added a listener
     * 
     * @param cl   the listener
     * @returns    true if the listener was removed or false if the listener was not found
     */
    public boolean removeListener (CoordinatorListener cl)
    {
        return _listeners.removeElement (cl);
    }
    
    /**
     * Send a message to the master (if this is a slave) or to all the slaves (if this is the master)
     */
    public void sendMsg (Object msg)
        throws CommException
    {
        if (_isMaster) {
            // Send the message to all the slaves
            Enumeration e = _slaves.elements();
            while (e.hasMoreElements()) {
                CommHelper connToSlave = (CommHelper) e.nextElement();
                connToSlave.sendObject (msg);
            }
        }
        else if (_isSlave) {
            // Send the message to the master
            _connToMaster.sendObject (msg);
        }
        else {
            throw new CommException ("cannot send message now due to election in progress");
        }
    }

    /**
     * Waits (optionally upto a specified maximum time) for the election of the master
     * to complete
     * 
     * @param maxWaitTime       the maximum time in milliseconds to wait until the
     *                          election completes (or < 0 to wait indefinitely)
     * 
     * @return true if the election completed successfully or false if election failed
     *              or timeout occurred before election completed
     */
    public boolean waitForElectionToComplete (long maxWaitTime)
    {
        while (true) {
            if (isMaster() || isSlave()) {
                return true;
            }
            long sleepTime = 100;
            if (maxWaitTime > 100) {
                maxWaitTime -= sleepTime;
            }
            else if (maxWaitTime > 0) {
                sleepTime = maxWaitTime;
            }
            else {
                return false;
            }
            try {
                Thread.sleep (sleepTime);
            }
            catch (InterruptedException e) {}
        }
    }

    /**
     * Returns the state of mastery of the coordinator
     * 
     * @return true if this is the master, false otherwise
     */
    public boolean isMaster()
    {
        return _isMaster;
    }

    /**
     * Returns the state of slavery of the coordinator
     * 
     * @return true if this is a slave, false otherwise
     */
    public boolean isSlave()
    {
        return _isSlave;
    }

    /**
     * Goes through an election process<p>
     * 
     * Tries to become the master and if that fails (because there is another
     * master), tries to connect to the master and becomes a slave
     */
    public void run()
    {
        if (_debug) {
            System.out.println ("MasterSlaveCoordinator: starting an election");
        }

        _isMaster = false;
        _isSlave = false;
        try {
            while (true) {
                if (_debug) {
                    System.out.println ("MasterSlaveCoordinator: trying to become master");
                }

                if (tryToBeMaster()) {
                    // This instance is now the master
                    if (_debug) {
                        System.out.println ("MasterSlaveCoordinator: became the master");
                    }
                    _isMaster = true;
                    _isSlave = false;

                    // Handle connections from slaves
                    while (true) {
                        Socket socketToSlave = _masterSocket.accept();
                        CommHelper connToSlave = new CommHelper ();
                        if (!connToSlave.init (socketToSlave)) {
                            socketToSlave.close();
                            throw new Exception ("could not initialize CommHelper");
                        }
                        _slaves.addElement (connToSlave);
                        SlaveHandler sh = new SlaveHandler (this, connToSlave);
                        sh.start();
                        if (_debug) {
                            System.out.println ("MasterSlaveCoordinator: received a connection from a slave");
                        }
                    }
                }
                else {
                    // Could not become the master - try to connect to the master
                    if (_debug) {
                        System.out.println ("MasterSlaveCoordinator: trying to connect to the master");
                    }
                    _isMaster = false;

                    _connToMaster = tryToConnectToMaster();
                    if (_connToMaster != null) {
                        MasterWaiter mw = new MasterWaiter (this, _connToMaster);
                        mw.start();
                        if (_debug) {
                            System.out.println ("MasterSlaveCoordinator: connected to the master");
                        }
                        _isSlave = true;
                        break;
                    }
                }

                // Could not become the master or connect to the master - sleep for
                // one second and try again
                try {
                    if (_debug) {
                        System.out.println ("MasterSlaveCoordinator: failed to become or contact the master: waiting for 1 sec before trying again");
                    }
                    Thread.sleep (1000);
                }
                catch (InterruptedException e) {};
            }
        }
        catch (Exception e) {
            if (_debug){
                e.printStackTrace();
            }
        }
    }

    /**
     * Invoked by the MasterWaiter when there is a message from the master
     * 
     * @param   msg             the message received from the master
     */
    void msgFromMaster (Object msg)
    {
        if (_debug) {
            System.out.println ("MasterSlaveCoordinator: received a message from the master");
        }
        
        // Send the message to all the listeners
        Enumeration e = _listeners.elements();
        while (e.hasMoreElements()) {
            CoordinatorListener cl = (CoordinatorListener) e.nextElement();
            cl.msgFromMaster (msg);
        }
    }

    /**
     * Invoked my the MasterWaiter when the master is dead - starts a new
     * election process
     * 
     * @param   connToMaster    the CommHelper handling the connection to the old master
     */
    void masterDied (CommHelper connToMaster)
    {
        if (_debug) {
            System.out.println ("MasterSlaveCoordinator: master died");
        }
        
        // Make sure that _isSlave is set to be false for now and let the
        // election determine if this instance will be the master or a slave
        _isSlave = false;

        // Sleep for a random period
        // Useful to avoid contention if many clients try to become the master
        try {
            Thread.sleep (_randNumGen.nextInt(1000));
        }
        catch (InterruptedException ex) {}

        // Start the election process
        Thread t = new Thread (this);
        t.start();

        // Notify all the listeners
        Enumeration e = _listeners.elements();
        while (e.hasMoreElements()) {
            CoordinatorListener cl = (CoordinatorListener) e.nextElement();
            cl.masterDied();
        }
    }

    /**
     * Invoked by the SlaveHandler when there is a message from a slave
     * 
     * @param   msg             the message received from the slave
     * @param   connToSlave     the CommHelper handling the connection to the slave
     */
    void msgFromSlave (Object msg, CommHelper connToSlave)
    {
        if (_debug) {
            System.out.println ("MasterSlaveCoordinator: received a message from a slave");
        }

        // Send the message to all the listeners
        Enumeration e = _listeners.elements();
        while (e.hasMoreElements()) {
            CoordinatorListener cl = (CoordinatorListener) e.nextElement();
            cl.msgFromSlave(msg);
        }
    }

    /**
     * Invoked by the SlaveHandler when a slave dies
     * 
     * @param   connToSlave     the CommHelper handling the connection to the slave
     */
    void slaveDied (CommHelper connToSlave)
    {
        if (_debug) {
            System.out.println ("MasterSlaveCoordinator: slave died");
        }

        _slaves.removeElement (connToSlave);

        // Notify all the listeners
        Enumeration e = _listeners.elements();
        while (e.hasMoreElements()) {
            CoordinatorListener cl = (CoordinatorListener) e.nextElement();
            cl.slaveDied();
        }
    }

    /**
     * Try to become the master
     * 
     * @return true if successful and false if not
     */
    protected boolean tryToBeMaster()
    {
        try {
            _masterSocket = new ServerSocket (_port);
            return true;
        }
        catch (Exception e) {
            return false;
        }
    }

    /**
     * Try to connect to the master
     * 
     * @return the CommHelper handling the connection or null if connection
     *         was not successful
     */
    protected CommHelper tryToConnectToMaster()
    {
        try {
            Socket socketToMaster = new Socket ("127.0.0.1", _port);
            CommHelper connToMaster = new CommHelper();
            if (!connToMaster.init (socketToMaster)) {
                socketToMaster.close();
                return null;
            }
            return connToMaster;
        }
        catch (Exception e) {
            return null;
        }
    }

    /*
     * Instance variables
     */
    private boolean         _debug;         // flag to control output of debugging info
    private boolean         _isMaster;      // flag to mark whether this instance is the master
    private boolean         _isSlave;       // flag to mark whether this instance is a slave
                                            //     (implies that it is connected to a master)
    private int             _port;          // port being used by this Coordinator
    private ServerSocket    _masterSocket;  // ServerSocket used if this is the master
    private CommHelper      _connToMaster;  // Used by slaves to communicate with the master
    private Vector          _slaves;        // CommHelper objects connecting to the slaves
                                            //     (used only if this is the master)
    private Random          _randNumGen;    // Random number generator
    private Vector          _listeners;     // Listeners interested in this coordinator

    /**
     * Inner class used by a slave to handle the slave's connection to the master
     */
    public class MasterWaiter extends Thread
    {
        public MasterWaiter (MasterSlaveCoordinator msc, CommHelper connToMaster)
        {
            _msc = msc;
            _connToMaster = connToMaster;
        }

        public void run()
        {
            try {
                while (true) {
                    Object msg = _connToMaster.receiveObject();
                    _msc.msgFromMaster (msg);
                }
            }
            catch (Exception e) {
                _msc.masterDied (_connToMaster);
            }
        }

        private MasterSlaveCoordinator _msc;
        private CommHelper _connToMaster;
    }

    /**
     * Inner class used by a master to handle the slave's connection to the master
     */
    public class SlaveHandler extends Thread
    {
        public SlaveHandler (MasterSlaveCoordinator msc, CommHelper connToSlave)
        {
            _msc = msc;
            _connToSlave = connToSlave;
        }

        public void run()
        {
            try {
                while (true) {
                    Object msg = _connToSlave.receiveObject();
                    _msc.msgFromSlave (msg, _connToSlave);
                }
            }
            catch (Exception e) {
                _msc.slaveDied (_connToSlave);
            }
        }

        private MasterSlaveCoordinator _msc;
        private CommHelper _connToSlave;
    }
}
