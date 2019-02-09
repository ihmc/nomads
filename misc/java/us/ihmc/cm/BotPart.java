package us.ihmc.cm;

import java.io.Serializable;

public abstract class BotPart
{
    public abstract String getURI();

    public Serializable getValue (String attr)
    {
        if (_rovingMind != null) {
            return _rovingMind.getBotPartValue (getURI(), attr);
        }
        throw new IllegalStateException ("Calling BotPart.getValue() on a botpart with null RovingMind");
    }

    public void setValue (String attr, Serializable value)
    {
        if (_rovingMind != null) {
            _rovingMind.setBotPartValue (getURI(), attr, value);
        }
        throw new IllegalStateException ("Calling BotPart.setValue() on a botpart with null RovingMind");
    }
    
    public void run (Runnable runnable)
    {
        if (_rovingMind != null) {
            _rovingMind.executeOnBotPartSync (getURI(), runnable);
        }
        throw new IllegalStateException ("Calling BotPart.run() on a botpart with null RovingMind");
    }

    public void setRovingMindRef (RovingMind rm)
    {
        _rovingMind = rm;
    }

    RovingMind _rovingMind = null;
}
