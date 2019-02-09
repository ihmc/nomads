package us.ihmc.aci.coord;

import java.io.Serializable;

import us.ihmc.aci.kernel.NodeInfoDataTypes;
import us.ihmc.nodeMon.NodeMonitorDataTypes;
import us.ihmc.util.ByteConverter;

/**
 *
 * NodeInfo.java
 *
 * Created on September 29, 2006, 12:06 AM
 *
 * @author  nsuri
 */
public class NodeInfo implements Serializable
{
    public int cpuClockFreq = -1;
    public int numberOfCPUs = -1;
    public int cpuUtilizationACI = -1;
    public int cpuUtilizationOther = -1;
    public long memoryUtilization = -1;
    public long totalMemory = -1;
    public long freeDisk = -1;
    public int tcpConnCount = -1;
    public int networkLinkSpeed = -1;
    public int inboundNetworkUtilization = -1;
    public int outboundNetworkUtilization = -1;
    public float gpsLatitude = -1;
    public float gpsLongitude = -1;
    public float gpsAltitude = -1;
    public float nodeSpeed = -1;
    public String properties = null;
    public long lastUpdateTime = -1;
    public int activeServices = -1;

    public NodeInfo (String nodeUUID)
    {
        _nodeUUID = nodeUUID;
    }

    public String getNodeUUID()
    {
        return _nodeUUID;
    }
    
    public long getTimeSinceLastUpdate()
    {
        return (System.currentTimeMillis() - lastUpdateTime);
    }

    public void parseNodeMonInfo (byte[] buf)
    {
        int pos = 0;
        int bufLen = buf.length;
        boolean bDone = false;
        while ((!bDone) && (pos < bufLen)) {
            switch (buf[pos]) {
                case NodeInfoDataTypes.NIDT_End:
                    // Reached the end of the buffer
                    pos++;
                    bDone = true;
                    break;

                case NodeMonitorDataTypes.NMDT_CPU_Clock_Freq:
                    myAssert (buf[pos+1] == 4);
                    cpuClockFreq = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_CPU_Number:
                    myAssert (buf[pos+1] == 1);
                    numberOfCPUs = (int) ByteConverter.from1ByteToUnsignedByte (buf, pos+2);
                    pos += 3;
                    break;

                case NodeMonitorDataTypes.NMDT_CPU_Utilization_ACI:
                    myAssert (buf[pos+1] == 1);
                    cpuUtilizationACI = (int) ByteConverter.from1ByteToUnsignedByte (buf, pos+2);
                    pos += 3;
                    break;

                case NodeMonitorDataTypes.NMDT_CPU_Utilization_Other:
                    myAssert (buf[pos+1] == 1);
                    cpuUtilizationOther = (int) ByteConverter.from1ByteToUnsignedByte (buf, pos+2);
                    pos += 3;
                    break;

                case NodeMonitorDataTypes.NMDT_Mem_Utilization:
                    myAssert (buf[pos+1] == 4);
                    memoryUtilization = ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_Total_Memory:
                    myAssert (buf[pos+1] == 4);
                    totalMemory = ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_Free_Disk:
                    myAssert (buf[pos+1] == 4);
                    freeDisk = ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_Established_TCP_Connections:
                    myAssert (buf[pos+1] == 4);
                    tcpConnCount = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_Network_Link_Speed:
                    myAssert (buf[pos+1] == 4);
                    networkLinkSpeed = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_Inbound_Network_Utilization:
                    myAssert (buf[pos+1] == 4);
                    inboundNetworkUtilization = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_Outbound_Network_Utilization:
                    myAssert (buf[pos+1] == 4);
                    outboundNetworkUtilization = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_GPS_Latitude:
                    myAssert (buf[pos+1] == 4);
                    gpsLatitude = 0.0f;     // Need to convert 4 bytes into a float
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_GPS_Longitude:
                    myAssert (buf[pos+1] == 4);
                    gpsLongitude = 0.0f;    // Need to convert 4 bytes into a float
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_GPS_Altitude:
                    myAssert (buf[pos+1] == 4);
                    gpsAltitude = 0.0f;     // Need to convert 4 bytes into a float
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_Node_Speed:
                    myAssert (buf[pos + 1] == 4);
                    nodeSpeed = 0.0f;       // Need to convert 4 bytes into a float
                    pos += 6;
                    break;

                case NodeInfoDataTypes.NIDT_NodeProperties:
                    int propStrLen = (int) buf[pos + 1];
                    properties = new String (buf, pos + 2, propStrLen-1); // propStrLen-1 to exclude the null terminator
                    pos += 2 + propStrLen;
                    break;

                case NodeInfoDataTypes.NIDT_ActiveServices:
                    myAssert (buf[pos + 1] == 4);
                    activeServices = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                default:
                    System.out.println ("Received unknown data type " + buf[pos]);
                    bDone = true;
            }
        }
        lastUpdateTime = System.currentTimeMillis();
    }

    public void display()
    {
        System.out.println ("CPU Clock Frequency = " + cpuClockFreq);
        System.out.println ("Number of CPUs = " + numberOfCPUs);
        System.out.println ("CPU Utilization of ACI Components = " + ((cpuUtilizationACI == -1) ? "N/A" : cpuUtilizationACI + ""));
        System.out.println ("CPU Utilization of Other Components = " + ((cpuUtilizationOther == -1) ? "N/A" : cpuUtilizationOther + ""));
        System.out.println ("Memory Utilization = " + memoryUtilization);
        System.out.println ("Total Memory = " + totalMemory);
        System.out.println ("Free Disk = " + freeDisk);
        System.out.println ("TCP Connection Count = " + tcpConnCount);
        System.out.println ("Network Link Speed = " + networkLinkSpeed);
        System.out.println ("Inbound Network Utilization = " + inboundNetworkUtilization);
        System.out.println ("Outbound Network Utilization = " + outboundNetworkUtilization);
        System.out.println ("GPS Latitude = " + gpsLatitude);
        System.out.println ("GPS Longitude = " + gpsLongitude);
        System.out.println ("GPS Altitude = " + gpsAltitude);
        System.out.println ("Node Speed = " + nodeSpeed);
        System.out.println ("Node Properties = " + properties);
        System.out.println ("Active Services = " + activeServices);
    }

    private void myAssert (boolean b)
    {
        if (!b) {
            System.out.println ("About to throw exception in myAssert");
            throw new RuntimeException ("myAssertion failed");
        }
    }

    protected String _nodeUUID;
}

