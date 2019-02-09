package netlogger.view;

import com.jfoenix.controls.JFXButton;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Pane;
import javafx.scene.text.Font;
import javafx.scene.text.Text;

/**
 * @author Blake Ordway (bordway@ihmc.us) on 8/20/2018
 */
public class SearchFilterPane extends Pane
{
    public SearchFilterPane (String text) {
        HBox hBox = new HBox();
        Text stringText = new Text(text);
        _xButton = new JFXButton("x");

        hBox.setStyle("-fx-background-color: #eaeaea;");
        hBox.setPadding(new Insets(0, 0, 0, 5));
        hBox.setSpacing(10);

        _xButton.setFont(Font.font(12));
        stringText.setFont(Font.font(14));
        _xButton.getStyleClass().add("searchX");

        HBox.setMargin(stringText, new Insets(10, 0, 0, 0));

        hBox.getChildren().addAll(stringText, _xButton);

        this.getChildren().add(hBox);
    }

    public void setOnClearSelected (EventHandler<MouseEvent> eventHandler) {
        _xButton.setOnMouseClicked(eventHandler);
    }

    private JFXButton _xButton;

}
