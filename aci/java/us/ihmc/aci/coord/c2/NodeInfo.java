package us.ihmc.aci.coord.c2;

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
public class NodeInfo
{
    public int cpuClockFreq;
    public int numberOfCPUs;
    public int cpuUtilizationAci;
    public int cpuUtilizationOther;
    public long memoryUtilization;
    public long totalMemory;
    public long freeDisk;
    public int tcpConnCount;
    public int networkLinkSpeed;
    public int inboundNetworkUtilization;
    public int outboundNetworkUtilization;
    public float gpsLatitude;
    public float gpsLongitude;
    public float gpsAltitude;
    public float nodeSpeed;

    public NodeInfo (String nodeUUID)
    {
        _nodeUUID = nodeUUID;
    }

    public String getNodeUUID()
    {
        return _nodeUUID;
    }

    public void parseNodeMonInfo (byte[] buf)
    {
        int pos = 0;
        int bufLen = buf.length;
        boolean bDone = false;
        while ((!bDone) && (pos < bufLen)) {
            switch (buf[pos]) {
                case NodeMonitorDataTypes.NMDT_End:
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
                    myAssert (buf[pos+1] == 4);
                    numberOfCPUs = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_CPU_Utilization_ACI:
                    myAssert (buf[pos+1] == 4);
                    cpuUtilizationAci = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
                    break;

                case NodeMonitorDataTypes.NMDT_CPU_Utilization_Other:
                    myAssert (buf[pos+1] == 4);
                    cpuUtilizationOther = (int) ByteConverter.from4BytesToUnsignedInt (buf, pos+2);
                    pos += 6;
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

                default:
                    System.out.println ("Received unknown data type " + buf[pos]);
                    bDone = true;
            }
        }
    }

    public void display()
    {
        System.out.println ("CPU Clock Frequency = " + cpuClockFreq);
        System.out.println ("Number of CPUs = " + numberOfCPUs);
        System.out.println ("CPU Utilization ACI = " + cpuUtilizationAci);
        System.out.println ("CPU Utilization Other = " + cpuUtilizationOther);
        System.out.println ("Memory Utilizaiton = " + memoryUtilization);
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
