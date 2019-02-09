package us.ihmc.cm.example;

import java.io.Serializable;
import us.ihmc.cm.*;

public class ExampleBotPart extends BotPart
{
    public ExampleBotPart (String uri)
    {
        if (uri == null) throw new IllegalArgumentException ("Null URI");
        _uri = uri;
    }
    
    public String getURI()
    {
        return _uri;
    }

    String _uri = null;
}
