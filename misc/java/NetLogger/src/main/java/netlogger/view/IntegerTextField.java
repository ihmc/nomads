package netlogger.view;

import com.jfoenix.controls.JFXTextField;

public class IntegerTextField extends JFXTextField
{
    public IntegerTextField () {
    }

    public void setUserChanged (boolean val) {
        _userChanged = val;
    }

    public boolean wasUserChanged () {
        return _userChanged;
    }


    private boolean _userChanged;
}
