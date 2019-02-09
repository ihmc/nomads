package us.ihmc.aci.kernel;

/**
 * NodeInfoDataTypes defines the constants that are used to exchange
 * information about nodes
 * 
 * The entries here must match those in NodeIndoDataTypes.h, in aci\cpp\kernel
 * 
 * See NodeInfoDataTypes.h for additional comments about the meaning of these
 * data types
 *
 * @author nsuri
 */
public class NodeInfoDataTypes
{
    public static final byte NIDT_End = 0x00;

    public static final byte NIDT_NodeProperties = 0x01;             // Comma-separated properties about the node
                                                                     // Currently used to identify nodes as client or server

    public static final byte NIDT_ActiveServices = 0x02;
}
