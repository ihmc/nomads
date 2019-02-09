package us.ihmc.cue.imsbridgesubscriber;


import aQute.bnd.annotation.component.*;
import aQute.bnd.annotation.metatype.Configurable;
import mil.dod.th.core.log.Logging;
import mil.dod.th.core.persistence.ObservationStore;
import mil.dod.th.core.system.TerraHarvestSystem;
import org.osgi.service.log.LogService;
import us.ihmc.cue.imsbridgesubscriber.subscriber.SubscriptionHandler;

import java.util.*;

@Component(immediate = true,
        designate = ConfigInterface.class, /* class containing config info for the metatype and config admin services */
        configurationPolicy = ConfigurationPolicy.require)
public class IMSBridgeSubscriber
{
    @Activate // <- tells bnd this is the activate method
    // activate method called by the framework when all dependencies have been satisfied and
    // the bundle should start processing
    public void activate (Map<String, Object> properties) {
        init();
        updateConfig(properties);
    }

    @Deactivate // <- tells bnd this is the deactivate method
    // deactivate method called by the framework when the bundle should be shut down
    // because the framework is stopping/the bundle 0 being uninstalled/etc.
    public void deactivate () {
        stop();
    }


    @Modified
    public void modified (Map<String, Object> properties) {
        updateConfig(properties);
    }

    @Reference
    // Get reference to the ObservationStore service so we can retrieve observations after they are posted
    // This method is called by the framework due to the @Reference annotation.
    public void setObservationStore (ObservationStore obsStore) {
        _obsStore = obsStore;
    }


    @Reference
    public void setTerraHarvestSystem(TerraHarvestSystem thSystem)
    {
        _systemID = thSystem.getId();
    }

    /**
     * Called on startup in the <code>activate()</code> method
     */
    private void init () {
        Logging.log(LogService.LOG_ERROR, _processName + "init called");
        _subscriptionHandler = new SubscriptionHandler();
        _subscriptionHandler.setControllerID(_systemID);
        _subscriptionHandler.setObservationStore(_obsStore);
    }

    /**
     * Called when the bundle is supposed to shutdown. Called by <code>deactivate()</code> method
     */
    private void stop () {
        _subscriptionHandler.closeConnection();
        Logging.log(LogService.LOG_INFO, _processName + ": STOPPED!!");
    }

    /**
     * Called by <code>modified</code> when the config is updated in the OSUS GUI
     * @param properties Properties received after the Properties are updated in the GUI
     */
    private void updateConfig (Map<String, Object> properties) {
        Optional<String> prevHost = Optional.ofNullable(_fedHost);
        int prevPort = _fedPort;

        ConfigInterface consumerConfig = Configurable.createConfigurable(ConfigInterface.class, properties);
        Boolean run = consumerConfig.Run();
        _fedHost = consumerConfig.host();
        _fedPort = consumerConfig.port();

        boolean urlChanged = !prevHost.orElse("").equals(_fedHost) || prevPort != _fedPort;

        Logging.log(LogService.LOG_DEBUG, this.getClass().getName() + "::URL changed: " + urlChanged + " Run: " + run);
        // Only update if we need to. The subscriber has an open web socket that shouldn't be updated unless necessary
        if ((run && urlChanged) ||
                (run && !_subscriptionHandler.isConnected())) {
            Logging.log(LogService.LOG_INFO, this.getClass().getName() + "::Creating publisher and subscriber for Federation at: " + _fedHost + ":" + _fedPort);
            setUpFederationConnections(_fedHost, _fedPort);
        }

        if (!run) {
            Logging.log(LogService.LOG_INFO, this.getClass().getName() + "::Calling close for Federation subscriber");
            // No need to close publisher since it only publishes when it gets events
            // and no events are put into the queue if run is not true
            _subscriptionHandler.closeConnection();
        }

        Logging.log(LogService.LOG_INFO, "Updating properties for " + _processName);
    }

    /**
     * Sets the publisher's host/port to publish to
     * @param host IP of the Federation instance
     * @param port Port of the Federation instance
     */
    private void setUpFederationConnections(String host, int port) {
        _subscriptionHandler.connect(host, port);
    }


    private String _processName = "imsbridgesubscriber";
    private ObservationStore _obsStore;

    private SubscriptionHandler _subscriptionHandler;
    private String _fedHost;
    private int _fedPort;
    private int _systemID = 0;
}
