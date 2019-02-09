package us.ihmc.aci.nodemon.sensors;

import us.ihmc.aci.ddam.DataType;

/**
 * AsyncNodeSensor.java
 * <p/>
 * Interface <code>AsyncNodeSensor</code> define methods in common with all sensors implementing this interface.
 * <code>AsyncNodeSensor</code> defines a sensor that returns data asynchronously to the caller.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface AsyncNodeSensor
{
    /**
     * After this method is invoked, the sensor is expected to asynchronously provide data
     * to the <code>NodeMon</code> by calling the <code>updateData()</code> method.
     */
    void start ();

    /**
     * This is a callback method that returns periodically data retrieved from the sensor
     *
     * @param type the <code>NodeMonDataType</code> for which we want to request the update.
     * @param obj the data <code>Object</code>
     */
    void update (DataType type, Object obj);

    /**
     * After this method is invoked, the sensor is expected to stop any threads that may have been started.
     * The <code>NodeMon</code> will delete this object once this method returns
     */
    void stop ();
}
