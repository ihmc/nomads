package us.ihmc.aci.util.dspro.soi;

import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.comm.CommException;

/**
 *
 * @author gbenincasa
 */
public class PeriodicTrackGenerator implements Runnable
{
    class Position
    {
        final String _node;
        final float _fLatitude;
        final float _fLongitude;
        final float _fAltitude;
        final String _location;
        final String _note;
        final long _timestamp;

        Position (String node, float fLatitude, float fLongitude, float fAltitude,
                  String location, String note)
        {
            _node = node;
            _fLatitude = fLatitude;
            _fLongitude = fLongitude;
            _fAltitude = fAltitude;
            _location = location;
            _note = note;
            _timestamp = System.currentTimeMillis();
        }
    }

    private final TrackHandler _handler;
    private final long _positionRefreshTimeout;
    private Position _latestPosition;

    PeriodicTrackGenerator (TrackHandler handler)
    {
        this (handler, 60000);
    }

    PeriodicTrackGenerator (TrackHandler handler, long positionRefreshTimeout)
    {
        _handler = handler;
        _latestPosition = null;
        _positionRefreshTimeout = positionRefreshTimeout;
    }

    public void run()
    {
        while (true) {
            long sleepTime = _positionRefreshTimeout;
            synchronized (this) {
                if (_latestPosition != null) {
                    sleepTime = System.currentTimeMillis() - _latestPosition._timestamp;
                    if (sleepTime > _positionRefreshTimeout) {
                        try {
                            _handler.setActualPosition (_latestPosition._node, _latestPosition._fLatitude, _latestPosition._fLongitude,
                                                        _latestPosition._fAltitude, _latestPosition._location, _latestPosition._note);
                            _latestPosition = new Position (_latestPosition._node, _latestPosition._fLatitude, _latestPosition._fLongitude,
                                                            _latestPosition._fAltitude, _latestPosition._location, _latestPosition._note);
                            
                        }
                        catch (CommException ex) {
                            Logger.getLogger(PeriodicTrackGenerator.class.getName()).log(Level.SEVERE, null, ex);
                        }
                    }
                }
            }

            try {
                Thread.sleep(sleepTime);
            }
            catch (InterruptedException ex) {
                Logger.getLogger(PeriodicTrackGenerator.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }

    public synchronized boolean calledSetActualPosition (String node, float fLatitude, float fLongitude, float fAltitude,
                                                         String location, String note)
    {
        _latestPosition = new Position (node, fLatitude, fLongitude, fAltitude, location, note);
        return true;
    }
}
