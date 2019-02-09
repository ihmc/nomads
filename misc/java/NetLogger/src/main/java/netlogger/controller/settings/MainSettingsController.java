package netlogger.controller.settings;

import com.jfoenix.controls.JFXButton;
import com.jfoenix.controls.JFXTreeView;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.scene.control.Button;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.TreeItem;
import javafx.scene.layout.AnchorPane;
import netlogger.NetLogger;
import netlogger.util.settings.SettingsManager;
import netlogger.model.ViewFactory;
import netlogger.util.ConfigFileManager;

import java.util.HashMap;
import java.util.Map;

/**
 * The MainSettingsController controls the settings pane's functionality
 */
public class MainSettingsController implements SettingsController
{
    @FXML
    public void initialize () {
        initializeViewLoaders();

        _settingsControllerAnchorPaneMap = new HashMap<>();

        NATSSettingsController natsSettingsController = _natsViewLoader.getController();
        GeneralSettingsController generalSettingsController = _generalViewLoader.getController();
        ElasticsearchSettingsController elasticsearchSettingsController = _elasticsearchViewLoader.getController();

        _settingsControllerAnchorPaneMap.put(natsSettingsController, _natsSettingsAnchorPane);
        _settingsControllerAnchorPaneMap.put(generalSettingsController, _generalSettingsAnchorPane);
        _settingsControllerAnchorPaneMap.put(elasticsearchSettingsController, _elasticsearchSettingsAnchorPane);

        generalSettingsController.setMainController(this);
        natsSettingsController.setMainController(this);
        elasticsearchSettingsController.setMainController(this);

        TreeItem<String> navigationRoot = new TreeItem<>("root");
        _settingTypesTreeView.setRoot(navigationRoot);

        TreeItem<String> general = new TreeItem<>("General");
        navigationRoot.getChildren().add(general);

        TreeItem<String> nats = new TreeItem<>("NATS");
        navigationRoot.getChildren().add(nats);

        TreeItem<String> elasticsearch = new TreeItem<>("Elasticsearch");
        navigationRoot.getChildren().add(elasticsearch);

        _settingTypesTreeView.getSelectionModel().selectedItemProperty().addListener((observable, oldValue, newValue) -> {
            if (newValue.equals(nats)) {
                setPaneContent(_natsSettingsAnchorPane);
            }

            if (newValue.equals(general)) {
                setPaneContent(_generalSettingsAnchorPane);
            }

            if (newValue.equals(elasticsearch)) {
                setPaneContent(_elasticsearchSettingsAnchorPane);
            }

        });

        _revertBtn.setDisable(true);

        _settingTypesTreeView.getSelectionModel().select(0);
    }


    private void initializeViewLoaders () {
        _natsViewLoader = ViewFactory.create("/fxml/settings/NatsSettingsView.fxml");
        _generalViewLoader = ViewFactory.create("/fxml/settings/GeneralSettingsView.fxml");
        _elasticsearchViewLoader = ViewFactory.create("/fxml/settings/ElasticsearchSettingsView.fxml");

        _natsSettingsAnchorPane = _natsViewLoader.getRoot();
        _generalSettingsAnchorPane = _generalViewLoader.getRoot();
        _elasticsearchSettingsAnchorPane = _elasticsearchViewLoader.getRoot();
    }

    public void setNetLogger (NetLogger netLogger) {
        _netLogger = netLogger;
    }

    public void registerChangedController () {
        _revertBtn.setDisable(false);
    }

    private void setPaneContent (AnchorPane content) {
        _mainSettingsScrollPane.setContent(content);
    }

    private void setClassSettings () {
        SettingsManager.getInstance().getSettingsBridge().updateAsync(getSettings());
    }

    @Override
    public void saveToConfig () {
        for (SettingsController settingsController : _settingsControllerAnchorPaneMap.keySet()) {
            settingsController.saveToConfig();
        }
        _revertBtn.setDisable(true);

        setClassSettings();
    }

    @Override
    public void setConfigManager (ConfigFileManager fileManager) {
        for (SettingsController settingsController : _settingsControllerAnchorPaneMap.keySet()) {
            settingsController.setConfigManager(fileManager);
        }

        initializeFromConfigFile();
    }

    @Override
    public void revertAll () {
        for (SettingsController settingsController : _settingsControllerAnchorPaneMap.keySet()) {
            settingsController.revertAll();
        }
        _revertBtn.setDisable(true);
    }

    @Override
    public void initializeFromConfigFile () {
        for (SettingsController settingsController : _settingsControllerAnchorPaneMap.keySet()) {
            settingsController.initializeFromConfigFile();
        }

        initializeListeners();
    }

    @Override
    public void initializeListeners () {

        for (SettingsController settingsController : _settingsControllerAnchorPaneMap.keySet()) {
            settingsController.initializeListeners();
        }

        _exitButton.setOnMouseClicked(event -> {
            _netLogger.closeSettings();
        });

        _applyBtn.setOnMouseClicked(event -> {
            saveToConfig();
        });

        _revertBtn.setOnMouseClicked(event -> {
            revertAll();
        });

        _saveCloseBtn.setOnMouseClicked(event -> {
            saveToConfig();
            _netLogger.closeSettings();
        });

        setClassSettings();
    }

    @Override
    public HashMap<String, Object> getSettings () {
        HashMap<String, Object> settingsHashMap = new HashMap<>();
        for (SettingsController settingsController : _settingsControllerAnchorPaneMap.keySet()) {
            settingsHashMap.putAll(settingsController.getSettings());
        }

        return settingsHashMap;
    }


    private AnchorPane _natsSettingsAnchorPane;
    private AnchorPane _generalSettingsAnchorPane;
    private AnchorPane _elasticsearchSettingsAnchorPane;
    private Map<SettingsController, AnchorPane> _settingsControllerAnchorPaneMap;
    private FXMLLoader _natsViewLoader;
    private FXMLLoader _generalViewLoader;
    private FXMLLoader _elasticsearchViewLoader;
    private NetLogger _netLogger;


    @FXML
    private JFXButton _applyBtn;
    @FXML
    private JFXButton _revertBtn;
    @FXML
    private JFXButton _saveCloseBtn;
    @FXML
    private Button _exitButton;
    @FXML
    private ScrollPane _mainSettingsScrollPane;
    @FXML
    private JFXTreeView<String> _settingTypesTreeView;
}