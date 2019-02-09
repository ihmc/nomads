package us.ihmc.aci.test;

import us.ihmc.aci.util.SNMPHelper;
import us.ihmc.aci.util.ByteArrayHandler;
import us.ihmc.aci.envMonitor.provider.arl.BBNAdHocSNMPProvider;
import snmp.*;

/**
 * BBNSnmpTest
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Jul 28, 2004 at 12:24:33 AM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class BBNSnmpTest implements SNMPTrapListener
{
    public static void main(String[] args)
    {
        if (args.length < 1) {
            System.out.println("Usage: SNMPTest <ipAddress>");
            System.exit(0);
        }

        try {
            BBNSnmpTest bbnTest = new BBNSnmpTest(args[0]);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public BBNSnmpTest (String routerIP)
        throws Exception
    {
        debugMsg ("Creating SNMPHelper..... to (" + routerIP + ")");
        _snmpHelper = new SNMPHelper (routerIP);
        //_snmpHelper.addTrapInterruption(this);

        System.out.println(_snmpHelper.getSystemDescription());

        /*
        debugMsg ("Number neighbors ---------------------------------------");
        int[] nlist = getListOfNeighbors();
        for (int i=0; i<nlist.length; i++) {
            debugMsg ("Node[" + i + "] " + nlist[i]);
        }
        debugMsg ("end of list");
        */
        /*
        debugMsg ("List of neighbors");
        int[] nlist = getListOfNeighbors();
        for (int i=0; i<nlist.length; i++) {
            debugMsg ("\t[Node" + i + "] " + nlist[i]);
        }
        debugMsg ("end of list....");
        */
   }

    public void processTrap(SNMPTrapPDU snmpTrapPDU)
    {
        debugMsg ("Got Trap");
    }


    private void debugMsg (String msg) {
        if (_debug) {
            System.out.println("[BBNSnmpTest] " + msg);
        }
    }

    //--------------------------------------------------------------------
    //BBN Specific
    public void getNumberOfNodes ()
    {
        try {
            SNMPObject[] varlist = _snmpHelper.getSNMPSequence(BBN_NUM_NODES_OID);

            for (int i=0; i<varlist.length; i++) {
                debugMsg("[" + i + "] " + varlist[i].getClass().getName());
            }

            for (int i=0; i<varlist.length; i++) {
                debugMsg("Handling element (" + i + ")");
                SNMPSequence snmpSeq = (SNMPSequence) varlist[i];
                debugMsg("\tsize: " + snmpSeq.size());
                for (int j=0; j<snmpSeq.size(); j++) {
                    debugMsg ("\t\t[" + i + "." + j + "] " + snmpSeq.getSNMPObjectAt(j).getClass().getName());
                    if (snmpSeq.getSNMPObjectAt(j) instanceof SNMPOctetString) {
                        SNMPOctetString OctVal = (SNMPOctetString) snmpSeq.getSNMPObjectAt(j);
                        debugMsg ("\t\t\t" + OctVal.toHexString());
                    }
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //--------------------------------------------------------------------
    //BBN Specific
    public void getEnergyTestOfAccess ()
    {
        try {
            SNMPObject[] varlist = _snmpHelper.getSNMPSequence(BBN_NODE_ENERGY_OID);

            /*debug starts here ---------------------  */
            for (int i=0; i<varlist.length; i++) {
                debugMsg("[" + i + "] " + varlist[i].getClass().getName());
            }

            for (int i=0; i<varlist.length; i++) {
                debugMsg("Handling element (" + i + ")");
                SNMPSequence snmpSeq = (SNMPSequence) varlist[i];
                debugMsg("\tsize: " + snmpSeq.size());
                for (int j=0; j<snmpSeq.size(); j++) {
                    debugMsg ("\t\t[" + i + "." + j + "] " + snmpSeq.getSNMPObjectAt(j).getClass().getName());
                    if (snmpSeq.getSNMPObjectAt(j) instanceof SNMPOctetString) {
                        SNMPOctetString OctVal = (SNMPOctetString) snmpSeq.getSNMPObjectAt(j);
                        debugMsg ("\t\t\t" + OctVal.toHexString());
                    }
                }
            }
            /*debug ends here ----------------------  */
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    public void getPathList ()
    {
        try {
            SNMPObject[] varlist = _snmpHelper.getSNMPSequence(BBN_PATH_LIST_OID);

            /*debug starts here ---------------------  */
            for (int i=0; i<varlist.length; i++) {
                debugMsg("[" + i + "] " + varlist[i].getClass().getName());
            }

            for (int i=0; i<varlist.length; i++) {
                debugMsg("Handling element (" + i + ")");
                SNMPSequence snmpSeq = (SNMPSequence) varlist[i];
                debugMsg("\tsize: " + snmpSeq.size());
                for (int j=0; j<snmpSeq.size(); j++) {
                    debugMsg ("\t\t[" + i + "." + j + "] " + snmpSeq.getSNMPObjectAt(j).getClass().getName());
                    if (snmpSeq.getSNMPObjectAt(j) instanceof SNMPOctetString) {
                        SNMPOctetString OctVal = (SNMPOctetString) snmpSeq.getSNMPObjectAt(j);
                        debugMsg ("\t\t\t" + OctVal.toString());
                        debugMsg ("\t\t\t" + OctVal.toHexString());
                        debugMsg ("-----");
                        getIntValuesFromByteArray (OctVal);
                    }
                }
            }
            /*debug ends here ----------------------  */
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void getIntValuesFromByteArray (SNMPOctetString OctVal)
    {
        byte[] binfo = (byte[]) OctVal.getValue();
        //for (int k=0; k<binfo.length; k++) {
        //    debugMsg ("binfo[" + k + "] " + binfo[k]);
        //}
        byte[] rawNodeID = new byte[4];
        if (binfo.length > 0) {
            int[] nodeList = new int[(int)(binfo.length/2)];
            debugMsg ("nodeList size: " + nodeList.length);
            for (int j2=0; j2<nodeList.length; j2++) {
                System.arraycopy(binfo, j2*2, rawNodeID, 2, 2);
                int nodeID = -1;
                //for (int k=0; k<rawNodeID.length; k++) {
                //    debugMsg ("rawNode[" + k + "] " + rawNodeID[k]);
                //}
                try {
                    nodeID = ByteArrayHandler.byteArrayToInt(rawNodeID, 0);
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
                debugMsg ("path: " + nodeID);
            }
        }
    }

    //--------------------------------------------------------------------
    //BBN Specific
    public void getForwardTableNumber ()
    {
        try {
            SNMPObject[] varlist = _snmpHelper.getSNMPSequence(BBN_FWRDTABLE_NUMBER);

            /*debug starts here ---------------------  */
            for (int i=0; i<varlist.length; i++) {
                debugMsg("[" + i + "] " + varlist[i].getClass().getName());
            }

            for (int i=0; i<varlist.length; i++) {
                debugMsg("Handling element (" + i + ")");
                SNMPSequence snmpSeq = (SNMPSequence) varlist[i];
                debugMsg("\tsize: " + snmpSeq.size());
                for (int j=0; j<snmpSeq.size(); j++) {
                    debugMsg ("\t\t[" + i + "." + j + "] " + snmpSeq.getSNMPObjectAt(j).getClass().getName());
                    if (snmpSeq.getSNMPObjectAt(j) instanceof SNMPOctetString) {
                        SNMPOctetString OctVal = (SNMPOctetString) snmpSeq.getSNMPObjectAt(j);
                        debugMsg ("\t\t\t" + OctVal.toHexString());
                    }
                }
            }
            /*debug ends here ----------------------  */
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void getPathCostTable()
    {
        try {
            SNMPObject[] varlist = _snmpHelper.getSNMPSequence(BBN_PATHCOST_TBL_OID);

            /*debug starts here ---------------------  */
            for (int i=0; i<varlist.length; i++) {
                debugMsg("[" + i + "] " + varlist[i].getClass().getName());
            }

            for (int i=0; i<varlist.length; i++) {
                debugMsg("Handling element (" + i + ")");
                SNMPSequence snmpSeq = (SNMPSequence) varlist[i];
                debugMsg("\tsize: " + snmpSeq.size());
                for (int j=0; j<snmpSeq.size(); j++) {
                    debugMsg ("\t\t[" + i + "." + j + "] " + snmpSeq.getSNMPObjectAt(j).getClass().getName());
                    if (snmpSeq.getSNMPObjectAt(j) instanceof SNMPOctetString) {
                        SNMPOctetString OctVal = (SNMPOctetString) snmpSeq.getSNMPObjectAt(j);
                        debugMsg ("\t\t\t" + OctVal.toHexString());
                    }
                }
            }
            /*debug ends here ----------------------  */
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    private void getPathListEntry()
    {
        try {
            SNMPObject[] varlist = _snmpHelper.getSNMPSequence(BBN_LIST_ENTRY_OID);

            /*debug starts here ---------------------  */
            for (int i=0; i<varlist.length; i++) {
                debugMsg("[" + i + "] " + varlist[i].getClass().getName());
            }

            for (int i=0; i<varlist.length; i++) {
                debugMsg("Handling element (" + i + ")");
                SNMPSequence snmpSeq = (SNMPSequence) varlist[i];
                debugMsg("\tsize: " + snmpSeq.size());
                for (int j=0; j<snmpSeq.size(); j++) {
                    debugMsg ("\t\t[" + i + "." + j + "] " + snmpSeq.getSNMPObjectAt(j).getClass().getName());
                    if (snmpSeq.getSNMPObjectAt(j) instanceof SNMPOctetString) {
                        SNMPOctetString OctVal = (SNMPOctetString) snmpSeq.getSNMPObjectAt(j);
                        debugMsg ("\t\t\t" + OctVal.toHexString());
                    }
                }
            }
            /*debug ends here ----------------------  */
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private int[] getListOfNeighbors ()
    {
        int[] nodeList = null;
        byte[] rawNodeID = new byte[4];
        try {
            SNMPObject[] varlist = _snmpHelper.getSNMPSequence(BBN_NEIGHOR_LIST_OID + ".0");
            debugMsg("List of neighbors as (SNMPObjects): " + varlist.length);
            for (int i=0; i<varlist.length; i++) {
                SNMPSequence entry = (SNMPSequence) varlist[i];
                SNMPObjectIdentifier objId = (SNMPObjectIdentifier) entry.getSNMPObjectAt(0);
                debugMsg("Object-Identifier: "+objId.toString());
                SNMPOctetString OctVal = (SNMPOctetString) entry.getSNMPObjectAt(1);
                debugMsg(OctVal.toHexString());
                byte[] binfo = (byte[]) OctVal.getValue();
                for (int k=0; k<binfo.length; k++) {
                    debugMsg ("binfo[" + k + "] " + binfo[k]);
                }
                if (binfo.length > 0) {
                    nodeList = new int[(int)(binfo.length/2)];
                    debugMsg ("nodeList size: " + nodeList.length);
                    for (int j=0; j<nodeList.length; j++) {
                        System.arraycopy(binfo, j*2, rawNodeID, 2, 2);
                        for (int k=0; k<rawNodeID.length; k++) {
                            debugMsg ("rawNode[" + k + "] " + rawNodeID[k]);
                        }
                        int nodeID = ByteArrayHandler.byteArrayToInt(rawNodeID, 0);
                        debugMsg ("nodeID: " + nodeID);
                        nodeList[j] = nodeID;
                    }
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return nodeList;
    }

    private boolean _debug = true;
    private SNMPHelper _snmpHelper;

    private static final String ENTERPRISE_OID =        "1.3.6.1.4.1";
    private static final String BBN_ENTERPRISE_OID =    "1.3.6.1.4.1.14";
    private static final String BBN_GATEWAY_OID =       "1.3.6.1.4.1.14.5";
    private static final String BBN_ROBOT_OID =         "1.3.6.1.4.1.14.5.15";
    private static final String BBN_INFOEXCHANGE_OID =  "1.3.6.1.4.1.14.5.15.3";
    private static final String BBN_NEIGHOR_LIST_OID =  "1.3.6.1.4.1.14.5.15.3.2";
    private static final String BBN_PATH_LIST_OID =     "1.3.6.1.4.1.14.5.15.3.4";
    private static final String BBN_NUM_NODES_OID =     "1.3.6.1.4.1.14.5.15.3.1";
    private static final String BBN_PATHCOST_TBL_OID =  "1.3.6.1.4.1.14.5.15.3.6";
    private static final String BBN_COST_ENTRY_OID =    "1.3.6.1.4.1.14.5.15.3.6.1";
    private static final String BBN_LIST_ENTRY_OID =    "1.3.6.1.4.1.14.5.15.3.6.1";    //same
    private static final String BBN_NODE_ENERGY_OID =   "1.3.6.1.4.1.14.5.15.3.6.1.7";
    private static final String BBN_FWRDTABLE_NUMBER =  "1.3.6.1.4.1.14.5.15.3.6.1.3";
}

