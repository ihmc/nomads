/*
 * SerialComm.java
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

package us.ihmc.util;

import java.io.*;
import java.util.*;
import javax.comm.*;

/**
 * The SerialComm class encapsulates the RS232 serial communications interface 
 * as provided by Javax.comm (aka gnu.io) classes. To get his working on 
 * windows, you'll need to download javax.comm package from Sun and install it 
 * according to the directions. In order for javax.comm to work on linux, you 
 * have to employ the RXTX package, available from: 
 * http://www.geeksville.com/~kevinh/linuxcomm.html
 * <p>
 *  @author Tom Cowin
 *  @author Chris Eagle
 *  - modifications
 *  @version $Revision: 1.14 $ 
 *  $Date: 2016/06/09 20:02:46 $
 */
public class SerialComm
{
    /**
     * Construct SerialComm object, assuming certain expected port identifiers 
     * based upon base level OS system is run upon. Open port, and initialize it 
     * according to expected parameters.  Create output and input streams. This 
     * constructor assumes common default settings.
     */
    public SerialComm()
    {
        this (1, 9600, 8, 1, "none");
    }

    /**
     * Construct SerialComm object, assuming certain expected port identifiers 
     * based upon base level OS system is run upon. Open port, and initialize it 
     * according to expected parameters.  Create output and input streams.
     * <p>
     * @param comPort integer indicator of preferred Serial Com Port - a '1' 
     * would indicate COM1 on Windows/DOS system, /dev/ttyS0 on Linux, 
     * /dev/ttya on Sun, etc. Currently coded to expect two com ports, 1 and 2.
     */
    public SerialComm (int comPort)
    {
        this (comPort, 9600, 8, 1, "none");
    }

    /**
     * Construct SerialComm object, assuming certain expected port identifiers 
     * based upon base level OS system is run upon. Open port, and initialize it 
     * according to expected parameters.  Create output and input streams.
     * <p>
     * @param comPort integer indicator of preferred Serial Com Port - a '1' 
     * would indicate COM1 on Windows/DOS system, /dev/ttyS0 on Linux, /dev/ttya 
     * on Sun, etc. Currently coded to expect two com ports, 1 and 2.
     * @param speed baudrate - needs to be a speed that COM port and app
     * can handle - i.e., one of 2400, 4800, 9600, 19200, 38400, 57600, etc.
     * @param databits actual number of databits - needs to be one of 5, 6 ,7 or 8.
     * @param stopbits actual number of stopbits - needs to be one of 1, 2 or 15(i.e., 1.5).
     * @param parity needs to be one of none, even, odd, mark or space.
     */
    public SerialComm (int comPort, int speed, int databits, int stopbits, String parity)
    {
        Enumeration portList;
        CommPortIdentifier portId;

        String OS = System.getProperty ("os.name");
        
        String primarySerialPortID = "COM1";
        String secondarySerialPortID = "COM2";

        if (OS.equalsIgnoreCase ("linux")) {
            primarySerialPortID = "/dev/ttyS0";
            secondarySerialPortID = "/dev/ttyS1";
        }
        else if (OS.startsWith("Solaris") || OS.startsWith("SunOS")) {
            primarySerialPortID = "/dev/ttya";
            secondarySerialPortID = "/dev/ttyb";
        }
        
        String preferredPort = primarySerialPortID;
        if (comPort == 2) {
            preferredPort = secondarySerialPortID;
        }
        
        try {
            FileWriter fw = new FileWriter("debugSerialIO.log");
            BufferedWriter bw = new BufferedWriter (fw);
            _logFile = new PrintWriter (bw);
        } 
        catch (Exception e) {
            e.printStackTrace();
        }
        
		//If you get this error message, d/l javax.comm and carefully install it. 
        //Include the comm.jar in your CLASSPATH.        
        portList = CommPortIdentifier.getPortIdentifiers();
        if ((portList == null) || (!portList.hasMoreElements())) {
            log("No COM ports found!");
            System.exit(0);
        }
        while (portList.hasMoreElements()) {
            portId = (CommPortIdentifier) portList.nextElement();
            if (portId.getPortType() == CommPortIdentifier.PORT_SERIAL) {
//                String portname = portId.getName();
                if (portId.getName().equals (preferredPort)) {
                    try {
                        _serialPort = (SerialPort) portId.open ("NOMADSSerialComm", 2000);
                       //name of app for ownership issues,time in ms
                       //to block waiting for open
                        if (_debug) {
                            log("serial port: " + preferredPort + " opened");
                        }
                    } 
                    catch (PortInUseException e) {
                        e.printStackTrace();
                    }
                    
                    try {
                        _outputStream = _serialPort.getOutputStream();
                        _inputStream = _serialPort.getInputStream();
                        
                        _bufferedReader = new BufferedReader (
                            new InputStreamReader (_inputStream));
                        
                        if (_outputStream == null || _inputStream == null ||
                            _bufferedReader == null) {
                            throw new IOException();
                        }
                    } 
                    catch (IOException e) {
                        e.printStackTrace();
                    }
                    
                    int db_param = SerialPort.DATABITS_8;
                    int sb_param = SerialPort.STOPBITS_1;
                    int par_param = SerialPort.PARITY_NONE;
                    
                    switch (databits) {
                        case 5:
                            db_param = SerialPort.DATABITS_5;
                            break;
                        case 6:
                            db_param = SerialPort.DATABITS_6;
                            break;
                        case 7:
                            db_param = SerialPort.DATABITS_7;
                            break;
                        case 8:
                            db_param = SerialPort.DATABITS_8;
                            break;
                    }
                    switch (stopbits) {
                        case 1:
                            sb_param = SerialPort.STOPBITS_1;
                            break;
                        case 2:
                            sb_param = SerialPort.STOPBITS_2;
                            break;
                        case 15:
                            sb_param = SerialPort.STOPBITS_1_5;
                            break;
                    }
                    
                    if (parity.equalsIgnoreCase ("none")) {
                        par_param = SerialPort.PARITY_NONE;
                    }
                    else if (parity.equalsIgnoreCase ("odd")) {
                        par_param = SerialPort.PARITY_ODD;
                    }
                    else if (parity.equalsIgnoreCase ("even")) {
                        par_param = SerialPort.PARITY_EVEN;
                    }
                    else if (parity.equalsIgnoreCase ("mark")) {
                        par_param = SerialPort.PARITY_MARK;
                    }
                    else if (parity.equalsIgnoreCase ("space")) {
                        par_param = SerialPort.PARITY_SPACE;
                    }
                    
                    try {
                        _serialPort.setSerialPortParams (/*9600,*/ speed,
                            db_param, sb_param, par_param);
                            
                        _serialPort.setFlowControlMode(SerialPort.FLOWCONTROL_NONE);
                    } 
                    catch (UnsupportedCommOperationException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
        if (_debug) {
            log("SerialComm constructor finished");
        }
    }

    /**
     * Set the flow control mode on the port. It expects either "none", "rtscts"
     * or "hardware", or "xonxoff" or "software".
     * <p>
     * @param flowControl string determines which type of flowcontrol to set on 
     *        port.
     */
    public void setFlowControl (String flowControl)
    {
        try {
            if (flowControl.equalsIgnoreCase("none")) {
                _serialPort.setFlowControlMode(SerialPort.FLOWCONTROL_NONE);
            }
            else if (flowControl.equalsIgnoreCase("RTSCTS") || 
                     flowControl.equalsIgnoreCase("hardware")) {
                _serialPort.setFlowControlMode(SerialPort.FLOWCONTROL_RTSCTS_IN | SerialPort.FLOWCONTROL_RTSCTS_OUT);
            }
            else if (flowControl.equalsIgnoreCase("XONXOFF") || 
                     flowControl.equalsIgnoreCase("software")) {
                _serialPort.setFlowControlMode(SerialPort.FLOWCONTROL_XONXOFF_IN | SerialPort.FLOWCONTROL_XONXOFF_OUT);
            }
        } 
        catch (UnsupportedCommOperationException e) {
            e.printStackTrace();
        }
    }

    /**
     * Sets the RTS line on or off
     *
     * @param setting  passing in true turns on RTS whereas false turns off RTS.
     */
    public void setRTS (boolean setting)
    {
        _serialPort.setRTS (setting);
    }

    /**
     * Returns the value of the RTS line
     *
     * @return true if RTS is currently on and false otherwise
     */
    public boolean getRTS()
    {
        return _serialPort.isRTS();
    }

    /**
     * Sets the DTR line on or off
     *
     * @param setting passing in true turns on DTR whereas false turns off DTR
     */
    public void setDTR (boolean setting)
    {
        _serialPort.setDTR (setting);
    }

    /**
     * Returns the value of the DTR line
     *
     * @return true if DTR is currently on and false otherwise
     */
    public boolean getDTR()
    {
        return _serialPort.isDTR();
    }

    /**
     * Send the argument character on the output stream, and indicate end of 
     * input with a carriage return.
     * <p>
     * @param buffer string to place onto output stream.
     */
    public void send (byte b)
    {
        try {
            _outputStream.write (b);
            if (_debug) {
                log("sending: " + b);
            }
        } 
        catch (IOException x) {
            x.printStackTrace();
        }
    }

    /**
     * Send the argument string on the output stream.
     * <p>
     * @param buffer string to place onto output stream.
     */
    public void send (String buffer)
    {
        try {
            _outputStream.write (buffer.getBytes());
            if (_debug) {
                log("sending: " + buffer);
            }
        } 
        catch (IOException x) {
            x.printStackTrace();
        }
    }

    /**
     * Send the argument string on the output stream, and indicate end of input 
     * with a carriage return.
     * <p>
     * @param buffer string to place onto output stream.
     */
    public void sendLine (String buffer)
    {
        try {
            _outputStream.write (buffer.getBytes());
            if (_debug) {
                log("sending: " + buffer);
            }
            _outputStream.write('\r');
        } 
        catch (IOException x) {
            x.printStackTrace();
        }
    }

    /**
     * Retrieves the next line of input from the stream, delimited by either a 
     * line feed or carriage return.  If the end of stream is reached, a null 
     * will be returned.
     * <p>
     * @return string next line of input.
     */
    public String receiveLine ()
    {
        String _returnBuffer = "";
        try {
            do {  } while (!_bufferedReader.ready());
            do{
                _returnBuffer = _bufferedReader.readLine();
                if (_debug) {
                    log("receiveLine: \"" + _returnBuffer + "\"");
                }
            }while (_returnBuffer.length() < 1);
            
            return (_returnBuffer.trim());
        } 
        catch (IOException e) {
            e.printStackTrace();
        }
        
        return("");
    }

    /**
     * Retrieves the next line of input from the stream, delimited by either a 
     * line feed or carriage return, and compares it to the provided input 
     * string to see if the two strings begin in same manner.  Response is 
     * boolean - true if the strings begin the same, false otherwise.
     * <p>
     * @param input string that is to be compared with string read in from line.
     * @return boolean indicating whether or not strings start the same.
     */
    public boolean matchSubstring (String input)
    {
        String _returnBuffer = "";
        try {
            do { } while (!_bufferedReader.ready());
            do {
                _returnBuffer = _bufferedReader.readLine();
                if (_debug) {
                    log("receiveLine expected got: \"" + _returnBuffer + "\"" 
                        + ", expected: " + input);
                }
            }while (_returnBuffer.length() < 1);
            
            return (_returnBuffer.trim().startsWith(input));
        } 
        catch (IOException e) {
            e.printStackTrace();
        }
        
        return(false);
    }


    /**
     * Read a continuous block or 'blob' of bytes from the input stream. 
     * Quantity as specified as the first argument.
     * <p>
     * @param integer number of bytes expected to comprise the blob to be read 
     *                from inout stream
     * @return char[] that contains the blob of characters.
     */
    public byte[] receiveBlob (int numbytes) 
    {
        byte[] blob = new byte[numbytes];
        try {
            for(int i=0;i<numbytes;i++) {
                blob[i] = (byte)_bufferedReader.read();
                if (_debug) {
                    log("got byte " + i + " in receiveBlob");
                }
            }
        } 
        catch (IOException e) {
            e.printStackTrace();
        }
        
        return(blob);
    }


    /**
     * Provides a local log facility in order to place error or informational 
     * debugging into a log file and also place it on stdout for operator.
     * <p>
     * @param string to print out to log file and stdout.
     */
    public void log (String out)
    {
        try{
            System.out.println(out);
            _logFile.println(out);
        } 
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    
    public InputStream _inputStream;
    public BufferedReader _bufferedReader;

    private SerialPort _serialPort = null;
    private static final boolean _debug = false;
    private OutputStream _outputStream;
    private PrintWriter _logFile;
}
