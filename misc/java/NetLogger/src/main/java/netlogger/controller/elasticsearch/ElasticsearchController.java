package netlogger.controller.elasticsearch;

import com.jfoenix.controls.JFXButton;
import elasticsearch.ElasticsearchInsertManager;
import javafx.application.Platform;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.fxml.FXML;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.VBox;
import netlogger.Launcher;
import netlogger.model.MeasureIn;
import netlogger.model.settings.LabelStrings;
import netlogger.model.settings.SettingField;
import netlogger.model.settings.Updatable;
import netlogger.util.ConfigConstants;
import netlogger.util.settings.SettingsManager;
import measure.proto.Measure;
import measure.proto.Subject;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;


public class ElasticsearchController implements Updatable
{
    @FXML
    public void initialize () {

        _elasticsearchInsertManager = new ElasticsearchInsertManager();
        _elasticsearchEnabledProperty = new SimpleBooleanProperty(false);

        _storeMeasures = false;

        _buttonVBox.disableProperty().bind(_elasticsearchEnabledProperty.not());

        _elasticsearchEnabledProperty.addListener((observable, oldValue, newValue) -> {
            if (!newValue){
                closeResources();
                Platform.runLater(this::setStartButton);
                _storeMeasures = false;
            }
        });

        _playPauseButton.setOnMouseClicked(event -> {
            if (_storeMeasures) {
                setStartButton();
            }
            else {
                setStopButton();
            }
            _storeMeasures = !_storeMeasures;
        });

        SettingsManager.getInstance().getSettingsBridge().addUpdatableClass(this);

    }

    private void setStartButton(){
        _playPauseButton.setText("Start");
        _playPauseButton.setStyle("-fx-background-color: #e7e7e7; -fx-text-fill: #009819");
        _playPauseButton.setGraphic(_playImageView);
    }

    private void setStopButton(){
        _playPauseButton.setText("Stop");
        _playPauseButton.setStyle("-fx-background-color: #e7e7e7; -fx-text-fill: #d63939");
        _playPauseButton.setGraphic(_pauseImageView);
    }

    public void offerMeasure (MeasureIn measureIn) {
        if (!_storeMeasures) {
            return;
        }
        if (isAllowedSubject(measureIn.getMeasure().getSubject())) {
            Measure measure = measureIn.getMeasure();

            String subj = measure.getSubject().toString();
            String natsTopic = measureIn.getNatsTopic();

            Map<String, Object> values = new HashMap<>(measure.getStringsMap());
            values.putAll(measure.getIntegersMap());
            values.putAll(measure.getDoublesMap());
            values.put("subject", subj);
            values.put("nats_topic", natsTopic);
            values.put("timestamp", new Date(measure.getTimestamp().getSeconds() * 1000));
            values.put("stored_timestamp", new Date(System.currentTimeMillis()));
            values.put("type", _typeString);

            _elasticsearchInsertManager.storeData(values);
        }
    }

    public void requestTermination () {
        SettingsManager.getInstance().getSettingsBridge().removeUpdatableClass(this);
        closeResources();
    }

    @Override
    public void update (HashMap<String, Object> settings) {
        String newHost = (String) settings.get(LabelStrings.HOST_SETTING_STRING);
        String newPort = (String) settings.get(LabelStrings.PORT_SETTING_STRING);
        boolean enabledNow = (Boolean)settings.getOrDefault(LabelStrings.ELASTICSEARCH_ENABLED, false);

        // Create a new elasticsearch if the host or port has changed or is is enabled now and it wasn't before
        if (!newHost.equals(_hostString) || !newPort.equals(_portString) || (enabledNow && !_elasticsearchEnabled)) {
            // But we have to check that it's actually enabled, not just that the host/port changed
            if (enabledNow) {
                _elasticsearchEnabled = true;

                closeResources();
                init(newHost, newPort, "http");
            }
        }

        _hostString = newHost;
        _portString = newPort;
        _elasticsearchEnabled = enabledNow;

        _schemeString = (String) settings.get(LabelStrings.SCHEME_SETTING_STRING);
        _maxWaitTimeString = (String) settings.get(LabelStrings.MAX_WAIT_TIME_SETTING_STRING);
        _maxCountString = (String) settings.get(LabelStrings.MAX_COUNT_SETTING_STRING);
        _typeString = (String) settings.getOrDefault(LabelStrings.TYPE_SETTING_STRING, "http");

        // Update all the relevant elasticsearch stuff based on the settings
        _elasticsearchInsertManager.updateMaxCount(Long.valueOf(_maxCountString));
        _elasticsearchInsertManager.updateMaxWaitTime(Long.valueOf(_maxWaitTimeString));

        _elasticsearchEnabledProperty.set(_elasticsearchEnabled);
    }


    private void closeResources () {
        _elasticsearchInsertManager.close();
    }

    private void init (String host, String port, String scheme) {
        if (_elasticsearchEnabled) {
            _elasticsearchInsertManager = new ElasticsearchInsertManager();

            _elasticsearchInsertManager.init(host,
                    Integer.valueOf(port), scheme);

            _elasticsearchInsertManager.checkConnectivity(host,
                    Integer.valueOf(port));

            _elasticsearchInsertManager.setIndex("stockbridge");
            _elasticsearchInsertManager.setIndexType("measure");

            new Thread(_elasticsearchInsertManager::run, "Elasticsearch storage thread").start();
        }
    }


    private boolean isAllowedSubject (Subject measureSubject) {
        return measureSubject.equals(Subject.traffic) ||
                measureSubject.equals(Subject.link_description) ||
                measureSubject.equals(Subject.rtt);
    }

    @FXML
    private VBox _buttonVBox;
    private ElasticsearchInsertManager _elasticsearchInsertManager;

    private BooleanProperty _elasticsearchEnabledProperty;

    @SettingField(settingText = LabelStrings.ELASTICSEARCH_ENABLED, configText = ConfigConstants.ELASTICSEARCH_ENABLED, defaultValue = "true")
    private boolean _elasticsearchEnabled;

    @SettingField(settingText = LabelStrings.HOST_SETTING_STRING, configText = ConfigConstants.ELASTICSEARCH_HOST, defaultValue = "127.0.0.1")
    private String _hostString;
    @SettingField(settingText = LabelStrings.MAX_COUNT_SETTING_STRING, configText = ConfigConstants.ELASTICSEARCH_MAX_STORED, defaultValue = "100")
    private String _maxCountString;
    @SettingField(settingText = LabelStrings.MAX_WAIT_TIME_SETTING_STRING, configText = ConfigConstants.ELASTICSEARCH_MAX_WAIT, defaultValue = "10000")
    private String _maxWaitTimeString;
    @FXML
    private JFXButton _playPauseButton;
    @SettingField(settingText = LabelStrings.PORT_SETTING_STRING, configText = ConfigConstants.ELASTICSEARCH_PORT, defaultValue = "9100")
    private String _portString;
    @SettingField(settingText = LabelStrings.SCHEME_SETTING_STRING, configText = ConfigConstants.ELASTICSEARCH_SCHEME, defaultValue = "http")
    private String _schemeString;
    private boolean _storeMeasures;
    @SettingField(settingText = LabelStrings.TYPE_SETTING_STRING, configText = ConfigConstants.ELASTICSEARCH_TYPE, defaultValue = "stockbridge")
    private String _typeString;
    private final ImageView _playImageView = new ImageView(new Image(Launcher.class.getResourceAsStream("/images/font-awesome_4-7-0_play_16_0_009819_none.png")));
    private final ImageView _pauseImageView = new ImageView(new Image(Launcher.class.getResourceAsStream("/images/font-awesome_4-7-0_pause_16_0_d63939_none.png")));
}
