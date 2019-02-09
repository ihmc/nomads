package us.ihmc.cue.OSUStoIMSBridgePlugin;


import aQute.bnd.annotation.component.*;
import aQute.bnd.annotation.metatype.Configurable;
import mil.dod.th.core.controller.TerraHarvestController;
import mil.dod.th.core.log.Logging;
import mil.dod.th.core.persistence.ObservationStore;
import org.osgi.service.event.Event;
import org.osgi.service.event.EventAdmin;
import org.osgi.service.event.EventConstants;
import org.osgi.service.event.EventHandler;
import org.osgi.service.log.LogService;
import java.util.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;


@Component(provide = {EventHandler.class}, /* provides EventHandler service to receive OSGi events */
        immediate = true, /* activate always even if no consumers */
        designate = OSUStoIMSBridgeConfigInterface.class, /* class containing config info for the metatype and config admin services */
        configurationPolicy = ConfigurationPolicy.require, /* activate bundle even if configuration does not exist */
        properties = {EventConstants.EVENT_TOPIC + "=" + ObservationStore.TOPIC_OBSERVATION_PERSISTED
                + "|" + ObservationStore.TOPIC_OBSERVATION_MERGED, /* register for events on this topic */
        })
public class OSUStoIMSBridge implements EventHandler
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

    @Override
    public void handleEvent (Event event) {
        // TODO Auto-generated method stub

        if (!_run) {
            Logging.log(LogService.LOG_INFO, "SampleConsumer::handleEvent...NOT RUNNING....IGNORING EVENT.");
            return;
        }

        try {
            // Check event topic to make sure it is something we are interested in.ima
            if (event.getTopic().compareTo(ObservationStore.TOPIC_OBSERVATION_PERSISTED) == 0
                    || event.getTopic().compareTo(ObservationStore.TOPIC_OBSERVATION_MERGED) == 0) {
                // We need to return as soon as possible. If handleEvent() takes too long,
                // it can cause the framework to time out and stop sending events to this bundle.

                // Get the UUID for the observation that was just posted.
                UUID obsUUID = (UUID) event.getProperty(ObservationStore.EVENT_PROP_OBSERVATION_UUID);
                if (_receivedUUIDs.contains(obsUUID)){
                    Logging.log(LogService.LOG_DEBUG, this.getClass().getSimpleName() + "::UUID " + obsUUID + " was received from" +
                            " IMS Bridge and should not be republished");
                    return;
                }

                Logging.log(LogService.LOG_INFO, "SampleConsumer: got event UUID: " + obsUUID.toString());
                // Put the UUID in the queue for processing by the background thread.
                _eventQueue.offer(obsUUID);
            }
            else {
                Logging.log(LogService.LOG_INFO, this.getClass().getSimpleName() + ":: unexpected event topic %s", event.getTopic());
            }
        } catch (Exception e) {

            Logging.log(LogService.LOG_INFO, this.getClass().getSimpleName() + ":: got exception %s", e.getMessage());
        }
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
    public void setUtilComponent(TerraHarvestController th) {
        _systemID = th.getId();
    }

    @Reference
    public void setEventAdmin(EventAdmin eventAdmin){
        //Logging.log(LogService.LOG_DEBUG, "OSUStoIMSBridge::EVENT ADMIN BEING SET!");
    }

    /**
     * Called on startup in the <code>activate()</code> method
     */
    private void init () {
        Logging.log(LogService.LOG_ERROR, _processName + "init called");
        _receivedUUIDs = new HashSet<>();

        // Start processing thread
        _observationPublisher = new ObservationPublisher(_eventQueue);
        _observationPublisher.setObservationStore(_obsStore);
        _observationPublisher.setName("ObservationPublisher");
        _observationPublisher.setDaemon(true);

        _subscriptionHandler = new SubscriptionHandler();
        _subscriptionHandler.setControllerID(_systemID);
        // Set handlers for connecting the subscriber to other classes
        _subscriptionHandler.setFCIDHandler(string -> _observationPublisher.updateFCID(string));
        _subscriptionHandler.setUUIDHandler(uuid -> {
            _receivedUUIDs.add(uuid);
        });
        _subscriptionHandler.setObservationStore(_obsStore);

        _observationPublisher.start();
    }

    /**
     * Called when the bundle is supposed to shutdown. Called by <code>deactivate()</code> method
     */
    private void stop () {
        if (_observationPublisher != null) {
            _observationPublisher.kill = true;
            _observationPublisher.interrupt();
            try {
                _observationPublisher.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            _observationPublisher = null;
        }

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

        OSUStoIMSBridgeConfigInterface consumerConfig = Configurable.createConfigurable(OSUStoIMSBridgeConfigInterface.class, properties);
        _run = consumerConfig.Run();
        _fedHost = consumerConfig.host();
        _fedPort = consumerConfig.port();

        boolean urlChanged = !prevHost.orElse("").equals(_fedHost) || prevPort != _fedPort;

        Logging.log(LogService.LOG_DEBUG, this.getClass().getSimpleName() + "::URL changed: " + urlChanged + " Run: " + _run);
        // Only update if we need to. The subscriber has an open web socket that shouldn't be updated unless necessary
        if ((_run && urlChanged) ||
            (_run && !_subscriptionHandler.isConnected())) {
            Logging.log(LogService.LOG_INFO, this.getClass().getSimpleName() + "::Creating publisher and subscriber for Federation at: " + _fedHost + ":" + _fedPort);
            setUpFederationConnections(_fedHost, _fedPort);
        }

        if (!_run) {
            Logging.log(LogService.LOG_INFO, this.getClass().getSimpleName() + "::Calling close for Federation subscriber");
            // No need to close publisher since it only publishes when it gets events
            // and no events are put into the queue if run is not true
            _subscriptionHandler.closeConnection();
        }

        Logging.log(LogService.LOG_INFO, "Updating properties for " + _processName);
    }

    /**
     * Connects the publisher and subscriber objects to the specific host:port Federation instance
     * @param host IP of the Federation instance
     * @param port Port of the Federation instance
     */
    private void setUpFederationConnections(String host, int port) {
        _subscriptionHandler.connect(host, port);
        _observationPublisher.updatePublishURL(host, port);
    }


    private Boolean _run = false;
    private String _processName = "OSUStoIMSBridgePlugin";
    private BlockingQueue<UUID> _eventQueue = new LinkedBlockingQueue<>();
    private ObservationStore _obsStore;
    private HashSet<UUID> _receivedUUIDs;

    private ObservationPublisher _observationPublisher = null;
    private SubscriptionHandler _subscriptionHandler;
    private String _fedHost;
    private int _fedPort;
    private int _systemID = 0;
}