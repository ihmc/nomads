package mil.army.cpi.ba4.discoveryLib;


public abstract class AbstractNetwork {

    public String UUID;
    public int networkType;
    public String ip;
    public byte priority;
    public int gatewayURN;

    public AbstractNetwork(){}

    protected AbstractNetwork(AbstractNetwork network) {
        this.UUID = network.UUID;
        this.networkType = network.networkType;
        this.ip = network.ip;
        this.priority = network.priority;
        this.gatewayURN = network.gatewayURN;
    }

    public static String intToIp(int ip) {

        //for inverted ips
        return ((ip >> 24) & 0xFF) + "." + ((ip >> 16) & 0xFF)
                + "." + ((ip >> 8) & 0xFF) + "." + (ip & 0xFF);

//        return (ip & 0xFF) + "." + ((ip >> 8) & 0xFF)
//                + "." + ((ip >> 16) & 0xFF) + "." + ((ip >> 24) & 0xFF);
    }


    public enum RadioType {
        SRW,
        WNW,
        SINCGARS,
        EPLRS,
        HAVEQUICK_II,
        VHF_FM,
        BFT1_LBAND,
        BFT2_LBAND,
        TOC_LAN,
        VWP,
        GPS,
        CSS_VSAT,
        WIFI,
        FOURG_Cellular,
        WIN_T_INC2_SNE,
        WIN_T_INC2_POP,
        WIN_T_INC2_TCN,
        WIN_T_INC2_STT,
        HNH,
        SMART_T,
        DVB,
        TROJAN
    }
}
