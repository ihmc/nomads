package netlogger.controller.settings;

import com.jfoenix.controls.JFXCheckBox;
import com.jfoenix.controls.JFXSlider;
import javafx.fxml.FXML;
import javafx.scene.control.ColorPicker;
import javafx.scene.control.Label;
import javafx.scene.control.TextFormatter;
import javafx.scene.input.*;
import javafx.scene.paint.Color;
import javafx.util.converter.IntegerStringConverter;
import netlogger.model.settings.*;
import netlogger.util.ConfigConstants;
import netlogger.util.ConfigFileManager;
import netlogger.view.HotKeyTextField;
import netlogger.view.IntegerTextField;

import java.util.*;


public class GeneralSettingsController implements SettingsController
{
    @FXML
    public void initialize () {
        createSettingsObjects();
        setDefaults();
    }

    @Override
    public void saveToConfig () {
        _fileManager.updateConfig(ConfigConstants.FILE_STORAGE_BOOL, _storeInFileSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.RELOAD_ON_SCROLL_BOOL, _reloadOnScrollSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.MULTIPLE_SEARCH_COND_BOOL, _multipleSearchCondSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.MEMORY_STORAGE_SIZE, _recordsInMemorySetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.HIGHLIGHT_COLOR, _highlightColorSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.FRAMES_PER_SECOND, _updatesPerSecondSetting.getCurrentState());

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
        boolean storeInFile = _fileManager.getBooleanValue(ConfigConstants.FILE_STORAGE_BOOL);
        boolean reloadOnScroll = _fileManager.getBooleanValue(ConfigConstants.RELOAD_ON_SCROLL_BOOL);
        boolean multipleSearchCond = _fileManager.getBooleanValue(ConfigConstants.MULTIPLE_SEARCH_COND_BOOL);


        String recordsInMemory = _fileManager.getStringValue(ConfigConstants.MEMORY_STORAGE_SIZE);
        String updatesPerSecond = _fileManager.getStringValue(ConfigConstants.FRAMES_PER_SECOND);
        String colorValue = _fileManager.getStringValue(ConfigConstants.HIGHLIGHT_COLOR);


        _storeInFileCheck.setSelected(storeInFile);
        _reloadOnScrollCheck.setSelected(reloadOnScroll);
        _multipleSearchCondCheck.setSelected(multipleSearchCond);
        _numberOfRecordsInMemoryText.setText(recordsInMemory);
        _updatesPerSecondSlider.setValue(Double.valueOf(updatesPerSecond));
        _highlightColorPicker.setValue(Color.web(colorValue));

        _storeInFileSetting.saveOriginalState(storeInFile);
        _reloadOnScrollSetting.saveOriginalState(reloadOnScroll);
        _multipleSearchCondSetting.saveOriginalState(multipleSearchCond);
        _updatesPerSecondSetting.saveOriginalState(Double.valueOf(updatesPerSecond));

        _recordsInMemorySetting.saveOriginalState(Integer.valueOf(recordsInMemory));
        _highlightColorSetting.saveOriginalState(colorValue);

        _storeInFileSetting.saveCurrentState(storeInFile);
        _reloadOnScrollSetting.saveCurrentState(reloadOnScroll);
        _multipleSearchCondSetting.saveCurrentState(multipleSearchCond);
        _updatesPerSecondSetting.saveCurrentState(Double.valueOf(updatesPerSecond));
        _recordsInMemorySetting.saveCurrentState(Integer.valueOf(recordsInMemory));
        _highlightColorSetting.saveCurrentState(colorValue);
    }

    @Override
    public void initializeListeners () {
        _storeInFileCheck.selectedProperty().addListener((observable, oldValue, newValue) -> {
            if (newValue) {
                enableFileOptions();
            }
            else {
                disableFileOptions();
            }
        });

        setHandlerForCheckbox(_storeInFileCheck, _storeInFileSetting);
        setHandlerForCheckbox(_reloadOnScrollCheck, _reloadOnScrollSetting);
        setHandlerForCheckbox(_multipleSearchCondCheck, _multipleSearchCondSetting);

        KeyCombination undoComb = new KeyCodeCombination(KeyCode.Z, KeyCombination.CONTROL_DOWN);

        setHandlerForTextBox(_numberOfRecordsInMemoryText, undoComb);

        _numberOfRecordsInMemoryText.textProperty().addListener(event -> {
            if (_numberOfRecordsInMemoryText.getText().isEmpty()) {
                return;
            }

            if (_numberOfRecordsInMemoryText.wasUserChanged()) {
                try {
                    _recordsInMemorySetting.saveCurrentState(Integer.valueOf(_numberOfRecordsInMemoryText.getText()));
                } catch (NumberFormatException ignored) {

                }

                _numberOfRecordsInMemoryText.setUserChanged(false);
                callParentControllerChanges();
            }
        });


        _numberOfRecordsInMemoryText.setTextFormatter(new TextFormatter<>(new IntegerStringConverter(), Integer.valueOf(_numberOfRecordsInMemoryText.getText()), change -> {
            String newText = change.getControlNewText();
            if (newText.matches("\\d*")) {
                return change;
            }
            else {
                return null;
            }
        }));

        _highlightColorPicker.addEventHandler(MouseEvent.MOUSE_CLICKED, event -> {
            _userChangedHighlightColor = true;
        });

        _highlightColorPicker.valueProperty().addListener((observable, oldValue, newValue) -> {
            if (!_userChangedHighlightColor) {
                return;
            }

            if (oldValue.equals(newValue)) {
                return;
            }

            String newColor = String.format("#%02X%02X%02X%02X",
                    (int) (newValue.getRed() * 255),
                    (int) (newValue.getGreen() * 255),
                    (int) (newValue.getBlue() * 255),
                    (int) (newValue.getOpacity() * 255));
            _highlightColorSetting.saveCurrentState(newColor);

            callParentControllerChanges();
        });

        _updatesPerSecondSlider.valueProperty().addListener((observable, oldValue, newValue) -> {
            if (oldValue.equals(newValue)) {
                return;
            }

            _updatesPerSecondSetting.saveCurrentState((double) newValue);
            callParentControllerChanges();
        });
    }

    @Override
    public HashMap<String, Object> getSettings () {
        HashMap<String, Object> settingsHashMap = new HashMap<>();
        settingsHashMap.put(_storeInFileBoolLabel.getText(), _storeInFileSetting.getCurrentState());
        settingsHashMap.put(_reloadOnScrollBoolLabel.getText(), _reloadOnScrollSetting.getCurrentState());
        settingsHashMap.put(_numberOfRecordsInMemoryLabel.getText(), _recordsInMemorySetting.getCurrentState());
        settingsHashMap.put(_highlightColorLabel.getText(), _highlightColorSetting.getCurrentState());
        settingsHashMap.put(_updatesPerSecondLabel.getText(), _updatesPerSecondSetting.getCurrentState());

        return settingsHashMap;
    }

    @Override
    public void setConfigManager (ConfigFileManager fileManager) {
        _fileManager = fileManager;
    }

    public void setMainController (MainSettingsController controller) {
        _mainSettingsController = controller;
    }

    private void callParentControllerChanges () {
        _mainSettingsController.registerChangedController();
    }

    private void createSettingsObjects () {
        _storeInFileSetting = new BooleanSetting();
        _reloadOnScrollSetting = new BooleanSetting();
        _multipleSearchCondSetting = new BooleanSetting();
        _updatesPerSecondSetting = new DoubleSetting();
        _recordsInMemorySetting = new IntegerSetting();
        _highlightColorSetting = new StringSetting();

        _allSettings = new ArrayList<>();
        _allSettings.addAll(Arrays.asList(_storeInFileSetting, _reloadOnScrollSetting, _updatesPerSecondSetting,
                _multipleSearchCondSetting, _recordsInMemorySetting, _highlightColorSetting));
    }

    private void disableFileOptions () {
        _reloadOnScrollCheck.setDisable(true);

        _reloadOnScrollSetting.saveCurrentState(false);
    }

    private void enableFileOptions () {
        _reloadOnScrollCheck.setDisable(false);

        _reloadOnScrollSetting.saveCurrentState(_reloadOnScrollCheck.isSelected());

    }

    private void setDataForSetting (Setting setting) {
        if (setting.equals(_storeInFileSetting)) {
            _storeInFileCheck.setSelected(_storeInFileSetting.getCurrentState());
        }
        else if (setting.equals(_reloadOnScrollSetting)) {
            _reloadOnScrollCheck.setSelected(_reloadOnScrollSetting.getCurrentState());
        }
        else if (setting.equals(_multipleSearchCondSetting)) {
            _multipleSearchCondCheck.setSelected(_multipleSearchCondSetting.getCurrentState());
        }
        else if (setting.equals(_recordsInMemorySetting)) {
            _numberOfRecordsInMemoryText.setText(String.valueOf(_recordsInMemorySetting.getCurrentState()));
        }
        else if (setting.equals(_highlightColorSetting)) {
            _highlightColorPicker.setValue(Color.web(_highlightColorSetting.getCurrentState()));
        }
        else if (setting.equals(_updatesPerSecondSetting)) {
            _updatesPerSecondSlider.setValue(_updatesPerSecondSetting.getCurrentState());
        }

    }

    private void setDefaults () {
        _userChangedHighlightColor = false;
    }

    private void setHandlerForCheckbox (JFXCheckBox checkbox, BooleanSetting relatedSetting) {
        checkbox.addEventHandler(MouseEvent.MOUSE_CLICKED, event -> {
            relatedSetting.saveCurrentState(checkbox.isSelected());

            callParentControllerChanges();
        });
    }

    private void setHandlerForTextBox (IntegerTextField textField, KeyCombination combination) {
        textField.addEventFilter(KeyEvent.KEY_PRESSED, event -> {
            if (combination.match(event)) {
                event.consume();
                return;
            }

            textField.setUserChanged(true);
        });
    }


    private MainSettingsController _mainSettingsController;

    private List<Setting> _allSettings;

    private BooleanSetting _storeInFileSetting;
    private BooleanSetting _reloadOnScrollSetting;
    private BooleanSetting _multipleSearchCondSetting;
    private IntegerSetting _recordsInMemorySetting;
    private DoubleSetting _updatesPerSecondSetting;
    private StringSetting _highlightColorSetting;
    private ConfigFileManager _fileManager;

    private boolean _userChangedHighlightColor;


    @FXML
    private Label _updatesPerSecondLabel;
    @FXML
    private JFXSlider _updatesPerSecondSlider;
    @FXML
    private ColorPicker _highlightColorPicker;
    @FXML
    private Label _storeInFileBoolLabel;
    @FXML
    private Label _reloadOnScrollBoolLabel;
    @FXML
    private Label _numberOfRecordsInMemoryLabel;
    @FXML
    private Label _highlightColorLabel;
    @FXML
    private JFXCheckBox _storeInFileCheck;
    @FXML
    private IntegerTextField _numberOfRecordsInMemoryText;
    @FXML
    private JFXCheckBox _reloadOnScrollCheck;
    @FXML
    private JFXCheckBox _multipleSearchCondCheck;

}
