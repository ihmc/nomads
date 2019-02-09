package netlogger.model.settings;

public abstract class Setting<T>
{
    public abstract void saveCurrentState (T state);

    public abstract void saveOriginalState (T state);

    public abstract T getCurrentState ();

    public abstract T getOriginalState ();
}
