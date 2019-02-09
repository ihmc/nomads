package netlogger.view;

import javafx.scene.Node;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.RowConstraints;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

public class SelfBalancingGridPane extends GridPane
{
    public SelfBalancingGridPane () {
        super();
    }

    public void setMaxColumns (int maxColumns) {
        _maxColumnsPerRow = maxColumns;
    }

    public void add (Node node) {
        int row = getChildren().size() / _maxColumnsPerRow;
        int column = getChildren().size() % _maxColumnsPerRow;

        add(node, column, row);

        checkRowConstraints();
    }

    public void remove (Node... nodes) {
        getChildren().removeAll(nodes);
        clearAndResetGrid();
        checkRowConstraints();
    }

    public void clearAndResetGrid () {
        List<Node> gridPaneChildren = new ArrayList<>(getChildren());
        getChildren().clear();
        for (int i = 0; i < gridPaneChildren.size(); i++) {
            add(gridPaneChildren.get(i), i % _maxColumnsPerRow, i / _maxColumnsPerRow);
        }
    }

    private void checkRowConstraints () {
        double totalRows = (double) getChildren().size() / (double) _maxColumnsPerRow;

        int numberOfRows = (int) Math.ceil(totalRows);
        int numberOfRowConstraints = getRowConstraints().size();
        if (numberOfRows > numberOfRowConstraints) {
            for (int i = 0; i < numberOfRows - numberOfRowConstraints; i++) {
                getRowConstraints().add(new RowConstraints(40));
            }
        }
        else if (numberOfRows < numberOfRowConstraints) {
            int removeStart = numberOfRowConstraints - (numberOfRowConstraints - numberOfRows) - 1;
            int removeEnd = numberOfRowConstraints - 1;
            if (!(removeStart < 0) || removeEnd < 0) {
                getRowConstraints().remove(removeStart, removeEnd);
            }
        }
    }


    private int _maxColumnsPerRow;
    private static final Logger _logger = LoggerFactory.getLogger(SelfBalancingGridPane.class);
}
