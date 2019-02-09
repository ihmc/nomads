package us.ihmc.cm;

import java.io.Serializable;

public interface PlatformProvider
{
    public abstract void setRovingMind (RovingMind rm);

    public abstract RovingMind getRovingMind ();
    
    public abstract void addBotPart (BotPart bp);

    public abstract void removeBotPart (String uri);

    public abstract Serializable getBotPartValue (String botPartURI, String attr);
        
    public abstract void setBotPartValue (String botPartURI, String attr, Serializable value);
}
