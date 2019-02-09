package netlogger.view;

import javafx.event.Event;
import javafx.scene.control.ContextMenu;
import javafx.scene.control.MenuItem;
import javafx.scene.control.TextArea;
import javafx.scene.input.Clipboard;
import javafx.scene.input.ClipboardContent;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.StackPane;
import javafx.scene.text.Text;
import netlogger.model.StyledString;
import netlogger.util.CustomMeasureToString;
import org.fxmisc.richtext.InlineCssTextArea;

public class MessageArea
{
    public MessageArea () {
        _anchorPane = new AnchorPane();
        initializeTextAreas();

        _anchorPane.setStyle("-fx-background-color: #bdbdbd7F;");
    }

    private void initializeTextAreas () {
        _textArea = new TextArea();
        setValuesForTextArea(_textArea);
        _textArea.textProperty().addListener((observable, oldValue, newValue) -> {
            Text t = new Text(newValue);
            t.setFont(_textArea.getFont());
            StackPane pane = new StackPane(t);
            pane.layout();
            double width = t.getLayoutBounds().getWidth();
            double padding = 20;
            _textArea.setPrefWidth(width + padding);
        });
        _textArea.setWrapText(false);
        _anchorPane.getChildren().add(_textArea);
    }

    public void setInfo (StyledString measure) {
        _textArea.setText(CustomMeasureToString.getStringFromMeasure(measure));
    }

    private void setValuesForTextArea (TextArea textArea) {
        textArea.setEditable(false);
        textArea.setOnScroll(Event::consume);
        textArea.setStyle("-fx-background-color: transparent;");

        textArea.setMaxHeight(50);
    }

    private void createRightClickMenu (InlineCssTextArea textArea) {
        ContextMenu menu = new ContextMenu();
        MenuItem copyItem = new MenuItem("Copy Text");

        copyItem.setOnAction(event -> {
            copyTextAreaText(textArea.getSelectedText());
        });
        menu.getItems().add(copyItem);
        textArea.setContextMenu(menu);
    }

    private void copyTextAreaText (String selectedText) {
        if (selectedText == null) {
            return;
        }
        ClipboardContent content = new ClipboardContent();
        content.putString(selectedText);
        Clipboard systemClipboard = Clipboard.getSystemClipboard();
        systemClipboard.setContent(content);
    }

    public AnchorPane getAnchorPane () {
        return _anchorPane;
    }

    // <------------------------------------------------------------------------------------------>
    private AnchorPane _anchorPane;
    private TextArea _textArea;

}