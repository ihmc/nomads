package netlogger.view;

import javafx.scene.Node;
import javafx.scene.control.ContextMenu;
import javafx.scene.control.ListCell;
import javafx.scene.control.MenuItem;
import netlogger.model.StyledString;
import netlogger.view.MessageArea;

/**
 * List cell that gets created/recycled whenever a ListView needs it
 */
public class MessageAreaCell extends ListCell<StyledString>
{

    public MessageAreaCell () {
        _area = new MessageArea();
        _view = _area.getAnchorPane();

        _contextMenu = new ContextMenu();

        MenuItem editItem = new MenuItem();
        editItem.setText("View movement!");
        editItem.setOnAction(event -> {
            StyledString item = this.getItem();
            // code to edit item...
        });
        _contextMenu.getItems().add(editItem);
    }

    @Override
    public void updateItem (StyledString obj, boolean empty) {
        super.updateItem(obj, empty);

        // If item != tracking type, don't have context menu?
        if (empty) {
            setText(null);
            setGraphic(null);
            setContextMenu(null);
        }
        else {
            _area.setInfo(obj);
            setContextMenu(_contextMenu);
            setGraphic(_view);
        }
    }

    private final MessageArea _area;
    private final Node _view;
    private final ContextMenu _contextMenu;
}
