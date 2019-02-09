package us.ihmc.aci.nodemon.sensors.custom;

import org.apache.log4j.Logger;
import org.snmp4j.*;
import org.snmp4j.mp.MPv1;
import org.snmp4j.mp.MPv2c;
import org.snmp4j.security.Priv3DES;
import org.snmp4j.security.SecurityProtocols;
import org.snmp4j.transport.AbstractTransportMapping;
import org.snmp4j.transport.DefaultTcpTransportMapping;
import org.snmp4j.transport.DefaultUdpTransportMapping;
import org.snmp4j.util.MultiThreadedMessageDispatcher;
import org.snmp4j.util.ThreadPool;
import us.ihmc.aci.ddam.DataType;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.data.process.SNMPContent;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;
import org.snmp4j.smi.*;

import java.io.IOException;
import java.util.Objects;
import java.util.Vector;

/**
 * SnmpPolledNodeSensor.java
 * <p/>
 * Class <code>SnmpPolledNodeSensor</code> provides Simple NETWORK Management Protocol (SNMP)
 * information exploiting the Java library snmp4j: http://snmp4j.org
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class SnmpPolledNodeSensor implements PolledNodeSensor, CommandResponder
{
    public SnmpPolledNodeSensor (NodeMon nodeMon, int port) throws IOException
    {
        _nodeMon = Objects.requireNonNull (nodeMon, "NodeMon can't be null");

        String address = "localhost/" + port;
        switch (_transportType) {
            case tcp:
                listen (new TcpAddress (address));
                break;
            case udp:
                listen (new UdpAddress (address));
                break;
            default:
                log.warn ("Unsupported transport type " + _transportType + " - using default one");
                listen (new UdpAddress (address));
        }

//        _target = createTarget (GenericAddress.parse (_routerIp + "/" + _routerPort));
    }

    @Override
    public synchronized void update (DataType type)
    {
        //log.debug ("Requested update for data: " + type);

//        if (!type.equals (DataType.PROCESS)) {
//            throw new IllegalArgumentException ("Sensor doesn't support type " + type);
//        }

        SNMPContent snmpContent = new SNMPContent();
        snmpContent.setSecurityModel (_latestEvent.getSecurityModel());
        snmpContent.setSecurityLevel (_latestEvent.getSecurityLevel());
        snmpContent.setMaxSizeResponsePDU (_latestEvent.getMaxSizeResponsePDU());
        snmpContent.setPduHandle (_latestEvent.getPduHandle());
        snmpContent.setStateReference (_latestEvent.getStateReference());
//        snmpContent.setPdu (event.getPDU());
        snmpContent.setMessageProcessingModel (_latestEvent.getMessageProcessingModel());
        snmpContent.setSecurityName (_latestEvent.getSecurityName());
        snmpContent.setProcessed (_latestEvent.isProcessed());
        snmpContent.setPeerAddress (_latestEvent.getPeerAddress());
        snmpContent.setTransportMapping (_latestEvent.getTransportMapping());
        snmpContent.setTmStateReference (_latestEvent.getTmStateReference());

        // TODO: add the variables to the SNMPContent object (look at method processPdu())
    }

    /**
     * Trap listener
     * @param address ip address where to listen to
     * @throws IOException if problems occur
     */
    private synchronized void listen (TransportIpAddress address) throws IOException
    {
        AbstractTransportMapping transport;
        if (address instanceof TcpAddress) {
            transport = new DefaultTcpTransportMapping ((TcpAddress) address);
        }
        else {
            transport = new DefaultUdpTransportMapping ((UdpAddress) address);
        }

        ThreadPool threadPool = ThreadPool.create ("DispatcherPool", 10);
        MessageDispatcher mDispatcher = new MultiThreadedMessageDispatcher (threadPool, new MessageDispatcherImpl());

        // Adding message processing models
        mDispatcher.addMessageProcessingModel (new MPv1());     // TODO: check what this is
        mDispatcher.addMessageProcessingModel (new MPv2c());    // TODO: check what this is

        // Adding all security protocols
        SecurityProtocols.getInstance().addDefaultProtocols();
        SecurityProtocols.getInstance().addPrivacyProtocol (new Priv3DES());

        // Creating Target
        CommunityTarget target = new CommunityTarget();
        target.setCommunity (new OctetString (_community));

        Snmp snmp = new Snmp (mDispatcher, transport);
        snmp.addCommandResponder (this);

        transport.listen();
        log.info ("SNMP sensor listening on " + address);

        try {
            this.wait();
        }
        catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    @Override
    public synchronized void processPdu (CommandResponderEvent event)
    {
        log.info ("Received PDU: " + event.toString());
        if (event.getPDU() == null) {
            log.warn ("No PDU in the received event. Ignoring it");
            return;
        }

        _latestEvent = event;
        PDU pdu = event.getPDU();
        Vector<? extends VariableBinding> varBinds = event.getPDU().getVariableBindings();
        if (varBinds != null && !varBinds.isEmpty()) {
            log.info ("Binding variables: ");
            for (VariableBinding vb : varBinds) {
                log.info (vb.toString());

                // TODO: add the variables to the SNMPContent object
            }
        }
    }

    private enum TransportType
    {
        tcp,
        udp
    }


//    private void getUpdates()
//    {
//        try {
//            TransportMapping transport;
//            switch (_transportType) {
//                case tcp:
//                    transport = new DefaultTcpTransportMapping();
//                    break;
//                case udp:
//                    transport = new DefaultUdpTransportMapping();
//                    break;
//                default:
//                    log.warn ("Unsupported transport type " + _transportType + " - using default one");
//                    transport = new DefaultUdpTransportMapping();
//            }
//
//            PDU pdu = new PDU();
//            pdu.add (new VariableBinding (new OID (_oid)));
//            pdu.setType (PDU.GET);
//
//            Snmp snmp = new Snmp (transport);
//
//            ResponseEvent response = snmp.getNodes (pdu, _target);
//            if (response == null) {
//                log.warn ("No response received from " + _routerIp);
//                snmp.close();
//                return;
//            }
//
//            if ((response.getResponse() != null) && (response.getResponse().getErrorStatusText() != null) &&
//                    response.getResponse().getErrorStatusText().equalsIgnoreCase ("success")) {
//                PDU pduResponse = response.getResponse();
//
//
////                String str = pduResponse.getVariableBindings().firstElement().toString();
////                if (str.contains ("=")) {
////                    str = str.substring (str.indexOf ("=") + 1, str.length());
////                }
//            }
//
//            snmp.close();
//        }
//        catch (IOException e) {
//            log.error ("Error while fetching SNMP statistics", e);
//        }
//    }


    private final NodeMon _nodeMon;
//    private final CommunityTarget _target;
    private static final TransportType _transportType = TransportType.udp;
    private static final String _community = "public";
    private CommandResponderEvent _latestEvent;


    private static final Logger log = Logger.getLogger (SnmpPolledNodeSensor.class);
}
