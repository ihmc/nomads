package us.ihmc.cue.imsbridgepublisher;


import aQute.bnd.annotation.component.*;
import aQute.bnd.annotation.metatype.Configurable;
import mil.dod.th.core.log.Logging;
import mil.dod.th.core.persistence.ObservationStore;
import org.osgi.service.event.Event;
import org.osgi.service.event.EventConstants;
import org.osgi.service.event.EventHandler;
import org.osgi.service.log.LogService;
import us.ihmc.cue.imsbridgepublisher.publisher.ObservationPublisher;

import java.util.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;


@Component(provide = {EventHandler.class}, /* provides EventHandler service to receive OSGi events */
        immediate = true, /* activate always even if no consumers */
        designate = ObservationListenerConfig.class, /* class containing config info for the metatype and config admin services */
        configurationPolicy = ConfigurationPolicy.require, /* activate bundle even if configuration does not exist */
        properties = {EventConstants.EVENT_TOPIC + "=" + ObservationStore.TOPIC_OBSERVATION_PERSISTED
                + "|" + ObservationStore.TOPIC_OBSERVATION_MERGED, /* register for events on this topic */
        })
public final class ObservationListener implements EventHandler
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

                Logging.log(LogService.LOG_INFO, "SampleConsumer: got event UUID: " + obsUUID.toString());
                // Put the UUID in the queue for processing by the background thread.
                _eventQueue.offer(obsUUID);
            }
            else {
                Logging.log(LogService.LOG_INFO, getClass().getName() + ":: unexpected event topic %s", event.getTopic());
            }
        } catch (Exception e) {

            Logging.log(LogService.LOG_INFO, getClass().getName() + ":: got exception %s", e.getMessage());
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

    /**
     * Called on startup in the <code>activate()</code> method
     */
    private void init () {
        Logging.log(LogService.LOG_INFO, getClass().getSimpleName() + "::init called");

        // Start processing thread
        _observationPublisher = new ObservationPublisher(_eventQueue);
        _observationPublisher.setObservationStore(_obsStore);
        _observationPublisher.setName("ObservationPublisher");
        _observationPublisher.setDaemon(true);

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

        Logging.log(LogService.LOG_INFO, getClass().getSimpleName() + ": STOPPED!!");
    }

    /**
     * Called by <code>modified</code> when the config is updated in the OSUS GUI
     * @param properties Properties received after the Properties are updated in the GUI
     */
    private void updateConfig (Map<String, Object> properties) {
        ObservationListenerConfig consumerConfig = Configurable.createConfigurable(ObservationListenerConfig.class, properties);
        _run = consumerConfig.Run();
        String fedHost = consumerConfig.host();
        int fedPort = consumerConfig.port();

        if (fedHost != null && !fedHost.isEmpty()) {
            setUpFederationConnections(fedHost, fedPort);
        }

        Logging.log(LogService.LOG_INFO, "Updating properties for " + getClass().getSimpleName());
    }

    /**
     * Sets the publisher's host/port to publish to
     * @param host IP of the Federation instance
     * @param port Port of the Federation instance
     */
    private void setUpFederationConnections(String host, int port) {
        _observationPublisher.updatePublishURL(host, port);
    }


    private Boolean _run = false;
    private BlockingQueue<UUID> _eventQueue = new LinkedBlockingQueue<>();
    private ObservationStore _obsStore;

    private ObservationPublisher _observationPublisher = null;
}