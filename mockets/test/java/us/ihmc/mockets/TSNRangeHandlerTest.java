/**
 * The TSNRangeHandlerTest class is a testcase for the 
 * TSNRangeHandler class.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import java.util.Random;

//public 
class TSNRangeHandlerTest 
//extends TestCase
{
    public static void main (String[] args) throws Exception
    {
        /* initialize logging */
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        //FileHandler fh = new FileHandler ("DataRecv%g.log");
        //fh.setLevel (Level.INFO);
        //fh.setFormatter (new SimpleFormatter());
        //logger.addHandler (fh);
        logger.setLevel (Level.WARNING);
            
        TSNRangeHandler rh = new TSNRangeHandler (true, false);
        System.out.println ("--------------------------------------------------");
        rh._dump (System.out);
        System.out.println ("--------------------------------------------------");
        
        rh.packetProcessed (0);
        System.out.println ("--------------------------------------------------");
        rh._dump (System.out);
        System.out.println ("--------------------------------------------------");

        rh.packetProcessed (1);
        rh.packetProcessed (2);
        rh.packetProcessed (3);
        System.out.println ("--------------------------------------------------");
        rh._dump (System.out);
        System.out.println ("--------------------------------------------------");

        rh.packetProcessed (2);
        rh.packetProcessed (1);
        System.out.println ("--------------------------------------------------");
        rh._dump (System.out);
        System.out.println ("--------------------------------------------------");

        rh.packetProcessed (10);
        System.out.println ("--------------------------------------------------");
        rh._dump (System.out);
        System.out.println ("--------------------------------------------------");
    }
}
/*
 * vim: et ts=4 sw=4
 */
