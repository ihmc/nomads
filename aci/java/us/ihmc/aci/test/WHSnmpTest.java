package us.ihmc.aci.test;

import us.ihmc.aci.util.SNMPWalker;
import uk.co.westhawk.snmp.stack.varbind;

import java.net.InetAddress;
import java.util.StringTokenizer;
import java.util.Vector;

/**
 * WHSnmpTest
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Aug 2, 2004 at 1:43:49 AM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class WHSnmpTest
{
    public static void main(String[] args)
    {
        if (args.length < 1) {
            getInterfaceAddress();
        }
        else {
            _nodeGatewayIP = args[0];
        }


        try {
            SNMPWalker snmpWalker = new SNMPWalker (_nodeGatewayIP, "public");
            /*
            System.out.println("--------------------------------------------");
            System.out.println("System info - noteGatewayIP: " + _nodeGatewayIP);
            System.out.println("--------------------------------------------");
            varbind[] testArray = snmpWalker.getMIBArray("1.3.6.1.2.1.1");
            System.out.println("Size of the testArray: " + testArray.length);
            for (int i=0; i<testArray.length; i++) {
                varbind vbind = testArray[i];
                System.out.println(vbind.toString());
            }
            */

            boolean tagtype = true;
            while (true) {
                Vector vinfo = new Vector();
                varbind[] nbr = snmpWalker.getMIBArray (_nbrCost.toString());
                if (nbr != null) {
                    if (tagtype) {
                        System.out.println("--------------------------------------------");
                        tagtype = false;
                    }
                    else {
                        System.out.println("- - - - - - - - - - - - - - - - - - - - - -");
                        tagtype = true;
                    }
                    for (int i=0; i<nbr.length; i++) {
                        varbind vbind = nbr[i];
                        String sinfo = getPair (vbind.toString());
                        if (!vinfo.contains(sinfo)) {
                            vinfo.addElement(sinfo);
                        }


                        /*
                        System.out.println(vbind.toString());
                        */


                        String entry = vbind.toString();
                        if (entry.indexOf(".1:")>0) {
                            //System.out.println(entry);
                            String prefix = "1.3.6.1.4.1.14.5.15.3.10.1.4.";
                            String part1 = entry.substring(entry.indexOf(prefix)+prefix.length(),entry.indexOf(".1:"));
                            String part2 = entry.substring(entry.indexOf(".1:")+3);
                            System.out.println(part1 + "\t->\t" + part2);
                        }
                        

                    }
                }
                else {
                    System.out.println("SNMP returned a NULL routing table");
                }
                Thread.sleep(2500);

                /*
                System.out.println("Size of the nbr: " + nbr.length);
                for (int i=0; i<nbr.length; i++) {
                    varbind vbind = nbr[i];
                    String sinfo = getPair (vbind.toString());
                    if (!vinfo.contains(sinfo)) {
                        vinfo.addElement(sinfo);
                    }
                    System.out.println(vbind.toString());
                }

                for (int i=0; i<vinfo.size(); i++) {
                    System.out.print ((String) vinfo.elementAt(i) + "\n");
                }
                System.out.println("\n");
                Thread.sleep(2500);
                */

                /*
                for (int i=0; i<nbr.length; i++) {
                    varbind vbind = nbr[i];
                    String sinfo = getPair (vbind.toString());
                    if (!vinfo.contains(sinfo)) {
                        vinfo.addElement(sinfo);
                    }
                    System.out.println(vbind.toString());
                }

                for (int i=0; i<vinfo.size(); i++) {
                    System.out.print ((String) vinfo.elementAt(i) + "\n");
                }
                System.out.println("\n");
                Thread.sleep(2500);
                */
            }



            /*
            System.out.println("--------------------------------------------");
            System.out.println("GPS info (Before)");
            System.out.println("--------------------------------------------");
            varbind[] gps = snmpWalker.getMIBArray (_gpsPosition.toString());
            System.out.println("Size of the nbr: " + gps.length);
            for (int i=0; i<gps.length; i++) {
                varbind vbind = gps[i];
                System.out.println(vbind.toString());
            }

            /*
            System.out.println("--------------------------------------------");
            System.out.println("Setting First time");
            System.out.println("--------------------------------------------");
            String gpsPos = "32.37043333 -84.80486667 0 45 60 70 " + System.currentTimeMillis();
            System.out.println("Set: " + gpsPos);
            try {
                snmpWalker.setMIBValue(_gpsPosition + ".0", gpsPos);
            }
            catch (Exception e) {
                e.printStackTrace();
            }

            System.out.println("--------------------------------------------");
            System.out.println("GPS info After First setting");
            System.out.println("--------------------------------------------");
            gps = snmpWalker.getMIBArray (_gpsPosition.toString());
            System.out.println("Size of the nbr: " + gps.length);
            for (int i=0; i<gps.length; i++) {
                varbind vbind = gps[i];
                System.out.println(vbind.toString());
            }

            /*
            System.out.println("--------------------------------------------");
            System.out.println("Setting Second time");
            System.out.println("--------------------------------------------");
            System.out.println("Set: " + gpsPos);
            gpsPos = "32.37043333 -84.80486667 0.0 45 60 70 " + System.currentTimeMillis();
            try {
                snmpWalker.setMIBValue(_gpsPosition + ".0", gpsPos);
            }
            catch (Exception e) {
                e.printStackTrace();
            }

            System.out.println("--------------------------------------------");
            System.out.println("GPS info After second setting");
            System.out.println("--------------------------------------------");
            gps = snmpWalker.getMIBArray (_gpsPosition.toString());
            System.out.println("Size of the nbr: " + gps.length);
            for (int i=0; i<gps.length; i++) {
                varbind vbind = gps[i];
                System.out.println(vbind.toString());
            }

            /*
            System.out.println("--------------------------------------------");
            System.out.println("Setting Third time");
            System.out.println("--------------------------------------------");
            gpsPos = "32.37043333 -84.80486667 0.0 0.0 0.0 0.0 " + System.currentTimeMillis();
            System.out.println("Set: " + gpsPos);
            try {
                snmpWalker.setMIBValue(_gpsPosition + ".0", gpsPos);
            }
            catch (Exception e) {
                e.printStackTrace();
            }

            System.out.println("--------------------------------------------");
            System.out.println("GPS info after third setting)");
            System.out.println("--------------------------------------------");
            gps = snmpWalker.getMIBArray (_gpsPosition.toString());
            System.out.println("Size of the nbr: " + gps.length);
            for (int i=0; i<gps.length; i++) {
                varbind vbind = gps[i];
                System.out.println(vbind.toString());
            }
            */
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static String getPair (String oidlist)
    {
        String pair = "";
        try {
            StringTokenizer st = new StringTokenizer(oidlist,":");
            String soid = st.nextToken();
            StringTokenizer st2 = new StringTokenizer (soid,".");
            for (int i=0; i<13; i++) {
                st2.nextToken();
            }
            pair = pair + st2.nextToken() + " -> " + st2.nextToken();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return pair;
    }

    private static void getInterfaceAddress ()
    {
        _nodeGatewayIP = "192.168.11.1";
        _nodeAdHocIP = "10.0.0.11";
        debugMsg ("looking up interfaces");
        try {
            InetAddress[] allInets = InetAddress.getAllByName("localhost");
            InetAddress add = InetAddress.getByName (InetAddress.getLocalHost().getHostAddress());
            System.out.println(add.getHostAddress());
            if (!add.isLoopbackAddress()) {
                String inetAdd = add.getHostAddress();
                debugMsg ("Setting local Address to (" + inetAdd + ")");
                StringTokenizer st = new StringTokenizer (inetAdd, ".");
                if (st.countTokens() == 4) {
                    String oct1 = st.nextToken();
                    String oct2 = st.nextToken();
                    String oct3 = st.nextToken();
                    String oct4 = st.nextToken();
                    _nodeGatewayIP = oct1 + "." + oct2 + "." + oct3 + ".1";
                    _nodeAdHocIP = "10.0.0." + oct3;
                    debugMsg ("Setting nodeGatewayIP to (" + _nodeGatewayIP + ")");
                    debugMsg ("Setting nodeAdHocIP to (" + _nodeAdHocIP + ")");
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[WHSnmpTest] " + msg);
        }
    }

    private static long _counter = 0;

    private static String _nodeGatewayIP = "192.168.11.1";
    private static String _nodeAdHocIP = "10.0.0.1";
    private static boolean _debug = true;
    private static final String _nbrCost = "1.3.6.1.4.1.14.5.15.3.10.1.4";
    private static final String _gpsPosition = "1.3.6.1.4.1.14.5.15.3.4";
}
