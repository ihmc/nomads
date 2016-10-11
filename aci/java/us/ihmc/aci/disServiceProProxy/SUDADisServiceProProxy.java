package us.ihmc.aci.disServiceProProxy;

import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.aci.dspro2.legacy.LegacyDSProProxy;
import us.ihmc.aci.util.dspro.soi.PeriodicTrackGeneratingTrackHandler;
import us.ihmc.aci.util.dspro.soi.TrackHandler;

import us.ihmc.comm.CommException;

/**
 * DisServiceProProxy version that cancel the received Soi Native Tracks from DSPro
 * and DisService data caches and pushes generated Tracks at every setActualPosition().
 *
 * @author Giacomo Benincasa (gbenincasa@ihmc.us), Enrico Casini (ecasini@ihmc.us)
 */
public class SUDADisServiceProProxy extends DisServiceProProxy
{
    private TrackHandler _trackHandler;

    public SUDADisServiceProProxy ()
    {
        super();
        _trackHandler = null;
    }

    public SUDADisServiceProProxy (short applicationId)
    {
        super(applicationId);
        _trackHandler = null;
    }

    public SUDADisServiceProProxy (short applicationId, String host, int iPort)
    {
        super (applicationId, host, iPort);
        _trackHandler = null;
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

