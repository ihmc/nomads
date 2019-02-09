package us.ihmc.aci.nodemon;

import us.ihmc.aci.ddam.Container;
import us.ihmc.aci.nodemon.discovery.DiscoveryService;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyListener;
import us.ihmc.aci.nodemon.scheduler.ProxyScheduler;
import us.ihmc.aci.nodemon.scheduler.Scheduler;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;
import us.ihmc.util.serialization.SerializationException;

import java.io.IOException;

/**
 * NodeMon.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface NodeMon
{
    void init () throws IOException, SerializationException;

    void addNodeSensor (PolledNodeSensor nodeSensor);

    void updateData (String nodeId, Container c);

    Scheduler getScheduler ();

    ProxyScheduler getProxyScheduler ();

    WorldState getWorldState ();

}
