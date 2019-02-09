package us.ihmc.aci.nodemon.sensors.info;


import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import org.hyperic.sigar.*;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.ddam.OperatingSystem;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.util.Config;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * SigarPolledNodeSensor.java
 * <p/>
 * Class <code>SigarPolledNodeSensor</code> provide system resources statistics provided by
 * the Java's "System Information Gatherer and Reporter" SIGAR library: https://github.com/hyperic/sigar
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class SigarPolledNodeSensor implements PolledNodeSensor
{
    public SigarPolledNodeSensor (NodeMon nodeMon)
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _sigar = new Sigar();
    }

    @Override
    public void update (DataType type)
    {
        switch (type) {
            case INFO:
                log.trace("Requested update for data: " + type);
                OperatingSystem os = getOperatingSystem(org.hyperic.sigar.OperatingSystem.getInstance());
                try {
                    List<CPU> cpus = getCPUs();
                    List<Network> nics = getNICs();

                    Info info = Info.newBuilder()
                            .setOs(os)
                            .addAllCpus(cpus)
                            .addAllNics(nics)
                            .setTimestamp(TimeUtil.getCurrentTime())
                            .build();

                    if (_nodeMon.getWorldState().getLocalNode() == null) {
                        log.warn("LocalNode not built yet, unable to update with new NodeInfo");
                        return;
                    }

                    Container c = ProtoUtils.toContainer(DataType.INFO, _nodeMon.getWorldState().getLocalNodeId(), info);
                    _nodeMon.updateData(_nodeMon.getWorldState().getLocalNodeId(), c);

                }
                catch (SigarException e) {
                    log.error("Error while fetching Sigar statistics", e);
                }

                break;
            default:
                //log.warn("Unsupported update request for this data type");
        }

    }

    private List<CPU> getCPUs () throws SigarException
    {
        CpuInfo[] cpuInfos = _sigar.getCpuInfoList();
        List<CPU> cpus = new ArrayList<>();

        //DISPLAY ONLY ONE cpu info for now to save size
        for (CpuInfo cpuInfo : cpuInfos) {

            CPU cpu = CPU.newBuilder()
                    .setVendor(cpuInfo.getVendor())
                    .setModel(cpuInfo.getModel())
                    .setFreq(cpuInfo.getMhz())
                    .setTotalCores(cpuInfo.getTotalCores())
                    .build();

            cpus.add(cpu);
            break;
        }

        return cpus;
    }

    public List<Network> getNICs () throws SigarException
    {
        String[] nicNames = _sigar.getNetInterfaceList();
        List<Network> nics = new ArrayList<>();

        String primaryMode = Config.getStringValue(Conf.GroupManager.NETIFS,
                DefaultValues.GroupManager.NETIFS);
        String primaryIP = primaryMode.split("/")[0];

        for (int i = 0; i < nicNames.length; i++) {

            NetInterfaceConfig netConfig = _sigar.getNetInterfaceConfig(nicNames[i]);

            //don't count loopback interface and double check if IP is 127.0.0.1, skip
            if (nicNames[i].equals("lo") || netConfig.getAddress().equals("127.0.0.1")) {
                continue;
            }

            Network nic = Network.newBuilder()
                    .setInterfaceName(netConfig.getName())
                    .setIpAddress(netConfig.getAddress())
                    .setMacAddress(netConfig.getHwaddr())
                    .setNetmask(netConfig.getNetmask())
                    .setBroadcast(netConfig.getBroadcast())
                    .setMtu(netConfig.getMtu())
                    .setIsPrimary(netConfig.getAddress().equals(primaryIP)) //true if matches primary ip
                    .build();

            nics.add(nic);
        }

        return nics;
    }

    private us.ihmc.aci.ddam.OperatingSystem getOperatingSystem (org.hyperic.sigar.OperatingSystem os)
    {
        return us.ihmc.aci.ddam.OperatingSystem.newBuilder()
                .setArch(os.getArch())
                .setDescription(os.getDescription())
                .setMachine(os.getMachine())
                .setName(os.getName())
                .setVendor(os.getVendor())
                .setVersion(os.getVersion())
                .setVendorVersion(os.getVendorVersion())
                .build();
    }

    private void test ()
    {
        try {

            org.hyperic.sigar.OperatingSystem os = org.hyperic.sigar.OperatingSystem.getInstance();
            log.debug("OS description: " + os.getDescription());
            log.debug("OS name: " + os.getName());
            log.debug("OS arch: " + os.getArch());
            log.debug("OS machine: " + os.getMachine());
            log.debug("OS version: " + os.getVersion());
            //log.debug("OS patch level: " + os.getPatchLevel()); //not significant
            //log.debug("OS vendor: " + os.getVendor());    // not significant
            log.debug("OS vendor version: " + os.getVendorVersion());

            CpuInfo[] cpuInfos = _sigar.getCpuInfoList();

            int i = 0;
            for (CpuInfo cpu : cpuInfos) {
                log.debug("===== CPU #" + (++i) + "=====");
                log.debug("Vendor: " + cpu.getVendor());
                log.debug("Model: " + cpu.getModel());
                log.debug("Mhz: " + cpu.getMhz());
                log.debug("Total cores: " + cpu.getTotalCores());
            }

            Cpu cpu = _sigar.getCpu();
            log.debug("CPU data. User: " + cpu.getUser() + " Sys: " + cpu.getSys());
            Mem mem = _sigar.getMem();
            log.debug("Mem data.\n Total: " + Utils.humanReadableByteCount(mem.getTotal(), true)
                    + "\n Used: " + Utils.humanReadableByteCount(mem.getUsed(), true)
                    + "\n Free: " + Utils.humanReadableByteCount(mem.getFree(), true));

            NetInterfaceConfig netInterfaceConfig = _sigar.getNetInterfaceConfig();

            log.debug("NETWORK data.\n Primary interface: " + netInterfaceConfig.getName()
                    + "\n IP address: " + netInterfaceConfig.getAddress()
                    + "\n MAC address: " + netInterfaceConfig.getHwaddr()
                    + "\n Netmask: " + netInterfaceConfig.getNetmask()
                    + "\n Broadcast: " + netInterfaceConfig.getBroadcast()
                    + "\n MTU: " + netInterfaceConfig.getMtu()
                    + "\n Destination: " + netInterfaceConfig.getDestination()
                    + "\n Description: " + netInterfaceConfig.getDescription());

            Tcp tcp = _sigar.getTcp();

            final String dnt = "    ";
            log.debug("=== TCP connections data ===");
            log.debug(dnt + tcp.getActiveOpens() + " active connections openings");
            log.debug(dnt + tcp.getPassiveOpens() + " passive connection openings");
            log.debug(dnt + tcp.getAttemptFails() + " failed connection attempts");
            log.debug(dnt + tcp.getEstabResets() + " connection resets received");
            log.debug(dnt + tcp.getCurrEstab() + " connections established");
            log.debug(dnt + tcp.getInSegs() + " segments received");
            log.debug(dnt + tcp.getOutSegs() + " segments send out");
            log.debug(dnt + tcp.getRetransSegs() + " segments retransmited");
            log.debug(dnt + tcp.getInErrs() + " bad segments received.");
            log.debug(dnt + tcp.getOutRsts() + " resets sent");

        }
        catch (SigarException e) {
            log.error("Error while fetching Sigar statistics", e);
        }
    }

    private final NodeMon _nodeMon;
    private final Sigar _sigar;

    private static final Logger log = Logger.getLogger(SigarPolledNodeSensor.class);
}
