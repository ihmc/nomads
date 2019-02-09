package netlogger.controller;

import javafx.event.EventHandler;
import javafx.event.EventType;
import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.input.MouseEvent;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * This class controls the popup information
 * @author Blake Ordway (bordway@ihmc.us) on 8/27/2018
 */
public class InfoPopupController
{
    @FXML
    public void initialize() {

    }


    public Label getContactInfoLabel () {
        return _contactInfoLabel;
    }

    public Label getSettingsLabel () {
        return _settingsLabel;
    }

    @FXML private Label _contactInfoLabel;
    @FXML private Label _settingsLabel;


    private static final Logger _logger = LoggerFactory.getLogger(InfoPopupController.class);
}
