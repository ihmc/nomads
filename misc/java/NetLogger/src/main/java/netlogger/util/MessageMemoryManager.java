package netlogger.util;

import javafx.event.EventHandler;
import javafx.scene.control.ScrollBar;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.input.MouseEvent;
import javafx.scene.input.ScrollEvent;
import netlogger.model.settings.SettingField;
import netlogger.model.settings.Updatable;
import netlogger.util.settings.SettingsManager;
import netlogger.view.MessageListView;

import java.util.HashMap;

public class MessageMemoryManager implements Updatable
{

    public MessageMemoryManager () {
        SettingsManager.getInstance().getSettingsBridge().addUpdatableClass(this);
    }

    public void setListView (MessageListView listView) {
        _messageListView = listView;
    }

    public void setScrollBar (ScrollBar bar) {
        _scrollBar = bar;
    }

    public void initialize () {
        _reloadMessages = true;

        EventHandler<ScrollEvent> scrollHandler = event -> {
            _userScrolled = true;
        };

        EventHandler<KeyEvent> keyEventHandler = event -> {
            if (event.getCode().equals(KeyCode.UP)) {
                _userScrolled = true;
            }
        };
        _messageListView.setKeyPressHandler(keyEventHandler);

        _messageListView.setScrollHandler(scrollHandler);

        _scrollBar.addEventFilter(MouseEvent.MOUSE_PRESSED, event -> {
            _userScrolled = true;
        });

        _scrollBar.addEventFilter(MouseEvent.MOUSE_DRAGGED, event -> {
            _userScrolled = true;
        });

        _scrollBar.visibleProperty().addListener((observable, oldValue, newValue) -> {
            if (newValue) {
                _scrollBar.setValue(1.0);
            }
        });

        _scrollBar.valueProperty().addListener((observable, oldValue, newValue) -> {
            if (newValue.doubleValue() != 1.0) {
                _messageListView.setMaximumSize(Integer.MAX_VALUE);
            }

            if (_userScrolled) {
                // Check that the scroll bar is at a certain spot, and that the user scrolled up
                if (newValue.doubleValue() <= .15 && oldValue.doubleValue() > newValue.doubleValue()) {
                    if (_reloadMessages) {
                        new Thread(this::loadMoreMessages, "Message loading thread").start();
                    }
                }

                if (newValue.doubleValue() == 1.0) {
                    constrainSize();
                }
                _userScrolled = false;
            }
        });
    }

    public void shutdownCalled () {
        SettingsManager.getInstance().getSettingsBridge().removeUpdatableClass(this);
    }

    @Override
    public void update (HashMap<String, Object> settings) {
        _reloadMessages = (boolean) settings.get(RELOAD_ON_SCROLL);
    }

    private void loadMoreMessages () {
//        String firstVal = _messageListView.getFirstVal();
//        if (firstVal == null){
//            return;
//        }
//
//        String oldestUUID = firstVal.split("UUID: ")[1];
//
//        if (_messageListView.getFirstVal() == null){
//            return;
//        }
//        String oldestUUID = _messageListView.getFirstVal().getUUID().getText();
//
//        int prevIndex = (int) (_scrollBar.getValue() * _messageListView.getItemCount());
//
//        List<StringMeasure> stringMeasureList = DataFileManager.getInstance().getOlderStringMeasures(oldestUUID, MessageConfig.DEFAULT_NUMBER_OF_MESSAGES);
//        if (stringMeasureList == null) {
//            return;
//        }
//        List<StyledString> styledMeasures = new ArrayList<>();
//        for (StringMeasure measure : stringMeasureList) {
//            if (MeasureFilter.getInstance().search(measure) && MeasureFilter.getInstance().checkTopic(measure)) {
//                StyledString wrapper = new StyledString(measure);
//                StylingManager.getInstance().setValues(wrapper);
//                styledMeasures.add(wrapper);
//            }
//        }
//
//        List<String> messages = new ArrayList<>();
//        for (StyledString measure : styledMeasures){
//            messages.add(CustomMeasureToString.getStringFromMeasure(measure));
//        }
//
//        Platform.runLater(() -> {
//            _messageListView.addToStart(messages);
//            _messageListView.scrollTo(prevIndex + styledMeasures.size());
//            _messageListView.removeNoMessagesText();
//        });
    }

    private void constrainSize () {
        _messageListView.setMaximumSize(150);
        _messageListView.removeRange(0, _messageListView.getItemCount() - 150);
    }


    @SettingField(settingText = RELOAD_ON_SCROLL, configText = ConfigConstants.RELOAD_ON_SCROLL_BOOL, defaultValue = "true")
    private boolean _reloadMessages;
    private boolean _userScrolled;
    private ScrollBar _scrollBar;
    private MessageListView _messageListView;

    private static final String RELOAD_ON_SCROLL = "Reload records on scroll";
}
