package us.ihmc.aci.nodemon.sensors;

import us.ihmc.aci.ddam.DataType;

/**
 * PolledNodeSensor.java
 * <p/>
 * Interface <code>PolledNodeSensor</code> define methods in common with all sensors implementing this interface.
 * <code>PolledNodeSensor</code> defines a sensor that requires periodic invocations to the method <code>updateData()
 * </code>
 * to receive data.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface PolledNodeSensor
{
    /**
     * This method is invoked periodically by the <code>NodeMon</code>.
     * The subclass must provide updated data to the NodeMonitor by invoking
     * the <code>updateData()</code> method
     *
     * @param type the <code>DataType</code> for which we want to request the update.
     */
    void update (DataType type);
}
