package netlogger.controller.settings;

import com.jfoenix.controls.JFXAlert;
import com.jfoenix.controls.JFXButton;
import com.jfoenix.controls.JFXDialogLayout;
import com.jfoenix.controls.JFXTextField;
import javafx.event.Event;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.scene.Node;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.scene.layout.BorderPane;
import javafx.stage.Modality;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import nats.NatsTopicsChooser;
import nats.NatsTopicsHelper;
import netlogger.controller.ContactAlertController;
import netlogger.model.settings.ListSetting;
import netlogger.model.settings.Setting;
import netlogger.model.settings.StringSetting;
import netlogger.util.ConfigConstants;
import netlogger.util.ConfigFileManager;
import netlogger.util.IPv4AddressValidator;
import netlogger.view.SelfBalancingGridPane;

import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.*;

public class NATSSettingsController implements SettingsController
{
    @FXML
    public void initialize () {
        createSettingsObjects();
        initializeListeners();

        _natsTopicGridPane.setMaxColumns(2);

        _addNewTopicBtn.setOnMouseClicked(event -> {
            createNewTopicField("");
            callParentControllerChanges();
        });

        createTopicsListObjects();

        _validator = new IPv4AddressValidator();
    }

    /**
     * Creates objects for the NATS Topic chooser
     */
    private void createTopicsListObjects () {
        FXMLLoader topicsListLoader = new FXMLLoader();
        topicsListLoader.setLocation(getClass().getResource("/nats/NATSTopicsPane.fxml"));
        try {
            topicsListLoader.load();
        } catch (IOException e) {
            e.printStackTrace();
        }

        _chooser = topicsListLoader.getController();
        String fileLocation = getClass().getResource("/nats/NATSTopics.json").getFile().replace("%20", " ");

        _chooser.parseTopicsFromPath(fileLocation);
        _chooser.addTopicsToView();

        _topicListBtn.setOnMouseClicked(event -> {
            List<String> topics = getTopics();
            _chooser.defaultSelectTopics(topics);

            JFXAlert alert = new JFXAlert();
            alert.initModality(Modality.APPLICATION_MODAL);
            alert.setOverlayClose(false);

            alert.setContent((BorderPane)topicsListLoader.getRoot());

            alert.show();

            _chooser.setButtonListener(event1 -> {
                List<String> chosenTopics = _chooser.getChosenTopics();

                removeMissingTopics(chosenTopics);

                chosenTopics.removeAll(findDuplicateTopics(chosenTopics));

                chosenTopics.forEach(this::createNewTopicField);

                callParentControllerChanges();

                alert.hideWithAnimation();
            });
        });
    }

    private List<String> findDuplicateTopics (List<String> topics) {
        List<String> possiblyUpdatedTopics = getTopics();
        List<String> duplicateTopics = new ArrayList<>();

        topics.forEach(topic -> {
            possiblyUpdatedTopics.forEach(currentTopics -> {
                if (NatsTopicsHelper.compareTopics(currentTopics, topic)) {
                    duplicateTopics.add(topic);
                }
            });
        });

        return duplicateTopics;
    }

    private void removeMissingTopics (List<String> topics) {
        List<String> currentTopics = getTopics();
        List<String> topicsToRemoveFromGrid = new ArrayList<>();

        currentTopics.forEach(topicInGrid -> {
            boolean topicIsInList = false;
            for (String topicFromList : topics) {
                if (topicInGrid.equals(topicFromList)) {
                    topicIsInList = true;
                    break;
                }
            }
            if (!topicIsInList) {
                topicsToRemoveFromGrid.add(topicInGrid);
            }
        });

        removeTopicsFromGrid(topicsToRemoveFromGrid);
    }

    private void removeTopicsFromGrid (List<String> topics) {
        List<Node> topicChildren = _natsTopicGridPane.getChildren();
        List<Node> removableChildren = new ArrayList<>();
        topics.forEach(s -> {
            for (Node topicField : topicChildren) {
                if (topicField instanceof JFXTextField) {
                    if (((JFXTextField) topicField).getText().equals(s) || ((JFXTextField) topicField).getText().equals("")) {
                        removableChildren.add(topicField);
                    }
                }
            }
        });

        Node[] nodes = new Node[removableChildren.size()];
        for (int i = 0; i < removableChildren.size(); i++) {
            nodes[i] = removableChildren.get(i);
        }

        _natsTopicGridPane.remove(nodes);
        checkToDisableAddButton();
    }

    @Override
    public void setConfigManager (ConfigFileManager fileManager) {
        _fileManager = fileManager;
    }

    private boolean gridPaneHasNoEmptyFields () {
        List<Node> topicChildren = _natsTopicGridPane.getChildren();
        for (Node topicField : topicChildren) {
            if (topicField instanceof JFXTextField) {
                String topic = ((JFXTextField) topicField).getText();
                if (topic.equals("")) {
                    return false;
                }
            }
        }
        return true;
    }

    public void setMainController (MainSettingsController controller) {
        _mainSettingsController = controller;
    }


    public void initializeListeners () {
        _natsPortTxt.textProperty().addListener((observable, oldValue, newValue) -> {
            _natsPortSetting.saveCurrentState(_natsPortTxt.getText());
            callParentControllerChanges();
        });

        _natsIPTxt.textProperty().addListener((observable, oldValue, newValue) -> {
            _natsIPSetting.saveCurrentState(_natsIPTxt.getText());
            callParentControllerChanges();
        });
    }

    /**
     * Update NATS data in the NetLogger
     */
    public void updateNatsData () {
        setHostIP();
        _natsPortSetting.saveCurrentState(_natsPortTxt.getText());

        _topics = getTopics();
    }

    private List<String> getTopics () {
        List<String> topics = new ArrayList<>();
        List<Node> topicChildren = _natsTopicGridPane.getChildren();
        for (Node topicField : topicChildren) {
            if (topicField instanceof JFXTextField) {
                String topic = ((JFXTextField) topicField).getText();
                if (!topic.equals("")) {
                    topics.add(topic);
                }
            }
        }

        return topics;
    }

    private void createSettingsObjects () {
        _natsIPSetting = new StringSetting();
        _natsPortSetting = new StringSetting();
        _topicsSetting = new ListSetting();

        _allSettings = new ArrayList<>();
        _allSettings.addAll(Arrays.asList(_natsIPSetting, _natsPortSetting, _topicsSetting));
    }

    private void setHostIP () {
        if (_validator.validateIP(_natsIPTxt.getText())) {
            _natsIPSetting.saveCurrentState(_natsIPTxt.getText());
        }
        else {
            try {
                InetAddress address = Inet4Address.getByName(_natsIPTxt.getText());
                _natsIPSetting.saveCurrentState(address.getHostAddress());

            } catch (UnknownHostException e) {
                e.printStackTrace();
            }
        }
    }

    private void createNewTopicField (String prefilledText) {
        JFXTextField newTopicField = new JFXTextField(prefilledText);
        newTopicField.textProperty().addListener((observable, oldValue, newValue) -> {
            checkToDisableAddButton();
        });

        newTopicField.setMaxWidth(150);
        _natsTopicGridPane.add(newTopicField);
        checkToDisableAddButton();
        if (_addNewTopicBtn.isDisabled()) {
            newTopicField.requestFocus();
        }
    }

    private void checkToDisableAddButton () {
        if (gridPaneHasNoEmptyFields()) {
            _addNewTopicBtn.setDisable(false);
        }
        else {
            _addNewTopicBtn.setDisable(true);
        }
    }

    private void callParentControllerChanges () {
        _mainSettingsController.registerChangedController();
    }


    private void setDataForSetting (Setting setting) {
        if (setting instanceof ListSetting) {
            List<String> topics = ((ListSetting) setting).getCurrentState();
            _natsTopicGridPane.getChildren().clear();
            for (String topic : topics) {
                createNewTopicField(topic);
            }
        }

        else if (setting instanceof StringSetting) {
            if (setting.equals(_natsIPSetting)) {
                _natsIPTxt.setText(_natsIPSetting.getCurrentState());
            }
            else if (setting.equals(_natsPortSetting)) {
                _natsPortTxt.setText(_natsPortSetting.getCurrentState());
            }
        }
    }


    @Override
    public void saveToConfig () {
        List<String> topics = new ArrayList<>();
        List<Node> topicChildren = _natsTopicGridPane.getChildren();
        for (Node topicField : topicChildren) {
            if (topicField instanceof JFXTextField) {
                String topic = ((JFXTextField) topicField).getText();
                if (!topic.equals("")) {
                    topics.add(topic);
                }
            }
        }
        _topicsSetting.saveCurrentState(topics);


        StringBuilder builder = new StringBuilder();
        for (String string : _topicsSetting.getCurrentState()) {
            builder.append(string).append(" ");
        }

        _fileManager.updateConfig(ConfigConstants.NATS_HOST, _natsIPSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.NATS_PORT, _natsPortSetting.getCurrentState());
        _fileManager.updateConfig(ConfigConstants.NATS_TOPICS, builder.toString());

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
        List<String> originalTopics = new ArrayList<>();
        StringTokenizer st = new StringTokenizer(_fileManager.getStringValue(ConfigConstants.NATS_TOPICS), " ");
        while (st.hasMoreTokens()) {
            String topic = st.nextToken();
            createNewTopicField(topic);
            originalTopics.add(topic);
        }

        String natsIP = _fileManager.getStringValue(ConfigConstants.NATS_HOST);
        String natsPort = _fileManager.getStringValue(ConfigConstants.NATS_PORT);

        _natsPortTxt.setText(natsPort);
        _natsIPTxt.setText(natsIP);

        _natsIPSetting.saveOriginalState(natsIP);
        _natsPortSetting.saveOriginalState(natsPort);
        _topicsSetting.saveOriginalState(originalTopics);


        _natsIPSetting.saveCurrentState(natsIP);
        _natsPortSetting.saveCurrentState(natsPort);
        _topicsSetting.saveCurrentState(originalTopics);
    }

    @Override
    public HashMap<String, Object> getSettings () {
        updateNatsData();
        HashMap<String, Object> settingsHashMap = new HashMap<>();
        settingsHashMap.put(_natsHostLabel.getText(), _natsIPSetting.getCurrentState());
        settingsHashMap.put(_natsPortLabel.getText(), _natsPortSetting.getCurrentState());
        settingsHashMap.put(_topicsLabel.getText(), _topics);

        return settingsHashMap;
    }

    private List<String> _topics;
    private IPv4AddressValidator _validator;
    private MainSettingsController _mainSettingsController;
    private ConfigFileManager _fileManager;
    private NatsTopicsChooser _chooser;

    private List<Setting> _allSettings;

    private StringSetting _natsIPSetting;
    private StringSetting _natsPortSetting;
    private ListSetting _topicsSetting;

    @FXML
    private JFXButton _topicListBtn;
    @FXML
    private JFXTextField _natsPortTxt;
    @FXML
    private JFXTextField _natsIPTxt;
    @FXML
    private JFXButton _addNewTopicBtn;
    @FXML
    private SelfBalancingGridPane _natsTopicGridPane;
    @FXML
    private Label _topicsLabel;
    @FXML
    private Label _natsHostLabel;
    @FXML
    private Label _natsPortLabel;
}
