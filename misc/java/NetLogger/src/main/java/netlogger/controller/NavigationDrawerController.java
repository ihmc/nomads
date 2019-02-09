package netlogger.controller;

import com.jfoenix.controls.JFXDrawer;
import javafx.event.EventHandler;
import javafx.event.EventType;
import javafx.fxml.FXML;
import javafx.scene.Node;
import javafx.scene.control.Label;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.VBox;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @author Blake Ordway (bordway@ihmc.us) on 8/27/2018
 */
public class NavigationDrawerController
{
    @FXML
    public void initialize () {
        _drawer.setSidePane(_vBox);
        _drawer.setDefaultDrawerSize(150);
        _drawer.setResizeContent(false);
        _drawer.setOverLayVisible(true);
        _drawer.setResizableOnDrag(false);

        addLabelHandler(_elasticsearchLabel);
        addLabelHandler(_kibanaLabel);
        addLabelHandler(_logLabel);
        addLabelHandler(_logVisualizationLabel);

        // Init log label as active
        _logLabel.getStyleClass().add("active");
    }

    public void setChangePaneHandler (EventType<MouseEvent> eventType, EventHandler<MouseEvent> handler) {
        _elasticsearchLabel.addEventFilter(eventType, handler);
        _kibanaLabel.addEventFilter(eventType, handler);
        _logLabel.addEventFilter(eventType, handler);
        _logVisualizationLabel.addEventFilter(eventType, handler);
    }

    private void addLabelHandler (Label label) {
        label.addEventFilter(MouseEvent.MOUSE_PRESSED, e -> {
            if (!label.getStyleClass().contains("active")) {
                label.getStyleClass().add("active");
            }
            for (Node labelSibling : label.getParent().getChildrenUnmodifiable()) {
                if (!labelSibling.equals(label)) {
                    labelSibling.getStyleClass().remove("active");
                }
            }
        });
    }

    @FXML
    private JFXDrawer _drawer;
    @FXML
    private Label _elasticsearchLabel;
    @FXML
    private Label _kibanaLabel;
    @FXML
    private Label _logLabel;
    @FXML
    private Label _logVisualizationLabel;
    @FXML
    private VBox _vBox;
    private static final Logger _logger = LoggerFactory.getLogger(NavigationDrawerController.class);
}
