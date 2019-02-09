package netlogger.controller;

import com.jfoenix.controls.JFXButton;
import com.jfoenix.controls.JFXChipView;
import com.jfoenix.controls.JFXRadioButton;
import com.jfoenix.controls.JFXTreeView;
import javafx.animation.KeyFrame;
import javafx.animation.Timeline;
import javafx.application.Platform;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.DoubleProperty;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.beans.property.SimpleDoubleProperty;
import javafx.collections.ListChangeListener;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.geometry.Insets;
import javafx.geometry.Orientation;
import javafx.scene.Node;
import javafx.scene.control.ListView;
import javafx.scene.control.ScrollBar;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.TextField;
import javafx.scene.layout.HBox;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.scene.text.Text;
import javafx.util.Duration;
import main.DatabrokerAdapter;
import netlogger.TestReceiver;
import netlogger.model.ObservableLimitedList;
import netlogger.model.StyledString;
import netlogger.model.ViewFactory;
import netlogger.model.messages.MeasureFilter;
import netlogger.model.messages.MeasureHandler;
import netlogger.model.settings.SettingField;
import netlogger.model.settings.Updatable;
import netlogger.util.ConfigConstants;
import netlogger.util.MessageMemoryManager;
import netlogger.util.settings.SettingsManager;
import netlogger.util.StylingManager;
import netlogger.view.MessageListView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.util.concurrent.BlockingDeque;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * The messages controller is the main class that controls everything related to the logs.
 */
public class MessagesController implements Updatable
{
    @FXML
    public void initialize () {
        _measureFilter = new MeasureFilter();
        _stylingManager = new StylingManager();
        _messageMemoryManager = new MessageMemoryManager();
        _pausedProperty = new SimpleBooleanProperty(false);
        _rateProperty = new SimpleDoubleProperty(1.0);
        _storedMeasures = new ArrayList<>();
        _maximumMeasuresInMemory = new AtomicInteger();

        FXMLLoader loader = ViewFactory.create("/fxml/SubjectCounter.fxml");
        _subjectCounter = loader.getController();

        _sideArea.getChildren().add(0, loader.getRoot());

        // Initialized to a size of 1 million because we want to show all of the records. If it's over the max limit,
        // they'll be dealt with accordingly.
        _measureQueue = new LinkedBlockingDeque<>(1_000_000);

        _addMeasureTimeline = new Timeline(new KeyFrame(Duration.seconds(1), event -> {
            _measureQueue.drainTo(_storedMeasures);
            addAllMeasures(new ArrayList<>(_storedMeasures));
            _storedMeasures.clear();
        }));

        _addMeasureTimeline.setCycleCount(Timeline.INDEFINITE);
        _addMeasureTimeline.rateProperty().bind(_rateProperty);

        _pausedProperty.addListener((observable, oldValue, newValue) -> {
            if (!newValue) {
                startTimeline();
            }
        });

        _searchField.setOnAction(event -> {
            String searchFieldText = _searchField.getText();
            if (searchFieldText.isEmpty()){
                return;
            }
            _filterField.getChips().add(searchFieldText);
            _searchField.setText("");
        });

        _filterScrollPane.vvalueProperty().addListener((observable, oldValue, newValue) -> {
            if (newValue.doubleValue() == 0){
                _filterField.setPadding(new Insets(20, 0,20,0));
            }
        });

        _filterField.getChips().addListener((ListChangeListener<? super String>) c -> {
            while (c.next()) {
                if (c.wasAdded()) {
                    for (String val : c.getAddedSubList()) {
                        _measureFilter.addNewSearchString(val);
                        _stylingManager.addString(val);
                    }
                }
                else {
                    for (String val : c.getRemoved()) {
                        removeSearchPane(val);
                    }
                }
            }
            Platform.runLater(this::resetView);
        });

        _logListView.setItems(new ObservableLimitedList<>(150));
        _messageListView = new MessageListView(_logListView, _noMessagesText, _listViewStackPane);

        _clearButton.setOnMouseClicked(event -> {
            if (_clearButton.getText().equals("Clear all")) {
                _clearButton.setText("Are you sure?");
                _clearButton.setStyle("-fx-background-color: #da1c1c;");
                _declineClear.setSelected(true);
                _clearHBox.getChildren().addAll(_confirmClear, _declineClear);
            }
            else {
                _clearButton.setText("Clear all");
                _clearButton.setStyle("-fx-background-color: #7399c6;");
                _clearHBox.getChildren().removeAll(_confirmClear, _declineClear);

                if (_confirmClear.isSelected()) {
                    clearView();
                }
            }
        });
        _clearHBox.getChildren().removeAll(_confirmClear, _declineClear);

        SettingsManager.getInstance().getSettingsBridge().addUpdatableClass(this);
    }

    /**
     * Starts the timeline
     */
    public void startTimeline () {
        _addMeasureTimeline.play();
    }

    public void initAdapter(DatabrokerAdapter adapter){
        if (adapter instanceof TestReceiver){
            ((TestReceiver) adapter).setConsumer(string -> _measureQueue.offer(string));
        }
    }


    /**
     * Set all variables that are not related to the view
     * @param measureHandler
     */
    public void init (MeasureHandler measureHandler) {
        NATSTopicsManager natsTopicsManager = new NATSTopicsManager(_topicHierarchyView, this::resetView);
        natsTopicsManager.setMeasureFilter(_measureFilter);

        measureHandler.setMeasureFilter(_measureFilter);
        measureHandler.setStylingManager(_stylingManager);
        measureHandler.setMeasureConsumer(measure -> _measureQueue.offer(measure));
        measureHandler.setTopicsManager(natsTopicsManager);

        new Thread(_subjectCounter, "Subject counter").start();
        measureHandler.setSubjectCounter(_subjectCounter);

        _handler = measureHandler;
    }

    public void findListViewScrollBar () {
        new Thread(() -> {
            boolean scrollbarFound = false;
            while (!scrollbarFound && !_terminationRequested) {
                Set<Node> nodes = _logListView.lookupAll(".scroll-bar");
                for (Node node : nodes) {
                    if (node instanceof ScrollBar) {
                        if (((ScrollBar) node).getOrientation() == Orientation.VERTICAL) {
                            _messageMemoryManager.setListView(_messageListView);
                            _messageMemoryManager.setScrollBar((ScrollBar) node);
                            _messageMemoryManager.initialize();
                            _messageListView.setScrollBar((ScrollBar) node);

                            scrollbarFound = true;
                        }
                    }
                }
                // If this sleep is not here, the memory footprint increase by at least 150MB (250MB sometimes)
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }, "Scrollbar finder").start();
    }


    private void addAllMeasures (List<StyledString> measures) {
        if (!_pausedProperty.get()) {
            Platform.runLater(() -> {
                _messageListView.addAllMessages(new ArrayList<>(measures));
                if (measures.size() > 0) {
                    _messageListView.removeNoMessagesText();
                }
            });
        }
        else {
            _storedMeasures.addAll(measures);
        }
    }

    public BooleanProperty pausedProperty () {
        return _pausedProperty;
    }

    @Override
    public void update (HashMap<String, Object> settings) {
        _maximumMeasuresInMemory.set((int) settings.get(MEASURES_IN_MEMORY));
        _rateProperty.setValue((double) settings.get(UPDATE_SPEED));

        setTotalMessagesInMemory(_maximumMeasuresInMemory.get());
    }

    private void resetView () {
        clearView();
        _messageListView.addNoMessagesText();

        // call elasticsearch search
    }

    /**
     * Request termination for the thread and request termination for other threads
     */
    public void requestTermination () {
        _terminationRequested = true;
        SettingsManager.getInstance().getSettingsBridge().removeUpdatableClass(this);

        _messageMemoryManager.shutdownCalled();
        _subjectCounter.requestTermination();

        _stylingManager.clearValues();
    }

    /**
     * Remove all the list items from the list view
     */
    private void clearView () {
        _messageListView.clearListView();
        _messageListView.addNoMessagesText();
    }

    /**
     * Removes the pane that shows the filters on the top
     *
     * @param filterString String to be removed
     */
    public void removeSearchPane (String filterString) {

        _measureFilter.removeSearchString(filterString);
        _stylingManager.removeString(filterString);
        Platform.runLater(this::resetView);
    }

    public void setTotalMessagesInMemory (int val) {
        _messageListView.setMaximumSize(val);
    }

    public void requestSearchFocus () {
        _searchField.requestFocus();
    }

    public ScrollPane _filterScrollPane;
    public VBox _sideArea;
    private SubjectCounter _subjectCounter;


    private boolean _terminationRequested;
    private BooleanProperty _pausedProperty;
    private MessageListView _messageListView;
    private MeasureHandler _handler;
    private MessageMemoryManager _messageMemoryManager;
    private MeasureFilter _measureFilter;
    private StylingManager _stylingManager;

    private Timeline _addMeasureTimeline;
    private BlockingDeque<StyledString> _measureQueue;
    private List<StyledString> _storedMeasures;

    @SettingField(settingText = MEASURES_IN_MEMORY, configText = ConfigConstants.MEMORY_STORAGE_SIZE, defaultValue = "150")
    private AtomicInteger _maximumMeasuresInMemory ;

    @SettingField(settingText = UPDATE_SPEED, configText = ConfigConstants.FRAMES_PER_SECOND, defaultValue = "1.0")
    private DoubleProperty _rateProperty;

    private static final String MEASURES_IN_MEMORY = "Number of records to store in memory";
    private static final String UPDATE_SPEED = "Updates per second";

    @FXML private JFXChipView<String> _filterField;

    @FXML
    private Text _noMessagesText;
    @FXML
    private StackPane _listViewStackPane;
    @FXML
    private HBox _clearHBox;
    @FXML
    private JFXRadioButton _declineClear;
    @FXML
    private JFXRadioButton _confirmClear;
    @FXML
    private JFXButton _clearButton;
    @FXML
    private TextField _searchField;
    @FXML
    private ListView<StyledString> _logListView;
    @FXML
    private JFXTreeView<String> _topicHierarchyView;
    @FXML
    private VBox _subjectNames;
}