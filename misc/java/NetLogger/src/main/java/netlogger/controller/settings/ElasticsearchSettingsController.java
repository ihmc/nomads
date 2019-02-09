package netlogger.controller.settings;

import com.jfoenix.controls.JFXCheckBox;
import com.jfoenix.controls.JFXTextField;
import javafx.fxml.FXML;
import netlogger.model.settings.BooleanSetting;
import netlogger.model.settings.LabelStrings;
import netlogger.model.settings.Setting;
import netlogger.model.settings.StringSetting;
import netlogger.util.ConfigConstants;
import netlogger.util.ConfigFileManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

public class ElasticsearchSettingsController implements SettingsController
{
    @FXML
    public void initialize () {
        createSettingsObjects();
    }

    @Override
    public void saveToConfig () {
        _fileManager.updateConfig(ConfigConstants.ELASTICSEARCH_HOST, _hostSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.ELASTICSEARCH_PORT, _portSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.ELASTICSEARCH_SCHEME, _schemeSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.ELASTICSEARCH_MAX_WAIT, _waitTimeSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.ELASTICSEARCH_MAX_STORED, _countSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.ELASTICSEARCH_TYPE, _typeSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.ELASTICSEARCH_ENABLED, _enabledSetting.getCurrentState());
    }

    @Override
    public void revertAll () {
        for (Setting setting : _allSettings) {
            setting.saveCurrentState(setting.getOriginalState());
            setDataForSetting(setting);
        }
    }

    @Override
    public void initializeFromConfigFile () {
        String defaultHost = _fileManager.getStringValue(ConfigConstants.ELASTICSEARCH_HOST);
        String defaultPort = _fileManager.getStringValue(ConfigConstants.ELASTICSEARCH_PORT);
        String defaultScheme = _fileManager.getStringValue(ConfigConstants.ELASTICSEARCH_SCHEME);
        String defaultWait = _fileManager.getStringValue(ConfigConstants.ELASTICSEARCH_MAX_WAIT);
        String defaultCount = _fileManager.getStringValue(ConfigConstants.ELASTICSEARCH_MAX_STORED);
        String defaultType = _fileManager.getStringValue(ConfigConstants.ELASTICSEARCH_TYPE);
        boolean defaultEnabled = _fileManager.getBooleanValue(ConfigConstants.ELASTICSEARCH_ENABLED);

        _hostText.setText(defaultHost);
        _portText.setText(defaultPort);
        _schemeText.setText(defaultScheme);
        _waitTimeText.setText(defaultWait);
        _countText.setText(defaultCount);
        _typeText.setText(defaultType);

        _enabledCheck.setSelected(defaultEnabled);

        _hostSetting.saveOriginalState(defaultHost);
        _portSetting.saveOriginalState(defaultPort);
        _schemeSetting.saveOriginalState(defaultScheme);
        _countSetting.saveOriginalState(defaultCount);
        _waitTimeSetting.saveOriginalState(defaultWait);
        _typeSetting.saveOriginalState(defaultType);
        _enabledSetting.saveOriginalState(defaultEnabled);

        _hostSetting.saveCurrentState(defaultHost);
        _portSetting.saveCurrentState(defaultPort);
        _schemeSetting.saveCurrentState(defaultScheme);
        _countSetting.saveCurrentState(defaultCount);
        _waitTimeSetting.saveCurrentState(defaultWait);
        _typeSetting.saveCurrentState(defaultType);
        _enabledSetting.saveCurrentState(defaultEnabled);

    }

    @Override
    public void initializeListeners () {
        _hostText.textProperty().addListener((observable, oldValue, newValue) -> {
            _hostSetting.saveCurrentState(newValue);
            callParentControllerChanges();
        });

        _portText.textProperty().addListener((observable, oldValue, newValue) -> {
            _portSetting.saveCurrentState(newValue);
            callParentControllerChanges();
        });

        _schemeText.textProperty().addListener((observable, oldValue, newValue) -> {
            _schemeSetting.saveCurrentState(newValue);
            callParentControllerChanges();
        });

        _waitTimeText.textProperty().addListener((observable, oldValue, newValue) -> {
            _waitTimeSetting.saveCurrentState(newValue);
            callParentControllerChanges();
        });

        _countText.textProperty().addListener((observable, oldValue, newValue) -> {
            _countSetting.saveCurrentState(newValue);
            callParentControllerChanges();
        });

        _typeText.textProperty().addListener((observable, oldValue, newValue) -> {
            _typeSetting.saveCurrentState(newValue);
            callParentControllerChanges();
        });

        _enabledCheck.selectedProperty().addListener((observable, oldValue, newValue) -> {
            _enabledSetting.saveCurrentState(newValue);
            callParentControllerChanges();
        });
    }

    @Override
    public HashMap<String, Object> getSettings () {
        HashMap<String, Object> settingsHashMap = new HashMap<>();
        settingsHashMap.put(LabelStrings.HOST_SETTING_STRING, _hostSetting.getCurrentState());
        settingsHashMap.put(LabelStrings.PORT_SETTING_STRING, _portSetting.getCurrentState());
        settingsHashMap.put(LabelStrings.SCHEME_SETTING_STRING, _schemeSetting.getCurrentState());
        settingsHashMap.put(LabelStrings.MAX_WAIT_TIME_SETTING_STRING, _waitTimeSetting.getCurrentState());
        settingsHashMap.put(LabelStrings.MAX_COUNT_SETTING_STRING, _countSetting.getCurrentState());
        settingsHashMap.put(LabelStrings.TYPE_SETTING_STRING, _typeSetting.getCurrentState());
        settingsHashMap.put(LabelStrings.ELASTICSEARCH_ENABLED, _enabledSetting.getCurrentState());

        return settingsHashMap;
    }

    @Override
    public void setConfigManager (ConfigFileManager fileManager) {
        _fileManager = fileManager;

    }

    public void setMainController (MainSettingsController mainController) {
        _mainSettingsController = mainController;
    }

    private void callParentControllerChanges () {
        _mainSettingsController.registerChangedController();
    }

    private void createSettingsObjects () {
        _hostSetting = new StringSetting();
        _portSetting = new StringSetting();
        _schemeSetting = new StringSetting();
        _waitTimeSetting = new StringSetting();
        _countSetting = new StringSetting();
        _typeSetting = new StringSetting();
        _enabledSetting = new BooleanSetting();

        _allSettings = new ArrayList<>();
        _allSettings.addAll(Arrays.asList(_hostSetting, _portSetting, _schemeSetting, _waitTimeSetting, _countSetting,
                _typeSetting));
    }

    private void setDataForSetting (Setting setting) {
        if (setting.equals(_hostSetting)) {
            _hostText.setText(String.valueOf(_hostSetting.getCurrentState()));
        }
        else if (setting.equals(_portSetting)) {
            _portText.setText(String.valueOf(_portSetting.getCurrentState()));
        }
        else if (setting.equals(_schemeSetting)) {
            _schemeText.setText(String.valueOf(_schemeSetting.getCurrentState()));
        }
        else if (setting.equals(_waitTimeSetting)) {
            _waitTimeText.setText(String.valueOf(_waitTimeSetting.getCurrentState()));
        }
        else if (setting.equals(_countSetting)) {
            _countText.setText(String.valueOf(_countSetting.getCurrentState()));
        }
        else if (setting.equals(_typeSetting)) {
            _typeText.setText(String.valueOf(_typeSetting.getCurrentState()));
        }
        else if (setting.equals(_enabledSetting)){
            _enabledCheck.setSelected(_enabledSetting.getCurrentState());
        }
    }

    @FXML
    private JFXCheckBox _enabledCheck;

    private MainSettingsController _mainSettingsController;
    private ConfigFileManager _fileManager;
    private List<Setting> _allSettings;

    private StringSetting _hostSetting;
    private StringSetting _portSetting;
    private StringSetting _schemeSetting;
    private StringSetting _waitTimeSetting;
    private StringSetting _countSetting;
    private StringSetting _typeSetting;
    private BooleanSetting _enabledSetting;

    @FXML
    private JFXTextField _hostText;
    @FXML
    private JFXTextField _portText;
    @FXML
    private JFXTextField _schemeText;
    @FXML
    private JFXTextField _waitTimeText;
    @FXML
    private JFXTextField _countText;
    @FXML
    private JFXTextField _typeText;

    private static final Logger _logger = LoggerFactory.getLogger(ElasticsearchSettingsController.class);

}
