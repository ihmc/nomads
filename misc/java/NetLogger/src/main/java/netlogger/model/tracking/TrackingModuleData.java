package netlogger.model.tracking;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TrackingModuleData
{
    public TrackingModuleData () {

    }

    public String getClientID () {
        return _clientID;
    }

    public String getIdentificationString () {
        return _applicationName + ":" + _moduleName;
    }

    public String getApplicationName () {
        return _applicationName;
    }

    public void setApplicationName (String applicationName) {
        _applicationName = applicationName;
    }

    public String getModuleName () {
        return _moduleName;
    }

    public void setClientID (String clientID) {
        _clientID = clientID;
    }

    public void setModuleName (String moduleName) {
        _moduleName = moduleName;
    }


    private String _moduleName;
    private String _applicationName;
    private String _clientID;
    private static final Logger _logger = LoggerFactory.getLogger(TrackingModuleData.class);
}
