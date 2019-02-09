package us.ihmc.android.aci.dspro;

import android.content.Context;
import android.view.KeyEvent;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;
import us.ihmc.android.aci.dspro.util.AndroidUtil;

/**
 * PortOnKeyListener.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class PortOnKeyListener implements View.OnKeyListener
{
    private int _port;
    private Context _context;

    public PortOnKeyListener (Context context)
    {
        _context = context;
    }

    @Override
    public boolean onKey (View view, int keyCode, KeyEvent keyEvent)
    {
        final EditText etPortValue = (EditText) view;

        // If the event is a key-down event on the "enter" button
        if ((keyEvent.getAction() == KeyEvent.ACTION_DOWN) &&
                (keyCode == KeyEvent.KEYCODE_ENTER)) {

            String port = etPortValue.getText().toString();

            try {
                if (!AndroidUtil.isValidPort(port)) {
                    Toast.makeText(_context, "Port should be between 0 and 65535", Toast.LENGTH_SHORT).show();
                    etPortValue.setText("");
                    return false;
                }

                // Perform action on key press
                _port = Integer.valueOf(port);
                Toast.makeText(_context, "Port is set to: " + _port, Toast.LENGTH_SHORT).show();
                return true;

            }
            catch (NumberFormatException e) {
                Toast.makeText(_context, "Not a valid port", Toast.LENGTH_SHORT).show();
                etPortValue.setText("");
                return false;
            }
        }
        return false;
    }

    public int getPort ()
    {
        return _port;
    }
}
