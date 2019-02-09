package netlogger.controller;

import com.jfoenix.controls.JFXButton;
import com.jfoenix.controls.JFXTextField;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.image.Image;
import javafx.stage.Screen;
import javafx.stage.Stage;
import netlogger.Launcher;
import netlogger.model.KibanaLauncher;

import java.io.IOException;

/**
 * Controls the webview of kibana being seen
 */
public class KibanaViewController
{
    @FXML
    public void initialize () {
        _launchBtn.setOnMouseClicked(click -> {
            if (_isOpen) {
                return;
            }

            openLauncher();
        });
    }

    public void requestTermination () {
        try {
            _launcher.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Opens the launcher for kibana
     */
    private void openLauncher () {
        double stageX;
        double stageY;
        ObservableList<Screen> screens = Screen.getScreens();
        if (screens.size() > 1) {
            stageX = screens.get(1).getVisualBounds().getMinX();
            stageY = screens.get(1).getVisualBounds().getMinY();
        }
        else {
            stageX = screens.get(0).getVisualBounds().getMinX();
            stageY = screens.get(0).getVisualBounds().getMinY();
        }

        _launcher = new KibanaLauncher(_urlTextField.getText(), stageX, stageY);
        Stage stage = new Stage();

        stage.getIcons().add(new Image(Launcher.class.getResourceAsStream("kibana-32x32.png")));
        stage.setOnHiding(event -> {
            _isOpen = false;
            try {
                _launcher.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        });
        try {
            _launcher.start(stage);
            _isOpen = true;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private KibanaLauncher _launcher;
    private boolean _isOpen;
    @FXML
    private JFXTextField _urlTextField;
    @FXML
    private JFXButton _launchBtn;
}

