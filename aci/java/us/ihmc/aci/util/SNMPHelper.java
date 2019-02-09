package us.ihmc.aci.util;

import snmp.*;

import java.net.InetAddress;

/**
 * SNMPHelper
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Jul 27, 2004 at 11:33:58 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class SNMPHelper
{
    public SNMPHelper (String routerIP)
            throws Exception
    {
        _ipAddress = routerIP;
        initialize();
    }

    public SNMPHelper (String ipAddress, String community)
            throws Exception
    {
        _community_name = community;
        _ipAddress = ipAddress;
        initialize();
    }

    public void addTrapInterruption (SNMPTrapListener listener)
            throws Exception
    {
        if (_trapIntr == null) {
            _trapIntr = new SNMPv1TrapListenerInterface();
            _trapIntr.addTrapListener (listener);
            _trapIntr.startReceiving();
            _trapThread = new Thread (_trapIntr);
            _trapThread.start();
        }
        else {
            _trapIntr = new SNMPv1TrapListenerInterface();
        }
    }

    public void removeTrapInterruption (SNMPTrapListener listener)
            throws Exception
    {
        if (_trapIntr != null) {
            _trapIntr.removeTrapListener(listener);
        }
    }

    //--------------------------------------------------------------------------
    // basic OID's
    public String getSystemDescription ()
            throws Exception
    {
        return (getSNMPOctetStringAsString (SYSTEM_DESC_OID));
    }

    public String getSystemIdentifier ()
            throws Exception
    {
        return (getSNMPObjectIdentifierAsString (SYSTEM_OBJECT_ID_OID));
    }

    public String getSystemUptime ()
            throws Exception
    {
        return (getSNMPTimeTicksAsString (SYSTEM_UPTIME_OID));
    }

    //--------------------------------------------------------------------------
    public SNMPObject getSNMPValue (String oid)
        throws Exception
    {
        SNMPVarBindList vars = _comIntr.getMIBEntry (oid);
        SNMPSequence var = (SNMPSequence) vars.getSNMPObjectAt (0);
        return var.getSNMPObjectAt (1);
    }

    public SNMPObject[] getSNMPSequence (String oid)
            throws Exception
    {
        SNMPVarBindList vars = _comIntr.getMIBEntry (oid);
        SNMPObject objList[] = new SNMPObject[vars.size()];
        for (int i=0; i<vars.size(); i++) {
            objList[i] = vars.getSNMPObjectAt(i);
        }
        return objList;
    }

    public String getSNMPOctetStringAsString (String oid)
        throws Exception
    {
        SNMPOctetString value = (SNMPOctetString) getSNMPValue (oid);
        return value.toString();
    }

    public String getSNMPObjectIdentifierAsString (String oid)
        throws Exception
    {
        SNMPObjectIdentifier value = (SNMPObjectIdentifier) getSNMPValue (oid);
        return value.toString();
    }

    public String getSNMPTimeTicksAsString (String oid)
        throws Exception
    {
        SNMPTimeTicks value = (SNMPTimeTicks) getSNMPValue (oid);
        return value.toString();
    }

    public String getSNMPIntegerAsString (String oid)
        throws Exception
    {
        SNMPInteger value = (SNMPInteger) getSNMPValue (oid);
        return value.toString();
    }

    //-----------------------------------------------------------------------------
    private void initialize()
            throws Exception
    {
        InetAddress routerAddr = InetAddress.getByName (_ipAddress);
        _comIntr = new SNMPv1CommunicationInterface (0, routerAddr, _community_name);
        debugMsg (_comIntr.toString());
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[SNMPHelper] " + msg);
        }
    }

    private boolean _debug = true;
    private String _community_name = "public";
    private String _ipAddress = null;
    private Thread _trapThread = null;
    private SNMPv1CommunicationInterface _comIntr;
    private SNMPv1TrapListenerInterface _trapIntr;

    private static final String MIB_OID =               "1.3.6.1.2.1";
    private static final String SYSTEM_OID =            "1.3.6.1.2.1.1.0";
    private static final String SYSTEM_DESC_OID =       "1.3.6.1.2.1.1.1.0";
    private static final String SYSTEM_OBJECT_ID_OID =  "1.3.6.1.2.1.1.2.0";
    private static final String SYSTEM_UPTIME_OID =     "1.3.6.1.2.1.1.3.0";
    private static final String INTERFACES_OID =        "1.3.6.1.2.1.2";
    private static final String INTERFACES_NUMBER_OID = "1.3.6.1.2.1.2.1.0";
}
