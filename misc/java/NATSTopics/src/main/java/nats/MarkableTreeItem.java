package nats;

import javafx.scene.control.TreeItem;
import javafx.scene.paint.Color;
import javafx.scene.shape.Circle;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MarkableTreeItem<T> extends TreeItem<T>
{
    public MarkableTreeItem(){
        super();

        _marked = false;
    }

    public MarkableTreeItem(T item){
        super(item);
    }

    public void mark(){
        _marked = true;
        Circle graphic = new Circle();
        graphic.setFill(Color.web("#0f0"));
        setGraphic(graphic);
    }

    public void unmark(){
        _marked = false;
    }

    public boolean isMarked(){
        return _marked;
    }

    public boolean isVisited(){
        return _visited;
    }

    public void setVisited (boolean visited) {
        _visited = visited;
    }

    public MarkableTreeItem<T> getNextRight(){
        return _nextRight;
    }

    public void setNextRight(MarkableTreeItem<T> nextRight){
        _nextRight = nextRight;
    }

    private boolean _marked;

    private boolean _visited;
    private MarkableTreeItem<T> _nextRight;

    private static final Logger _logger = LoggerFactory.getLogger(MarkableTreeItem.class);
}
