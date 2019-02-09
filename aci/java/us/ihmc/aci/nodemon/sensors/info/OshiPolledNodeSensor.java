package us.ihmc.aci.nodemon.sensors.info;

import org.apache.log4j.Logger;
import oshi.SystemInfo;
import oshi.hardware.HardwareAbstractionLayer;
import oshi.hardware.PowerSource;
import oshi.hardware.Processor;
import oshi.software.os.OperatingSystem;
import us.ihmc.aci.ddam.DataType;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;

import java.util.Objects;

/**
 * OshiPolledNodeSensor.java
 * <p/>
 * Class <code>OshiPolledNodeSensor</code> provide system resources statistics provided by
 * the Java Oshi's library: https://github.com/dblock/oshi
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class OshiPolledNodeSensor implements PolledNodeSensor
{
    public OshiPolledNodeSensor (NodeMon nodeMon)
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _systemInfo = new SystemInfo();
    }

    @Override
    public void update (DataType type)
    {
        OperatingSystem os = _systemInfo.getOperatingSystem();
        log.info("OS: " + os);
        //_nodeMon.updateData(os);


        HardwareAbstractionLayer hal = _systemInfo.getHardware();
        log.info("Memory - available: " + hal.getMemory().getAvailable());
        log.info("Memory - total: " + hal.getMemory().getTotal());
        int i = 0;
        for (Processor p : hal.getProcessors()) {
            log.info("Processor #" + i + ": " + p);
            i++;
        }

        i = 0;
        for (PowerSource ps : hal.getPowerSources()) {
            log.info("PowerSource #" + i + ": " + ps);
            i++;
        }
        //_nodeMon.updateData(hal);
    }

    private final SystemInfo _systemInfo;
    private final NodeMon _nodeMon;

    private static final Logger log = Logger.getLogger(OshiPolledNodeSensor.class);

}
