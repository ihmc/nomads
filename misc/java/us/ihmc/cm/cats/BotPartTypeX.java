package us.ihmc.cm.cats;

import java.io.Serializable;
import us.ihmc.cm.*;

public class BotPartTypeX extends BotPart
{
    public BotPartTypeX (String URI)
    {
        _uri = URI;
    }
    
    public String getURI()
    {
        return _uri;
    }
    
    String _uri;

}
