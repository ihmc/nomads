package us.ihmc.aci.util.dspro.soi;

import us.ihmc.aci.disServiceProProxy.DisServiceProProxyInterface;
import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class PeriodicTrackGeneratingTrackHandler extends TrackHandler
{
    private PeriodicTrackGenerator _updater;

    public PeriodicTrackGeneratingTrackHandler (DisServiceProProxyInterface dspro)
    {
         super (dspro);
         _updater = new PeriodicTrackGenerator (this);
    }

    @Override
    public void run()
    {
        new Thread (_updater).start();
        super.run();
    }

    @Override
    public synchronized boolean setActualPosition (String nodeId, float fLatitude, float fLongitude, float fAltitude,
                                                   String location, String note) throws CommException
    {
        boolean rc = super.setActualPosition (nodeId, fLatitude, fLongitude, fAltitude, location, note);
        _updater.calledSetActualPosition (nodeId, fLatitude, fLongitude, fAltitude, location, note);

        return rc;
    }
}
