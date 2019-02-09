package us.ihmc.aci.netviewer.proxy;

import us.ihmc.aci.nodemon.custom.DisServiceContent;
import us.ihmc.aci.nodemon.custom.MocketsContent;
import us.ihmc.aci.nodemon.data.node.Node;
import us.ihmc.aci.nodemon.data.node.core.Neighbor;
import us.ihmc.aci.nodemon.data.node.info.CPUInfo;
import us.ihmc.aci.nodemon.data.node.info.NetworkInfo;
import us.ihmc.aci.nodemon.data.node.info.OperatingSystem;

/**
 * Contains static methods that help building a node info and stats summary
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
class SummaryBuilder
{
    /**
     * Appends at the beginning and at the end of @s the tag bold
     * @param s string that needs to be bold
     * @return the string recognized as bold
     */
    private static String buildBoldString (String s)
    {
        return "<bold>" + s +"</bold>";
    }

    /**
     * Build a combination of @s1 and @s2 where @s1 will be bold
     * @param s1 first string that will be bold
     * @param s2 second string
     * @return a combination of @s1 and @s2 where @s1 will be bold
     */
    private static String buildBoldGeneral (String s1, String s2)
    {
        return new StringBuilder()
                .append (buildBoldString (s1))
                .append (s2)
                .append ("\n\n")
                .toString();
    }

    /**
     * Build a combination of @s1 and @s2
     * @param s1 first string
     * @param s2 second string
     * @return a combination of @s1 and @s2
     */
    private static String buildGeneral (String s1, String s2)
    {
        return new StringBuilder()
                .append (s1)
                .append (" ")
                .append (s2)
                .append ("\n")
                .toString();
    }

    /**
     * Build a combination of @s and @l
     * @param s string value
     * @param l long value
     * @return a combination of @s and @l
     */
    private static String buildGeneral (String s, long l)
    {
        return new StringBuilder()
                .append (s)
                .append (" ")
                .append (l)
                .append ("\n")
                .toString();
    }

    /**
     * Build a combination of @s and @d
     * @param s string value
     * @param d double value
     * @return a combination of @s and @l
     */
    private static String buildGeneral (String s, double d)
    {
        return new StringBuilder()
                .append (s)
                .append (" ")
                .append (d)
                .append ("\n")
                .toString();
    }

    /**
     * Build a combination of @s and @i
     * @param s string value
     * @param i int value
     * @return a combination of @s and @i
     */
    private static String buildGeneral (String s, int i)
    {
        return new StringBuilder()
                .append (s)
                .append(" ")
                .append (i)
                .append ("\n")
                .toString();
    }

    /**
     * Builds the node id summary
     * @param node node instance
     * @return the node id summary line
     */
    static String buildNodeId (Node node)
    {
        if (node == null) {
            return "";
        }

        return buildBoldGeneral("NODE ID: ", node.getId());
    }

    /**
     * Builds the node name summary
     * @param node node instance
     * @return the node name summary line
     */
    static String buildNodeName (Node node)
    {
        if (node == null) {
            return "";
        }

        return buildBoldGeneral("NODE NAME: ", node.getNodeCore().getName());
    }

    /**
     * Builds the node neighbors summary
     * @param node node instance
     * @return the node neighbors summary
     */
    static String buildNeighbors (Node node)
    {
        if ((node == null) || (node.getNodeCore().getNeighbors() == null) || (node.getNodeCore().getNeighbors().isEmpty())) {
            return "";
        }

        StringBuilder sb = new StringBuilder();

        sb.append (buildBoldString ("NEIGHBORS: "));
        sb.append ("\n");
        for (Neighbor neighbor : node.getNodeCore().getNeighbors()) {
            sb.append (" - ");
            sb.append (neighbor.getNodeId());
            sb.append ("\n");
        }
        sb.append ("\n");

        return sb.toString();
    }

    /**
     * Builds the node operating system summary
     * @param os operating system info instance
     * @return the node operating system summary
     */
    static String buildOperatingSystem (OperatingSystem os)
    {
        if (os == null) {
            return "";
        }

        StringBuilder sb = new StringBuilder();

        sb.append (buildBoldString ("OPERATING SYSTEM: "));
        sb.append (os.getName());
        sb.append (" ");
        sb.append (os.getArch());
        sb.append (" ");
        sb.append (os.getVersion());
        sb.append (" ");
        sb.append (os.getMachine());
        sb.append (" ");
        sb.append (os.getVendor());
        sb.append (" ");
        sb.append (os.getVendorVersion());
        if (os.getDescription() != null) {
            sb.append (" - ");
            sb.append (os.getDescription());
        }
        sb.append ("\n\n");

        return sb.toString();
    }

    /**
     * Builds the node CPUs info summary
     * @param cpuInfo CPUs info array
     * @return the node CPUs info summary
     */
    static String buildCPIInfo (CPUInfo[] cpuInfo)
    {
        if ((cpuInfo == null) || (cpuInfo.length == 0)) {
            return "";
        }

        StringBuilder sb = new StringBuilder();

        sb.append (buildBoldString ("CPU INFO: "));
        sb.append ("\n");
        for (CPUInfo singleCPU : cpuInfo) {
            sb.append (" - ");
            sb.append (singleCPU.getModel());
            sb.append (" ");
            sb.append (singleCPU.getVendor());
            sb.append (" ");
            int nCores = singleCPU.getTotalCores();
            sb.append (nCores);
            sb.append (nCores == 1 ? " core " : " cores ");
            sb.append (singleCPU.getFreq());
            sb.append (" MHz");
            sb.append (" \n");
        }
        sb.append ("\n");

        return sb.toString();
    }

    /**
     * Builds the node networks info summary
     * @param networkInfo networks info array
     * @return the node networks info summary
     */
    static String buildNetworkInfo (NetworkInfo[] networkInfo)
    {
        if ((networkInfo == null) || (networkInfo.length == 0)) {
            return "";
        }

        StringBuilder sb = new StringBuilder();

        sb.append (buildBoldString ("NETWORK INFO: "));
        sb.append ("\n");
        for (NetworkInfo singleIface : networkInfo) {
            String ifaceName = singleIface.getInterfaceName();
            if (ifaceName == null) {
                continue;
            }

            sb.append (buildBoldString (ifaceName));
            sb.append ("\t");

            sb.append ("HWaddr ");
            sb.append (singleIface.getMACAddress());
            sb.append ("\n");

            sb.append ("\t");
            sb.append ("inet addr: ");
            sb.append (singleIface.getIPAddress());
            sb.append ("\n");

            sb.append ("\t");
            sb.append ("Bcast: ");
            sb.append (singleIface.getBroadcast());
            sb.append ("\n");

            sb.append ("\t");
            sb.append ("Mask: ");
            sb.append (singleIface.getNetmask());
            sb.append ("\n");

            sb.append ("\t");
            sb.append ("MTU: ");
            sb.append (singleIface.getMTU());
            sb.append ("\n");

            if (singleIface.isPrimary()) {
                sb.append ("\t");
                sb.append ("Primary Interface");
                sb.append ("\n");
            }
            sb.append ("\n");
        }

        return sb.toString();
    }

    /**
     * Builds the summary info coming from the <code>DisServiceCustomProcess</code>
     * @param dsc disservice process content
     * @return the summary info coming from the <code>DisServiceCustomProcess</code>
     */
    static String buildDisServiceStats (DisServiceContent dsc)
    {
        if (dsc == null) {
            return "";
        }

        return new StringBuilder()
                .append (buildBoldString ("DISSERVICE STATS"))
                .append ("\n")
                .append (buildGeneral ("Data Messages Received:", dsc.getDataMessagesReceived()))
                .append (buildGeneral ("Data Bytes Received:", dsc.getDataBytesReceived()))
                .append (buildGeneral ("Data Fragments Received:", dsc.getDataFragmentsReceived()))
                .append (buildGeneral ("Data Fragment Bytes Received:", dsc.getDataFragmentBytesReceived()))
                .append (buildGeneral ("Missing Fragment Request Messages Sent:", dsc
                        .getMissingFragmentRequestMessagesSent()))
                .append (buildGeneral ("Missing Fragment Request Bytes Sent:", dsc.getMissingFragmentRequestBytesSent()))
                .append (buildGeneral ("Missing Fragment Request Messages Received:", dsc
                        .getMissingFragmentRequestMessagesReceived()))
                .append (buildGeneral ("Missing Fragment Request Bytes Received:", dsc.getMissingFragmentRequestBytesReceived()))
                .append (buildGeneral ("Data Cache Query Messages Sent:", dsc.getDataCacheQueryMessagesSent()))
                .append (buildGeneral ("Data Cache Query Bytes Sent:", dsc.getDataCacheQueryBytesSent()))
                .append (buildGeneral ("Data Cache Query Messages Received:", dsc.getDataCacheQueryMessagesReceived()))
                .append (buildGeneral ("Data Cache Query Bytes Received:", dsc.getDataCacheQueryBytesReceived()))
                .append (buildGeneral ("Topology State Messages Sent:", dsc.getTopologyStateMessagesSent()))
                .append (buildGeneral ("Topology State Bytes Sent:", dsc.getTopologyStateBytesSent()))
                .append (buildGeneral ("Topology State Messages Received:", dsc.getTopologyStateMessagesReceived()))
                .append (buildGeneral ("Topology State Bytes Received:", dsc.getTopologyStateBytesReceived()))
                .append (buildGeneral ("Keep Alive Messages Sent:", dsc.getKeepAliveMessagesSent()))
                .append (buildGeneral ("Keep Alive Messages Received:", dsc.getKeepAliveMessagesReceived()))
                .append (buildGeneral ("Query Messages Sent:", dsc.getQueryMessagesSent()))
                .append (buildGeneral ("Query Messages Received:", dsc.getQueryMessagesReceived()))
                .append (buildGeneral ("Query Hits Messages Sent:", dsc.getQueryHitsMessagesSent()))
                .append (buildGeneral ("Query Hits Messages Received:", dsc.getQueryHitsMessagesReceived()))
                .append ("\n")
                .toString();
    }

    /**
     * Builds the summary info coming from the <code>MocketsCustomProcess</code>
     * @param mc mockets process content
     * @return the summary info coming from the <code>MocketsCustomProcess</code>
     */
    static String buildMockets (MocketsContent mc)
    {
        if (mc == null) {
            return "";
        }

        return new StringBuilder()
                .append (buildBoldString ("MOCKETS STATS"))
                .append ("\n")
                .append (buildGeneral ("PID", mc.getPID()))
                .append (buildGeneral ("Identifier:", mc.getIdentifier()))
                .append (buildGeneral ("Local Address:", mc.getLocalAddr() + ":" + mc.getLocalPort()))
                .append (buildGeneral ("Remote Address:", mc.getRemoteAddr() + ":" + mc.getRemotePort()))
                .append (buildGeneral ("Last Contact Time:", mc.getLastContactTime()))
                .append (buildGeneral ("Sent Bytes:", mc.getSentBytes()))
                .append (buildGeneral ("Sent Packets", mc.getSentPackets()))
                .append (buildGeneral ("Retransmits:", mc.getRetransmits()))
                .append (buildGeneral ("Received Bytes:", mc.getReceivedBytes()))
                .append (buildGeneral ("Duplicated Discarded Packets:", mc.getDuplicatedDiscardedPackets()))
                .append (buildGeneral ("No Room Discarded Packets:", mc.getNoRoomDiscardedPackets()))
                .append (buildGeneral ("Reassembly Skipped Discarded Packets:", mc.getReassemblySkippedDiscardedPackets()))
                .append (buildGeneral ("Estimated RTT:", mc.getEstimatedRTT()))
                .append (buildGeneral ("Unacknowledged Data Size:", mc.getUnacknowledgedDataSize()))
                .append (buildGeneral ("Unacknowledged Queue Size:", mc.getUnacknowledgedQueueSize()))
                .append (buildGeneral ("Pending Data Size:", mc.getPendingDataSize()))
                .append (buildGeneral ("Pending Packet Queue Size:", mc.getPendingPacketQueueSize()))
                .append (buildGeneral ("Reliable Sequenced Data Size:", mc.getReliableSequencedDataSize()))
                .append (buildGeneral ("Reliable Sequenced Packet Queue Size:", mc.getReliableSequencedPacketQueueSize()))
                .append (buildGeneral ("Reliable Unsequenced Data Size:", mc.getReliableUnsequencedDataSize()))
                .append (buildGeneral ("Reliable Unsequenced Packet Queue Size:", mc.getReliableUnsequencedPacketQueueSize()))
                .toString();
    }
}
