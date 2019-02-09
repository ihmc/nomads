package us.ihmc.cm.example;

import java.io.Serializable;
import java.util.Hashtable;
import java.util.Vector;

import us.ihmc.cm.*;
import us.ihmc.nomads.Agent;
import us.ihmc.nomads.StrongMobilityService;
import us.ihmc.util.URI;

public class ExamplePlatformProvider implements PlatformProvider
{
    public ExamplePlatformProvider ()
    {
        _botPartMap = new Hashtable();
    }

    
    public void setRovingMind (RovingMind rm)
    {
        _rovMind = rm;
    }
    
    public RovingMind getRovingMind ()
    {
        return _rovMind;
    }
    
    public void addBotPart (BotPart bp)
    {
        if (bp == null) throw new IllegalArgumentException ("Null botpart");
        String bpuri = bp.getURI();
        if (bpuri == null) throw new IllegalArgumentException ("Null botpart URI");
        _botPartMap.put (bpuri, bp);
    }

    public void removeBotPart (String bpuri)
    {
        if (bpuri == null) throw new IllegalArgumentException ("Null botpart URI");
        _botPartMap.remove (bpuri);
    }

    public Serializable getBotPartValue (String botPartURI, String attr)
    {
        Long l = new Long (System.currentTimeMillis ());
        return l;
    }
        
    public void setBotPartValue (String botPartURI, String attr, Serializable value)
    {
    }
    
    private Hashtable  _botPartMap;
    private RovingMind _rovMind;
}
