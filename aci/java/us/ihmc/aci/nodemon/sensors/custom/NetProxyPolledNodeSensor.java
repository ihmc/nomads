package us.ihmc.aci.nodemon.sensors.custom;

import us.ihmc.aci.ddam.DataType;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;

/**
 * NetProxyPolledNodeSensor.java
 * <p/>
 * Class <code>NetProxyPolledNodeSensor</code> defines a NetProxy protocol
 * sensor that requires periodic invocations to the method <code>updateData()</code> to receive data.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class NetProxyPolledNodeSensor implements PolledNodeSensor
{
    @Override
    public void update (DataType type)
    {

    }
}
