package us.ihmc.aci.util;

import uk.co.westhawk.snmp.stack.*;
import uk.co.westhawk.snmp.pdu.BlockPdu;
import uk.co.westhawk.snmp.pdu.OneSetPdu;


import java.util.Vector;

/**
 * SNMPWalker
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Aug 2, 2004 at 1:10:24 AM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class SNMPWalker
{
    public SNMPWalker (String host)
            throws Exception
    {
        init (host, _defaultPort, _defaultCommunity);
    }

    public SNMPWalker (String host, String community)
            throws Exception
    {
        init (host, _defaultPort, community);
    }

    public SNMPWalker (String host, int port, String community)
            throws Exception
    {
        init (host, port, community);
    }

    //-----------------------------------------------------------------------
    public void setMIBValue (String oid, String svalue)
            throws Exception
    {
        try {
            AsnOctets ansVal = new AsnOctets (svalue);
            OneSetPdu oneSetPdu = new OneSetPdu(_snmpContextPool, oid, ansVal);
            oneSetPdu.setRetryIntervals(_retry_array);
            oneSetPdu.send();
            varbind[] last_result = oneSetPdu.getResponseVarbinds();
            debugMsg(oneSetPdu.getErrorStatusString());
        }
        catch (PduException exc) {
            if (_debug) {
                System.err.println("SNMPWalker.setMIBValue: PduException: " + exc.getMessage());
                exc.printStackTrace();
            }
            throw exc;
        }
        catch (java.io.IOException exc) {
            if (_debug) {
                System.err.println("SNMPWalker.setMIBValue: IOException: " + exc.getMessage());
                exc.printStackTrace();
            }
            throw exc;
        }
    }


    //-----------------------------------------------------------------------
    public varbind[] getMIBArray (String baseOID)
    {
        AsnObjectId base_oid = new AsnObjectId(baseOID);
	    AsnObjectId new_oid = base_oid;
        Vector values = new Vector();
        varbind[] result = null;
        varbind last_result = null;

        try {
            while (true) {
                BlockPdu bpdu = new BlockPdu(_snmpContextPool);
                bpdu.setRetryIntervals(_retry_array);
                bpdu.setPduType(BlockPdu.GETNEXT);
                bpdu.addOid(new_oid.toString());
                last_result = bpdu.getResponseVariableBinding();
                if (AsnObject.debug > 5) {
                    System.out.println("SNMPWalker.getMibArray: got " + last_result);
                }
                if (last_result == null) {
                    debugMsg ("Last result is NULL");
                    break;
                }
                new_oid = last_result.getOid();
                if (!new_oid.startsWith(base_oid)) {
                    //debugMsg ("Wrong base. Base");
                    //debugMsg (new_oid + " should start with " + base_oid);
                    break;
                }
                values.addElement(last_result);
            }
        }
        catch (PduException exc) {
            if (_debug) {
                System.err.println("SNMPWalker.getMibArray: PduException: " + exc.getMessage());
                exc.printStackTrace();
            }
            return result;
        }
        catch (java.io.IOException exc) {
            if (_debug) {
                System.err.println("SNMPWalker.getMibArray: IOException: " + exc.getMessage());
                exc.printStackTrace();
            }
            return result;
        }

        result = new varbind[values.size()];
        values.copyInto(result);
        return result;
    }

    /////////////////////////////////////////////////////////////////////////
    private void init (String host, int port, String community)
            throws Exception
    {
        debugMsg ("Setting up context to (" + host + ":" + port + " [" + community+ "])");
        _snmpContextPool = new SnmpContextPool (host, port);
        _snmpContextPool.setCommunity(community);
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println ("[SNMPWalker] " + msg);
        }
    }

    private boolean _debug = false;
    private int _retry_array[] = {1000, 1500, 2000};     //1 sec, then 1.5 secs, then 2 secs.
    private String _defaultCommunity = "public";
    private int _defaultPort = 161;
    private SnmpContextPool _snmpContextPool = null;
}
