package netlogger.view.tracking;

import javafx.scene.Cursor;
import javafx.scene.layout.StackPane;
import javafx.scene.text.Text;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * A modified stackpane that holds a single rectangle and text. Handles animations between itself and another
 * {@link ModuleView}
 */
public class ModuleView extends StackPane
{
    public ModuleView (double width, double height, String name) {

        setMinWidth(width);
        setMinHeight(height);
        setMaxWidth(width);
        setMaxHeight(height);

        setCursor(Cursor.HAND);

        getChildren().addAll(new Text(name));
    }

    private static final Logger _logger = LoggerFactory.getLogger(ModuleView.class);
}
