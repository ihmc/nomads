package netlogger.view;

import com.google.common.collect.EvictingQueue;
import javafx.event.EventHandler;
import javafx.scene.control.ListView;
import javafx.scene.control.ScrollBar;
import javafx.scene.input.KeyEvent;
import javafx.scene.input.ScrollEvent;
import javafx.scene.layout.StackPane;
import javafx.scene.text.Text;
import netlogger.model.ObservableLimitedList;
import netlogger.model.StyledString;

import java.util.List;

public class MessageListView
{
    public MessageListView (ListView<StyledString> listView, Text noMessagesText, StackPane parent) {
        _messageListView = listView;
        _noMessagesText = noMessagesText;
        _listViewStackPane = parent;

        _messageListView.setCellFactory(param -> new MessageAreaCell());

        _messageListView.addEventFilter(ScrollEvent.ANY, event -> {
            if (_scrollBar != null) {
                double scrollbarValue = _scrollBar.getValue();
                double deltaY = (double) _itemsToScroll / ((double) _messageListView.getItems().size() - 1);

                // If this is an upward scroll, Y is positive, change deltaY to negative
                deltaY = event.getDeltaY() > 0 ? -deltaY : deltaY;

                if (deltaY + scrollbarValue > 1.0) {
                    deltaY = 1.0 - scrollbarValue;
                }
                else if (deltaY + scrollbarValue < 0.0) {
                    deltaY = scrollbarValue * -1;
                }

                _scrollBar.setValue(scrollbarValue + deltaY);
            }
        });

    }

    public void setScrollHandler (EventHandler<ScrollEvent> scrollHandler) {
        _messageListView.addEventFilter(ScrollEvent.ANY, scrollHandler);
    }

    public void setScrollBar (ScrollBar listViewScrollBar) {
        _scrollBar = listViewScrollBar;
    }

    public void setKeyPressHandler (EventHandler<KeyEvent> keyDownHandler) {
        _messageListView.setOnKeyPressed(keyDownHandler);
    }

    public synchronized void addMessage (StyledString message) {
        _messageListView.getItems().add(message);
    }

    public synchronized void addToStart (List<StyledString> message) {
        _messageListView.getItems().addAll(0, message);
    }

    public void scrollTo (int pos) {
        _messageListView.scrollTo(pos);
    }

    public synchronized StyledString getFirstVal () {
        if (_messageListView.getItems().size() > 0) {
            return _messageListView.getItems().get(0);
        }
        return null;
    }

    public void addAllMessages (EvictingQueue<StyledString> messageList) {
        _messageListView.getItems().addAll(messageList);
    }

    public void addAllMessages (List<StyledString> messageList) {
        _messageListView.getItems().addAll(messageList);
    }

    public void clearListView () {
        _messageListView.getItems().clear();
    }


    public void removeNoMessagesText () {
        _listViewStackPane.getChildren().remove(_noMessagesText);
    }

    public void addNoMessagesText () {
        if (!_listViewStackPane.getChildren().contains(_noMessagesText)) {
            _listViewStackPane.getChildren().add(_noMessagesText);
        }
    }

    public int getItemCount () {
        return _messageListView.getItems().size();
    }

    public synchronized void setMaximumSize (int maxSize) {
        if (_messageListView.getItems() instanceof ObservableLimitedList) {
            ObservableLimitedList<StyledString> newList = new ObservableLimitedList<>(maxSize);
            if (maxSize > _messageListView.getItems().size()) {
                newList.addAll(_messageListView.getItems());
            }
            else {
                List<StyledString> items = _messageListView.getItems().subList(_messageListView.getItems().size() - maxSize, _messageListView.getItems().size());
                newList.addAll(items);
            }
            _messageListView.setItems(newList);
        }
    }

    public void removeRange (int from, int to) {
        _messageListView.getItems().remove(from, to);
    }

    private ListView<StyledString> _messageListView;
    private Text _noMessagesText;
    private StackPane _listViewStackPane;
    private ScrollBar _scrollBar;
    private final int _itemsToScroll = 3;

}
