package us.ihmc.aci.disServiceProProxy.util;

import java.util.Calendar;
import java.util.GregorianCalendar;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxy;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class PositionUpdater implements Runnable
{
    /**
     * The path is retrieved from dspro, so this constructor can be used only
     * after that the path has been registered with dspro.
     */
    PositionUpdater(DisServiceProProxy dspro) throws CommException
    {
        _dspro = dspro;
        _path = dspro.getActualPath();
        _wayPointIndex = 0;
    }

    PositionUpdater(DisServiceProProxy dspro, NodePath path)
    {
        _dspro = dspro;
        _path = path;
        _wayPointIndex = 0;
    }

    public void run()
    {
        long defaultSleep = 3000;
        long sleepTime = 0;

        do {
            sleepTime = 0;

            // Update Way Point
            if ((_path != null) && (_path.getType() != NodePath.FIXED_LOCATION)) {
                if (_wayPointIndex < _path.getLength()) {
                    try {
                        _dspro.setActualPosition (_path.getLatitude(_wayPointIndex),
                                                  _path.getLongitude(_wayPointIndex),
                                                  _path.getAltitude(_wayPointIndex),
                                                  _path.getLocation(_wayPointIndex),
                                                  _path.getNote(_wayPointIndex));
                        System.out.println("The current way point is: <" + _wayPointIndex + ">");
                        if (_wayPointIndex < (_path.getLength()-1)) {
                            sleepTime = getTimeAtWayPoint(_path, _wayPointIndex+1) - getTimeAtWayPoint(_path, _wayPointIndex);
                            if (sleepTime < 0) {
                                System.out.println("***\n*** Sleep time = " + sleepTime + " and the index is " + _wayPointIndex + "\n***");
                                sleepTime = 30000;
                            }
                        }
                    }
                    catch (Exception e) {
                        System.err.println(e.getMessage());
                    }
                    _wayPointIndex++;
                }
            }

            // Sleep
            System.out.println("***\n***Sleep time is <" + sleepTime + ">\n***");
            try { Thread.sleep ( (sleepTime <= 0 ? defaultSleep : sleepTime) ); }
            catch (Exception e) { }
        } while (true);
    }

    private long getTimeAtWayPoint (NodePath path, int index) throws Exception
    {
        return (new GregorianCalendar (path.getTime(index, Calendar.YEAR),
                                       path.getTime(index, Calendar.MONTH),
                                       path.getTime(index, Calendar.DAY_OF_MONTH),
                                       path.getTime(index, Calendar.HOUR_OF_DAY),
                                       path.getTime(index, Calendar.MINUTE),
                                       path.getTime(index, Calendar.SECOND))
               ).getTimeInMillis();
    }

    private DisServiceProProxy _dspro;
    private NodePath _path;
    private int _wayPointIndex;

}
