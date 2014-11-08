/**
 * The FlowACKManagerTest class is a testcase for the 
 * FlowACKManager class.
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
class FlowACKManagerTest 
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
            
        FlowACKManager fam = new FlowACKManager (SequentialArithmetic.subtract(0, 1), true, false);
        System.out.println ("--------------------------------------------------");
        fam._dump (System.out);
        System.out.println ("--------------------------------------------------");
        
        fam.receivedPacket (0);
        System.out.println ("--------------------------------------------------");
        fam._dump (System.out);
        System.out.println ("--------------------------------------------------");

        fam.receivedPacket (1);
        fam.receivedPacket (2);
        fam.receivedPacket (3);
        System.out.println ("--------------------------------------------------");
        fam._dump (System.out);
        System.out.println ("--------------------------------------------------");

        fam.receivedPacket (2);
        fam.receivedPacket (1);
        System.out.println ("--------------------------------------------------");
        fam._dump (System.out);
        System.out.println ("--------------------------------------------------");

        fam.receivedPacket (10);
        System.out.println ("--------------------------------------------------");
        fam._dump (System.out);
        System.out.println ("--------------------------------------------------");
    }
}

/*
 * vim: et ts=4 sw=4
 */
