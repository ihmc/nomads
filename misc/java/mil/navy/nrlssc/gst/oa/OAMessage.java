package mil.navy.nrlssc.gst.oa;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.gst.util.GHubDataInputStream;

public abstract class OAMessage
{
    public OAMessage()
    {
        super();
        _content = null;
    }

    public byte[] getContentAsByteArray()
    {
        return _content;
    }

    public static byte[] leftNullPad (String str, int size) throws UnsupportedEncodingException
    {
        if (str.length() < size) {
            while (str.length() < size) {
                str += "\0";
            }
        }
        else {
            str = str.substring(0, size);
        }
        return str.getBytes ("US-ASCII");
    }

    public boolean deserialize (GHubDataInputStream dataInput, int len)
    {
        try {
            dataInput.mark (len);
            _content = dataInput.readByteArray (len);
            dataInput.reset();
            return deserialize (dataInput.readByteArray (len));
        }
        catch (IOException ex) {
            Logger.getLogger (OAMessage.class.getName()).log (Level.SEVERE, null, ex);
            return false;
        }
    }

    public abstract boolean deserialize (byte messageBuffer[]);

    public abstract byte[] serialize() throws IOException;

    public abstract void printMessage();

    private byte[] _content;
}

