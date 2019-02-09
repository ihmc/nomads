package netlogger.controller;

import com.jfoenix.controls.JFXAlert;
import com.jfoenix.controls.JFXButton;
import com.jfoenix.controls.JFXDialogLayout;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.Tooltip;
import javafx.scene.layout.HBox;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.*;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;

import static javafx.scene.input.MouseEvent.MOUSE_CLICKED;

/**
 * This class is used to control the contact alert that shows up when the "Contact" label is clicked
 * @author Blake Ordway (bordway@ihmc.us) on 8/28/2018
 */
public class ContactAlertController
{
    @FXML
    public void initialize(){
        _dialogLayout.setBody(_bodyHBox);
        _dialogLayout.setHeading(_headingLabel);

        _closeButton = new JFXButton("Close");
        _dialogLayout.setActions(_closeButton);

        // Set up the email address hyperlink
        Label emailAddressLabel = new Label("bordway@ihmc.us");
        emailAddressLabel.addEventFilter(MOUSE_CLICKED, event1 -> {
            if (Desktop.isDesktopSupported()){
                Desktop desktop = Desktop.getDesktop();
                if (desktop.isSupported(Desktop.Action.MAIL)){
                    try {
                        URI mailTo = new URI("mailto:" +  emailAddressLabel.getText());
                        desktop.mail(mailTo);
                    } catch (URISyntaxException | IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        });
        Label authorLabel = new Label("Author: ");
        authorLabel.setId("author-label");
        emailAddressLabel.getStyleClass().add("email-label");
        emailAddressLabel.setTooltip(new Tooltip("Email Blake Ordway (this will open your default mail app"));

        _bodyHBox.getChildren().addAll(authorLabel, emailAddressLabel);

    }

    /**
     * Sets the eventhandler for when the close button is pressed
     * @param eventHandler Event handler
     */
    public void setCallback(EventHandler<ActionEvent> eventHandler){
        _closeButton.setOnAction(eventHandler);
    }

    @FXML private JFXDialogLayout _dialogLayout;

    @FXML private HBox _bodyHBox;
    @FXML private Label _headingLabel;
    private Button _closeButton;

    private static final Logger _logger = LoggerFactory.getLogger(ContactAlertController.class);
}
