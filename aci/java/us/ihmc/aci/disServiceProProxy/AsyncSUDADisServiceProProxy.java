package us.ihmc.aci.disServiceProProxy;

import us.ihmc.aci.util.dspro.soi.PeriodicTrackGeneratingTrackHandler;
import us.ihmc.aci.util.dspro.soi.TrackHandler;
import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class AsyncSUDADisServiceProProxy extends AsyncDisServiceProProxy
{
    private TrackHandler _trackHandler;

    public AsyncSUDADisServiceProProxy()
    {
        super (DEFAULT_POLLING_TIME);
    }

    public AsyncSUDADisServiceProProxy (long pollingTime)
    {
        super (pollingTime);
    }

    public AsyncSUDADisServiceProProxy (short applicationId)
    {
        super (applicationId);
    }

    public AsyncSUDADisServiceProProxy (short applicationId, long pollingTime)
    {
        super (applicationId, pollingTime);
    }

    public AsyncSUDADisServiceProProxy (short applicationId, long reinitializationAttemptInterval, long pollingTime)
    {
        super (applicationId, reinitializationAttemptInterval, pollingTime);
    }

    public AsyncSUDADisServiceProProxy (short applicationId, String host, int iPort)
    {
    	super (applicationId, host, iPort);
    }

    public AsyncSUDADisServiceProProxy (short applicationId, String host, int iPort, long pollingTime)
    {
    	super (applicationId, host, iPort, pollingTime);
    }

    public AsyncSUDADisServiceProProxy (short applicationId, String host, int port,
                                        long reinitializationAttemptInterval, long pollingTime)
    {
        super (applicationId, host, port, reinitializationAttemptInterval, pollingTime);
    }

    @Override
    public synchronized boolean setMetadataPossibleValues (String xMLMetadataValues) throws CommException
    {
        return super.setMetadataPossibleValues(xMLMetadataValues);
    }

    @Override
    public int init() throws Exception
    {
        _trackHandler = new PeriodicTrackGeneratingTrackHandler (this);
        int rc = super.init();
        new Thread (_trackHandler).start();
        return rc;
    }

    @Override
    public synchronized boolean setActualPosition (float fLatitude, float fLongitude, float fAltitude,
                                                   String location, String note) throws CommException
    {
        _trackHandler.setActualPosition (getNodeId(), fLatitude, fLongitude, fAltitude, location, note);
        return super.setActualPosition(fLatitude, fLongitude, fAltitude, location, note);
    }
}
