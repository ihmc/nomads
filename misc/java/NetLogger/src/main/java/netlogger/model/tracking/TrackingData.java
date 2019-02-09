package netlogger.model.tracking;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * Container for the {@link TrackingMeasure} of this module as well as data about the module
 */
public class TrackingData
{
    public TrackingData (String applicationName, String clientID) {
        _dataReceived = new HashMap<>();
        _data = new TrackingModuleData();
        _data.setApplicationName(applicationName);
        _data.setClientID(clientID);
    }

    /**
     * Add a new tracked measure based on the current UUID
     *
     * @param trackingMeasure
     */
    public synchronized void newLogReceived (TrackingMeasure trackingMeasure) {
        if (_dataReceived.containsKey(trackingMeasure.getCurrLogUUID())) {
            _logger.info("{} contains UUID: {} already", _data.getIdentificationString(), trackingMeasure.getCurrLogUUID());
        }
        _dataReceived.put(trackingMeasure.getCurrLogUUID(), trackingMeasure);
    }

    /**
     * Remove the tracked measure based on the previous UUID
     *
     * @param trackingMeasure
     */
    public synchronized void removeLog (TrackingMeasure trackingMeasure) {
        _dataReceived.remove(trackingMeasure.getPrevLogUUID());
    }

    public TrackingModuleData getData () {
        return _data;
    }

    public List<TrackingMeasure> getTrackingMeasures () {
        return new ArrayList<>(_dataReceived.values());
    }


    private HashMap<String, TrackingMeasure> _dataReceived;
    private TrackingModuleData _data;
    private static final Logger _logger = LoggerFactory.getLogger(TrackingData.class);
}
