package netlogger.model;

import com.google.protobuf.InvalidProtocolBufferException;
import io.nats.client.Message;
import main.DatabrokerAdapter;
import measure.proto.Measure;
import nats.NATSConnectionBroker;
import netlogger.model.messages.MeasureHandler;
import netlogger.model.settings.LabelStrings;
import netlogger.model.settings.SettingField;
import netlogger.model.settings.Updatable;
import netlogger.util.ConfigConstants;
import netlogger.util.settings.SettingsManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.BlockingDeque;
import java.util.concurrent.LinkedBlockingDeque;

/**
 * The message receiver class leverages the nats broker interface to get measures and send them to
 * the designated handler.
 */
public class MessageReceiver extends DatabrokerAdapter implements Updatable
{
    /**
     * Constructor
     */
    public MessageReceiver () {
        _terminationRequested = false;

        _natsTopics = new ArrayList<>();
        _brokerInterface = new NATSConnectionBroker();

        SettingsManager.getInstance().getSettingsBridge().addUpdatableClass(this);
    }

    /**
     * Start up the broker interface
     */
    @Override
    public void connect () {
        if (!_brokerInterface.connect(_host, _port)) {
            _logger.error("Error connecting with NATS server...");
            return;
        }
        else {
            _logger.info("Connected with NATS server on " + _host + ":" + _port);
        }

        for (String topic : _natsTopics) {
            ((NATSConnectionBroker)_brokerInterface).getNatsSubscriptionHandler().subscribe(topic, this::handleMessage);
        }
    }

    /**
     * Request termination for the thread
     */
    @Override
    public void requestTermination () {
        _terminationRequested = true;

        SettingsManager.getInstance().getSettingsBridge().removeUpdatableClass(this);
    }

    /**
     * Launcher function. Runs while the thread has not been asked to terminate
     */
    @Override
    public void run () {

        while (!_terminationRequested) {
            checkMeasures();
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        _brokerInterface.closeConnection();
    }

    /**
     * Set the handler for the measures
     * @param handler The handler for the measure
     */
    public void setHandler (MeasureHandler handler) {
        _measureHandler = handler;
    }

    @Override
    public void setInfo (String host, String port, List<String> topics) {
        _host = host;
        _port = port;

        _natsTopics = topics;
    }

    @Override
    public void update (HashMap<String, Object> settings) {
        String newHost = (String) settings.get(LabelStrings.NATS_HOST_TEXT);
        String newPort = (String) settings.get(LabelStrings.NATS_PORT_TEXT);

        List<String> newSettings = (List<String>) settings.get(LabelStrings.NATS_TOPICS_TEXT);

        if (!newHost.equals(_host) || !newPort.equals(_port)) {
            setInfo(newHost, newPort, newSettings);
            connect();
        }
        else {
            changeTopics(newSettings);
        }
    }

    /**
     * Give a measure to the handler to setCurrent
     *
     * @param measure new measure
     */
    private void addMeasure (MeasureIn measure) {
        _measureHandler.handleNewMeasure(measure);
    }

    /**
     * Changes the topics that the NATS broker should subscribe to
     *
     * @param topics
     */
    private void changeTopics (List<String> topics) {
        List<String> removedTopics = new ArrayList<>();

        for (String oldTopic : _natsTopics) {
            // If the new topics don't contain a previously subscribed topic, unsubscribe
            if (!topics.contains(oldTopic)) {
                removedTopics.add(oldTopic);
            }
        }

        for (String removedTopic : removedTopics) {
            _natsTopics.remove(removedTopic);
            ((NATSConnectionBroker)_brokerInterface).getNatsSubscriptionHandler().unsubscribe(removedTopic);
        }

        for (String newTopic : topics) {
            // If the old topics don't contain a new topic, subscribe to it
            if (!_natsTopics.contains(newTopic)) {
                _natsTopics.add(newTopic);
                ((NATSConnectionBroker)_brokerInterface).getNatsSubscriptionHandler().subscribe(newTopic, this::handleMessage);
            }
        }
    }

    /**
     * Checks the interface for any new measures
     */
    private void checkMeasures () {
        ArrayList<MeasureIn> measures = new ArrayList<>();
        _measureQueue.drainTo(measures);

        for (MeasureIn measure : measures) {
            addMeasure(measure);
        }

        measures.clear();
    }

    private void handleMessage (Message message) {
        String topic = message.getSubject();
        Measure measure;
        _logger.debug("Received message from nats on topic: {}", message.getSubject());
        try {
            measure = Measure.parseFrom(message.getData());
        } catch (InvalidProtocolBufferException e) {
            e.printStackTrace();
            return;
        }

        MeasureIn measureIn = new MeasureIn(measure, topic);
        _measureQueue.offer(measureIn);
    }


    @SettingField(settingText = LabelStrings.NATS_HOST_TEXT, configText = ConfigConstants.NATS_HOST, defaultValue = "128.49.235.115")
    private String _host;
    private MeasureHandler _measureHandler;

    @SettingField(settingText = LabelStrings.NATS_TOPICS_TEXT, configText = ConfigConstants.NATS_TOPICS, defaultValue = "")
    private List<String> _natsTopics;
    @SettingField(settingText = LabelStrings.NATS_PORT_TEXT, configText = ConfigConstants.NATS_PORT, defaultValue = "4222")
    private String _port;

    private final BlockingDeque<MeasureIn> _measureQueue = new LinkedBlockingDeque<>(1000);
    private static final Logger _logger = LoggerFactory.getLogger(MessageReceiver.class);

}
