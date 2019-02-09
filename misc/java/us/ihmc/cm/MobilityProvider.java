package us.ihmc.cm;

public interface MobilityProvider
{
    public abstract void setRovingMind (RovingMind rm);
    
    public abstract RovingMind getRovingMind ();

    public abstract void addBotPart (BotPart bp);

    public abstract void removeBotPart (String uri);

    public abstract String getCurrentBotPart ();

    public abstract void setTimeSlice (long t);
    
    public abstract long getTimeSlice ();
    
    public abstract Object getSynchronizationToken ();
    
    public abstract void start ();
    
    public abstract void moveRovMindAsap ();
}


