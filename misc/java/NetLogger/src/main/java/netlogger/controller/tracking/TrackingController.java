package netlogger.controller.tracking;

import javafx.application.Platform;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.fxml.FXML;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.ColorPicker;
import javafx.scene.control.ScrollPane;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.scene.shape.Circle;
import javafx.scene.text.Text;
import netlogger.model.messages.MeasureHandler;
import netlogger.model.timeseries.DefaultColumnNames;
import netlogger.model.timeseries.TrackingMeasureTimeSeries;
import netlogger.model.timeseries.TrackingMeasureTimeSeriesItem;
import netlogger.model.tracking.CircleColorByType;
import netlogger.model.tracking.TrackingMeasure;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Map;
import java.util.function.Consumer;

/**
 * The purpose of the tracking controller is to receive {@link TrackingMeasure} data from the {@link MeasureHandler} and
 * delegate which application controller should take note of a received/sent data
 */
public class TrackingController
{
    @FXML
    public void initialize () {
        _applicationLevelTracking = new HashMap<>();
        _timeSeries = new TrackingMeasureTimeSeries();
        _pausedProperty = new SimpleBooleanProperty(true);

        _trackingPathCalculator = new TrackingPathCalculator();
        _trackingObjectPoolManager = new TrackingObjectPoolManager();
        _viewScrollPane.setStyle("-fx-focus-color: #0000;\n" +
                "-fx-faint-focus-color: 0000;\n" +
                "-fx-background-color:transparent;");

        _colorScrollPane.setStyle("-fx-focus-color: 0000;\n" +
                "-fx-faint-focus-color: 0000;\n" +
                "-fx-background-color:transparent;");
        StackPane.setAlignment(_colorScrollPane, Pos.TOP_RIGHT);
        StackPane.setAlignment(_viewScrollPane, Pos.CENTER);
        CircleColorByType.getInstance().addListener(change -> {
            if (change.wasRemoved()) {
                return;
            }
            Platform.runLater(() -> {
                String typeName = change.getKey();
                Color value = change.getValueAdded();

                HBox hBox = new HBox();

                ColorPicker colorPicker = new ColorPicker(value);
                colorPicker.setStyle("-fx-background-color: transparent;");
                colorPicker.setMaxWidth(40);
                HBox.setMargin(colorPicker, new Insets(-2, 0, 0, 0));

                colorPicker.setOnAction(event -> {
                    Color c = colorPicker.getValue();
                    CircleColorByType.getInstance().updateColorForType(typeName, c);
                });

                hBox.getChildren().addAll(colorPicker, new Text(typeName));
                _colorVBox.getChildren().addAll(hBox);
            });

        });

        _trackingAnimator = new TrackingAnimator(_trackingObjectPoolManager);
        _trackingAnimator.pausedPropertyProperty().bind(_pausedProperty);

        Consumer<Circle> newCircleListener = circle -> Platform.runLater(() -> {
            _mainView.getChildren().add(circle);
        });

        _trackingObjectPoolManager.addListener(newCircleListener);

    }

    public BooleanProperty pausedProperty () {
        return _pausedProperty;
    }

    public void setTrackingConfigFile (String file) {
        _trackingPathCalculator.setTrackingConfigPath(file);
    }

    public void handleDataTrackingMeasure (TrackingMeasure trackingMeasure) {
        _timeSeries.add(generateTimeSeriesItem(trackingMeasure));

        String type = trackingMeasure.getType();
        if (type == null || type.equals("null")) {
            type = "unknown";
        }

        Color colorForType = CircleColorByType.getInstance().checkType(type);

        String sourceApplicationName = trackingMeasure.getSourceApplication();
        String destApplicationName = trackingMeasure.getDestApplication();
        String sourceClient = trackingMeasure.getSourceClientID();
        String destClient = trackingMeasure.getDestClientID();

        // These are inter-application sending, so for now we won't do anything
        // In the future, we will update the TrackingClientController with which module
        // in the application this should be sent to
        if (sourceClient.equals(destClient)) {
            return;
        }

        TrackingClientController sourceApplication = _applicationLevelTracking.get(sourceClient);
        TrackingClientController destApplication = _applicationLevelTracking.get(destClient);

        if (sourceApplication == null) {
            sourceApplication = createNewController(sourceApplicationName, sourceClient);
            _applicationLevelTracking.put(sourceClient, sourceApplication);
        }

        if (destApplication == null) {
            destApplication = createNewController(destApplicationName, destClient);
            _applicationLevelTracking.put(destClient, destApplication);
        }

        _trackingAnimator.addPath(_trackingPathCalculator.getPath(sourceApplicationName, destApplicationName), colorForType);
    }

    private TrackingMeasureTimeSeriesItem generateTimeSeriesItem (TrackingMeasure measure) {
        Map<String, String> column = new HashMap<>();
        column.put(DefaultColumnNames.SOURCE_APPLICATION_STRING, measure.getSourceApplication());
        column.put(DefaultColumnNames.SOURCE_MODULE_STRING, measure.getSourceModule());
        column.put(DefaultColumnNames.DEST_APPLICATION_STRING, measure.getDestApplication());
        column.put(DefaultColumnNames.DEST_MODULE_STRING, measure.getDestModule());
        column.put(DefaultColumnNames.PREV_DATA_ID_STRING, measure.getPrevLogUUID());
        column.put(DefaultColumnNames.CURR_DATA_ID_STRING, measure.getCurrLogUUID());
        column.put(DefaultColumnNames.CHECKSUM_STRING, measure.getChecksum());
        column.put(DefaultColumnNames.DATA_TYPE_STRING, measure.getType());
        column.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, measure.getSourceClientID());
        column.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, measure.getDestClientID());

        return new TrackingMeasureTimeSeriesItem(column, measure.getMeasureTimestamp().getSeconds() * 1000);
    }

    /**
     * Pause updating the view in the tracking application. Data can and should be moved between applications/modules
     * but there is no need to play animations if the view isn't showing
     */
    public void pauseView () {
        //_canRun = false;
    }

    /**
     * Let the views startTimeline updating
     */
    public void unpauseView () {
        //_canRun = true;
    }

    private TrackingClientController createNewController (String applicationName, String clientID) {
        TrackingClientController controller = new TrackingClientController(applicationName, clientID, _timeSeries);
        controller.getModuleView().setStyle("-fx-border-color:#f00");

        Platform.runLater(() -> {
            int column = _viewsAdded / _maxVertical;
            int row = _viewsAdded % _maxVertical;
            double paddingTop = 10;
            double paddingSide = 10;
            if (row != 0) {
                paddingTop = 100;
            }
            if (column != 0) {
                paddingSide = 100;
            }

            double positionTop = (controller.getModuleView().getMaxHeight() + paddingTop) * row;
            double positionLeft = (controller.getModuleView().getMaxWidth() + paddingSide) * column;

            _trackingPathCalculator.addNewModuleView(controller.getModuleView(), clientID);
            _mainView.getChildren().add(controller.getModuleView());
            _viewsAdded++;

            AnchorPane.setTopAnchor(controller.getModuleView(), positionTop);
            AnchorPane.setLeftAnchor(controller.getModuleView(), positionLeft);

        });

        return controller;
    }

    @FXML
    public ScrollPane _viewScrollPane;

    @FXML
    private ScrollPane _colorScrollPane;
    @FXML
    private AnchorPane _mainView;
    @FXML
    private VBox _colorVBox;

    private BooleanProperty _pausedProperty;

    private int _maxVertical = 3;
    private int _viewsAdded = 0;
    private TrackingMeasureTimeSeries _timeSeries;
    private TrackingAnimator _trackingAnimator;

    private TrackingPathCalculator _trackingPathCalculator;
    private TrackingObjectPoolManager _trackingObjectPoolManager;
    private HashMap<String, TrackingClientController> _applicationLevelTracking;
    private static final Logger _logger = LoggerFactory.getLogger(TrackingController.class);
}
