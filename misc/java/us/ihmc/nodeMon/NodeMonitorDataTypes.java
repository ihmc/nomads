package us.ihmc.nodeMon;

/**
 * NodeMonDataTypes defines the constants that are used to identify the specific
 * data types used by the Node Monitor
 * 
 * The entries here must match those in NodeMonitorDataTypes.h, in misc\cpp\nodeMon
 * 
 * See NodeMonitorDataTypes.h for additional comments about the meaning of these
 * data types
 */
public class NodeMonitorDataTypes
{
    public static final byte NMDT_End = 0x00;

    /* CPU-related data */
    public static final byte NMDT_CPU_Clock_Freq = 0x11;                 // CPU clock frequency - 4 byte integer
    public static final byte NMDT_CPU_Number = 0x12;                     // Number of installed CPUs - 1 byte integer
    public static final byte NMDT_CPU_Utilization_ACI = 0x13;            // The percentage of CPU used by ACI components (kernel + services) - 1 byte integer
    public static final byte NMDT_CPU_Utilization_Other = 0x14;          // The percentage of CPU used by other processes - 1 byte integer

    /* Memory-related data */
    public static final byte NMDT_Mem_Utilization = 0x20;                // The percentage of used memory - value is a 4 byte unsigned integer
    public static final byte NMDT_Total_Memory = 0x21;                   // The amount of installed RAM memory in MB - value is a 4 byte unsigned integer

    /* Storage-related data */
    public static final byte NMDT_Free_Disk = 0x30;                      // The amount of free disk space in MB - value is a 4 byte unsigned integer

    /* Network-related data */
    public static final byte NMDT_Established_TCP_Connections = 0x40;    // The number of established TCP connections - value is a 4 byte unsigned integer
    public static final byte NMDT_Network_Link_Speed = 0x41;             // The network link speed - value is a 4 byte unsigned integer
    public static final byte NMDT_Inbound_Network_Utilization = 0x42;    // The Inbound Network Utilization - value is a 4 byte unsigned integer
    public static final byte NMDT_Outbound_Network_Utilization = 0x43;   // The Outbound Network Utilization - value is a 4 byte unsigned integer

    /* Physical node data */
    public static final byte NMDT_GPS_Latitude = 0x50;                   // The GPS Latitude of the node - value is a 4 byte float
    public static final byte NMDT_GPS_Longitude = 0x51;                  // The GPS Longitude of the node - value is a 4 byte float
    public static final byte NMDT_GPS_Altitude = 0x52;                   // The GPS Altitude of the node - value is a 4 byte float
    public static final byte NMDT_Node_Speed = 0x53;                     // The node speed (m/s) - value is a 4 byte float
}
