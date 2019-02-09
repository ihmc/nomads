package netlogger.util;

import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyCombination;

public class KeyCodeToModifier
{
    public static KeyCombination.Modifier getModifier (KeyCode e) {
        if (e == KeyCode.ALT) {
            return KeyCombination.ALT_DOWN;
        }
        else if (e == KeyCode.CONTROL) {
            return KeyCombination.CONTROL_DOWN;
        }
        else if (e == KeyCode.META) {
            return KeyCombination.META_DOWN;
        }
        else if (e == KeyCode.SHORTCUT) {
            return KeyCombination.SHORTCUT_DOWN;
        }
        else if (e == KeyCode.SHIFT) {
            return KeyCombination.SHIFT_DOWN;
        }

        return null;
    }
}
