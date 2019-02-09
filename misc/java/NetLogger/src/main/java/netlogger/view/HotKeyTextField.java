package netlogger.view;

import com.jfoenix.controls.JFXTextField;
import javafx.scene.control.Tooltip;
import javafx.scene.input.KeyCode;

import java.util.ArrayList;
import java.util.List;

public class HotKeyTextField extends JFXTextField
{
    public HotKeyTextField () {
        _keyCodes = new ArrayList<>();
        _modifierKeys = new ArrayList<>();

        getStyleClass().add("hotkey-text-field");

        focusedProperty().addListener((observable, oldValue, newValue) -> {
            if (newValue) {
                setPromptText("Listening...");
            }
            else {
                setPromptText("None!");
            }
        });

        setOnKeyPressed(event -> {
            if (event.getCode() == KeyCode.ESCAPE) {
                _keyCodes.clear();
                _modifierKeys.clear();
                _hotKeyString = "";
                setText("");
                return;
            }

            if (event.getCode().isModifierKey()) {
                handleModifierKey(event.getCode());
            }
            else {
                handleRegularKey(event.getCode());
            }

            reorderKeyText();
            setText(_hotKeyString);
        });

        selectionProperty().addListener((observable, oldValue, newValue) -> {
            selectRange(0, 0);
        });


        textProperty().addListener((observable, oldValue, newValue) -> {
            if (oldValue.equals(newValue)) {
                return;
            }
            if (!newValue.equals(_hotKeyString)) {
                setText(_hotKeyString);
            }
        });

        setTooltip(new Tooltip("Press Escape to clear hotkey and rebind!"));
    }

    public void setDefault (KeyCode... keyCodes) {
        for (KeyCode code : keyCodes) {
            if (code.isModifierKey()) {
                handleModifierKey(code);
            }
            else {
                handleRegularKey(code);
            }
        }

        reorderKeyText();
        setText(_hotKeyString);
    }

    public void setModifierKeySize (int val) {
        _maxModifierKeys = val;
    }

    public void setRegularKeySize (int val) {
        _maxRegularKeys = val;
    }

    private void handleModifierKey (KeyCode code) {
        if (_modifierKeys.contains(code) || _modifierKeys.size() == _maxModifierKeys) {
            return;
        }

        _modifierKeys.add(code);
    }

    private void handleRegularKey (KeyCode code) {
        if (_keyCodes.contains(code) || _keyCodes.size() == _maxRegularKeys) {
            return;
        }

        _keyCodes.add(code);
    }

    private void reorderKeyText () {
        int keyCounter = 0;
        StringBuilder keyStringBuilder = new StringBuilder();
        for (KeyCode modifierCode : _modifierKeys) {
            if (keyCounter != 0) {
                keyStringBuilder.append(" + ");
            }

            keyStringBuilder.append(modifierCode.getName());
            keyCounter++;
        }

        for (KeyCode code : _keyCodes) {
            if (keyCounter != 0) {
                keyStringBuilder.append(" + ");
            }

            keyStringBuilder.append(code.getName());
            keyCounter++;
        }

        _hotKeyString = keyStringBuilder.toString();
    }

    private List<KeyCode> getHotkeyCodes () {
        List<KeyCode> allKeys = _modifierKeys;
        allKeys.addAll(_keyCodes);

        return allKeys;
    }


    private String _hotKeyString = "";
    private int _maxModifierKeys;
    private int _maxRegularKeys;
    private List<KeyCode> _keyCodes;
    private List<KeyCode> _modifierKeys;
}
