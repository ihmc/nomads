package netlogger;

import com.jfoenix.controls.*;
import javafx.application.Platform;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyCodeCombination;
import javafx.scene.input.KeyCombination;
import javafx.scene.layout.*;
import javafx.stage.Modality;
import javafx.stage.Stage;
import main.DatabrokerAdapter;
import netlogger.controller.ContactAlertController;
import netlogger.controller.KibanaViewController;
import netlogger.controller.MessagesController;
import netlogger.controller.ToolbarController;
import netlogger.controller.elasticsearch.ElasticsearchController;
import netlogger.controller.settings.MainSettingsController;
import netlogger.controller.tracking.TrackingController;
import netlogger.model.MessageReceiver;
import netlogger.model.ViewFactory;
import netlogger.model.messages.MeasureHandler;
import netlogger.util.ConfigConstants;
import netlogger.util.ConfigFileManager;
import netlogger.util.ConfigInjector;
import netlogger.util.KeyCodeToModifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import static javafx.scene.input.KeyCombination.Modifier;
import static javafx.scene.input.MouseEvent.MOUSE_CLICKED;
import static javafx.scene.input.MouseEvent.MOUSE_PRESSED;

/**
 * The main loading class for the NetLogger. Instantiates controllers and the main view.
 */
public class NetLogger implements ConfigInjector
{

    public void closeSettings () {
        if (_alert != null){
            _alert.hideWithAnimation();
        }
    }

    public void createSettings () {
        _settingsViewControllerLoader = ViewFactory.create("/fxml/settings/SettingsView.fxml");
        _mainSettingsController = _settingsViewControllerLoader.getController();
        _mainSettingsController.setNetLogger(this);
    }

    /**
     * Used to get the MainView Anchor Pane so the view can be put wherever
     * @return
     */
    public Parent getAnchorPaneAsParent () {
        return _mainContainerAnchorPane;
    }

    /**
     * Used in the NetViewer to determine if the NATS connection is open, don't remove
     * @return
     */
    public DatabrokerAdapter getDatabrokerAdapter () {
        return _databrokerAdapter;
    }

    @FXML
    public void initialize () {

        new Thread(() -> {
            while (true) {
                _logger.trace("TRACE");
                _logger.debug("DEBUG");
                _logger.info("INFO");
                _logger.warn("WARN");
                _logger.error("ERROR");
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }).start();

        _messagesControllerLoader = ViewFactory.create("/fxml/MessagesView.fxml");
        _messagesController = _messagesControllerLoader.getController();

        _kibanaViewControllerLoader = ViewFactory.create("/fxml/KibanaView.fxml");
        _kibanaViewController = _kibanaViewControllerLoader.getController();

        _elasticsearchViewControllerLoader = ViewFactory.create("/fxml/ElasticsearchView.fxml");
        _elasticsearchManager = _elasticsearchViewControllerLoader.getController();

        _logVisualizationControllerLoader = ViewFactory.create("/fxml/LogTracking.fxml");
        _trackingController = _logVisualizationControllerLoader.getController();
        _trackingController.pauseView();

        _toolbarController.getNavigationDrawerController().setChangePaneHandler(MOUSE_PRESSED, event -> {
            if (((Label)event.getSource()).getText().equalsIgnoreCase("elasticsearch")){
                showElasticsearch();
            }
            else if (((Label)event.getSource()).getText().equalsIgnoreCase("logs")){
                showLog();
            }
            else if (((Label)event.getSource()).getText().equalsIgnoreCase("tracking")){
                showTracking();
            }
            else if (((Label)event.getSource()).getText().equalsIgnoreCase("kibana")){
                showKibana();
            }
        });

        _toolbarController.getInfoPopupController().getContactInfoLabel().addEventFilter(MOUSE_CLICKED, event -> {
                JFXAlert alert = new JFXAlert(_primaryStage);
                alert.initModality(Modality.APPLICATION_MODAL);
                alert.setOverlayClose(false);
                FXMLLoader loader = ViewFactory.create("/fxml/toolbar/ContactAlert.fxml");

                alert.setContent((JFXDialogLayout)loader.getRoot());
                ((ContactAlertController)loader.getController()).setCallback(event1 -> alert.hideWithAnimation());

                alert.show();
        });

        _toolbarController.getInfoPopupController().getSettingsLabel().addEventFilter(MOUSE_CLICKED, event -> {
            _alert = new JFXAlert(_primaryStage);
            _alert.initModality(Modality.APPLICATION_MODAL);
            _alert.setOverlayClose(false);
            _alert.setSize(700, 600);

            BorderPane content = _settingsViewControllerLoader.getRoot();
            _alert.setContent(content);
            _alert.show();
        });

        AnchorPane view = _messagesControllerLoader.getRoot();
        _mutablePane.getChildren().setAll(view);

        MeasureHandler measureHandler = new MeasureHandler();
        measureHandler.setESController(_elasticsearchManager);
        measureHandler.setTrackingController(_trackingController);

        _toolbarController.setDrawersStack(_navDrawerStack);

        _messagesController.init(measureHandler);

        // Initialize a MessageReceiver type as the data broker adapter
        MessageReceiver receiver = new MessageReceiver();
        receiver.setHandler(measureHandler);

//        TestReceiver receiver = new TestReceiver();
        _databrokerAdapter = receiver;

//        _messagesController.initAdapter(_databrokerAdapter);

        _messagesShowing = new SimpleBooleanProperty(true);
        _trackingShowing = new SimpleBooleanProperty(false);

        _trackingController.pausedProperty().bind(_trackingShowing.not());
        _messagesController.pausedProperty().bind(_messagesShowing.not());

        _mainContainerAnchorPane.heightProperty().addListener((observable, oldValue, newValue) -> {
            _mutablePane.setPrefHeight(newValue.doubleValue());
            if (_settingsViewControllerLoader == null) {
                return;
            }
            ((Region) _settingsViewControllerLoader.getRoot()).setMaxHeight(newValue.doubleValue() / 1.5);
        });

        _messagesController.findListViewScrollBar();
    }

    /**
     * Create a stage for the anchorpane to use
     */
    public void initializeAsStandalone () {
        _primaryStage = new Stage();
        Parent root = getAnchorPaneAsParent();
        Scene scene = new Scene(root);

        // Set up the stage
        _primaryStage.setTitle("NetLogger");
        _primaryStage.setScene(scene);
        _primaryStage.show();

        _primaryStage.setOnHidden(event -> {
            shutdown();
            Platform.exit();
        });
    }

    @Override
    public void setConfigManager (ConfigFileManager fileManager) {
        _fileManager = fileManager;
        setNatsDataFromConfigFile();

        _mainSettingsController.setConfigManager(fileManager);
    }

    /**
     * Set the NATS configuration data
     *
     * @param natsIP   The NATS server's IP address to connect to
     * @param natsPort The NATS port on the server to connect to
     * @param topics   The list of topics
     */
    public void setNatsData (String natsIP, String natsPort, List<String> topics) {
        _databrokerAdapter.setInfo(natsIP, natsPort, topics);
        _databrokerAdapter.connect();
    }

    /**
     * Stop threads on the controllers. Should be called when the creating class/application determines it's time
     * to get rid of NetLogger.
     */
    public void shutdown () {
        _messagesController.requestTermination();
        _databrokerAdapter.requestTermination();
        _elasticsearchManager.requestTermination();
    }

    /**
     * Start controller threads, should be called by the creating class of NetLogger
     */
    public void startThreads () {
        new Thread(_databrokerAdapter, "Message receiver").start();
        _messagesController.startTimeline();
    }

    private void setNatsDataFromConfigFile () {
        String natsHost = _fileManager.getStringValue(ConfigConstants.NATS_HOST);
        String natsPort = _fileManager.getStringValue(ConfigConstants.NATS_PORT);
        List<String> topics = new ArrayList<>();
        StringTokenizer st = new StringTokenizer(_fileManager.getStringValue(ConfigConstants.NATS_TOPICS), " ");
        while (st.hasMoreTokens()) {
            topics.add(st.nextToken());
        }

        setNatsData(natsHost, natsPort, topics);
    }

    /**
     * Show Elasticsearch view and change button css
     */
    private void showElasticsearch () {
        StackPane view = _elasticsearchViewControllerLoader.getRoot();
        _mutablePane.getChildren().setAll(view);

        _trackingShowing.setValue(false);
        _messagesShowing.setValue(false);
    }

    private void showKibana () {
        StackPane view = _kibanaViewControllerLoader.getRoot();
        _mutablePane.getChildren().setAll(view);

        _trackingShowing.setValue(false);
        _messagesShowing.setValue(false);

    }

    /**
     * Show the Log view and change button css
     */
    private void showLog () {
        AnchorPane view = _messagesControllerLoader.getRoot();
        _mutablePane.getChildren().setAll(view);

        _trackingShowing.setValue(false);
        _messagesShowing.setValue(true);

    }

    private void showTracking () {
        if (_trackingController == null) {
            return;
        }
        _trackingShowing.setValue(true);
        _messagesShowing.setValue(false);

        StackPane view = _logVisualizationControllerLoader.getRoot();
        _mutablePane.getChildren().setAll(view);
    }

    private JFXAlert _alert;


    @FXML private JFXDrawersStack _navDrawerStack;
    @FXML private ToolbarController _toolbarController;

    @FXML private VBox _toolbar;

    private MessagesController _messagesController;
    private ElasticsearchController _elasticsearchManager;
    private MainSettingsController _mainSettingsController;
    private KibanaViewController _kibanaViewController;
    private TrackingController _trackingController;
    private DatabrokerAdapter _databrokerAdapter;

    private Stage _primaryStage;
    private FXMLLoader _messagesControllerLoader;
    private FXMLLoader _kibanaViewControllerLoader;
    private FXMLLoader _settingsViewControllerLoader;
    private FXMLLoader _elasticsearchViewControllerLoader;
    private FXMLLoader _logVisualizationControllerLoader;

    private ConfigFileManager _fileManager;

    private BooleanProperty _messagesShowing;
    private BooleanProperty _trackingShowing;

    @FXML
    private AnchorPane _mainContainerAnchorPane;
    @FXML
    private Pane _mutablePane;

    private static final Logger _logger = LoggerFactory.getLogger(NetLogger.class);
}
