/*    
       This software may be used and distributed according to the terms of
       the GNU General Public License (GPL), incorporated herein by
       reference.  Drivers based on this skeleton fall under the GPL and
       must retain the authorship (implicit copyright) notice.
 
       This program is distributed in the hope that it will be useful, but
       WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
       General Public License for more details.
 */
package cricketdaemon.position;

import cricketdaemon.Util;
import cricketdaemon.module.*;
import cricketdaemon.reader.*;
import cricketdaemon.space.DistanceEstimate;
import java.util.*;

/**
 * A Data object that holds the coordinate and the derived distances
 * for each of the beacons used in the Matrix solver.
 *
 * @author Allen Miu
 */
public class PositionData extends Data
{
    private CartesianCoord coord;
    private DistanceEstimate[] distances;
    private double speedOfSoundFactor;
    private String algor = "";

    public final static PositionData invalidInstance;

    static {
	double[] di = new double[1];
	int[]    ii = new int[1];
	String[] si = new String[1];

	si[0] = "Invalid";
	di[0] = Double.NaN;
	ii[0] = Integer.MIN_VALUE;

	invalidInstance = 
	    new PositionData(Beacon.invalidInstance, Double.NaN, Double.NaN, Double.NaN,
			     di, di, di, si, ii, di, Double.NaN);
    }

    // FIXME: add sequence number and timeStamp
    // FIXME: add beacon coordinates for the respective d_bar values
    public PositionData(Beacon b,
			double x, double y, double z,
			double[] bx, double[] by, double[] bz,
			String[] space, int[] id, 
			double[] d_bar, 
			double k)
    {
	this(b, x, y, z, bx, by, bz, space, id, d_bar, k, space.length);
    }

    public PositionData(Beacon b,
			double x, double y, double z,
			double[] bx, double[] by, double[] bz,
			String[] space, int[] id, double[] d_bar, 
			double k, int length)
    {
	super(b);
	speedOfSoundFactor = k;
	coord = new CartesianCoord(x, y, z);
	distances = new DistanceEstimate[length];
	for(int i = 0; i < length; i++)
	    distances[i] = new DistanceEstimate(space[i], id[i], 
						new CartesianCoord(bx[i], by[i], bz[i]), 
						d_bar[i]);
    }

    public void setAlgor(String a) { algor = a; }
    public String getAlgor() { return algor; }

    public interface PositionDataHandler {
	public void processPositionData(PositionData newData, 
					BitSet fieldMask);
    }

    public void process(Object handler, BitSet fieldMask) 
    {
	Util.assert_nonjava(handler instanceof PositionDataHandler,
		    "PositionData::process() incorrect handler type\nMake sure the offending module 'implements' PositionData.");
	PositionDataHandler h = (PositionDataHandler) handler;
	h.processPositionData(this, fieldMask);
    }

    public CartesianCoord getCoord()
    {
	return coord;
    }

    public DistanceEstimate[] getDistances()
    {
	return distances;
    }

    public double getSpeedOfSoundFactor()
    {
	return speedOfSoundFactor;
    }

    public String toString()
    {
	String result = "";
	/*result += timeStamp;
	result += " " + coord.toString();
	if(distances != null) {
	    for(int i = 0; i < distances.length; i++)
		result += " "+ distances[i];
	}
	result += " "+speedOfSoundFactor;*/

        result += " " + coord.toString();
	return result;
    }
}

