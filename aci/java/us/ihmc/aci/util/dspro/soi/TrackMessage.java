package us.ihmc.aci.util.dspro.soi;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public final class TrackMessage
{
    private String _messageId;
    private String _trackId;
    private String _trackAction;
    private long _sequenceId;

    public TrackMessage (String messageId, String trackId, String trackAction, long sequenceId)
    {
        this._messageId = messageId;
        this._trackId = trackId;
        this._trackAction = trackAction;
        this._sequenceId = sequenceId;
    }

    public String getMessageId()
    {
        return this._messageId;
    }

    public String getTrackAction()
    {
        return this._trackAction;
    }

    public long getSequenceId()
    {
        return this._sequenceId;
    }

    public String getTrackId ()
    {
        return _trackId;
    }
}

