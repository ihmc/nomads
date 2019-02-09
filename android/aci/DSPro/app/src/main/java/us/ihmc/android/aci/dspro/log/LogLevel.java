package us.ihmc.android.aci.dspro.log;

/**
 * LogLevel.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public enum LogLevel
{
    SevereError(1),
    MildError(2),
    Warning(3),
    Info(4),
    NetDetailDebug(5),
    LowDetailDebug(6),
    MediumDetailDebug(7),
    HighDetailDebug(8);

    private LogLevel(int level)
    {
        _level = level;
    }

    public int getLevel()
    {
        return _level;
    }

    private int _level;
}
