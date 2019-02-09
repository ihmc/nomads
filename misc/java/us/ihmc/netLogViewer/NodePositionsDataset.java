package us.ihmc.netLogViewer;

import org.jfree.data.xy.AbstractXYDataset;
import java.util.Vector;

/**
 * @author: Maggie Breedy
 * Date: Feb 7, 2008
 * Time: 11:35:33 AM
 * @version $Revision$
 */

public class NodePositionsDataset extends AbstractXYDataset
{
    public void addPoint (String nodeID, double x, double y)
    {
        for (int i = 0; i < _dataVector.size(); i++) {
            NodePositionSeries nps = (NodePositionSeries) _dataVector.elementAt (i);
            //System.out.println("NodeID: " + nodeID);
            //System.out.println("x/y " + x + " / " + y);
            if (nps.getNodeID().equals (nodeID)) {
                nps.addDatapoint (x, y);
                fireDatasetChanged();
                return;
            }
        }
        NodePositionSeries newNPS = new NodePositionSeries (nodeID);
        newNPS.addDatapoint (x, y);
        _dataVector.addElement (newNPS);
        fireDatasetChanged();
        return;
    }

    public int getSeriesCount()
    {
        return _dataVector.size ();
    }

    public Comparable getSeriesKey (int series)
    {
        NodePositionSeries nps = (NodePositionSeries) _dataVector.elementAt (series);
        return nps.getNodeID ();
    }

    public int getItemCount (int series)
    {
        NodePositionSeries nps = (NodePositionSeries) _dataVector.elementAt (series);
        return nps.getItemCount ();
    }

    public Number getX (int series, int item)
    {
        NodePositionSeries nps = (NodePositionSeries) _dataVector.elementAt (series);
        return nps.getX (item);
    }

    public Number getY (int series, int item)
    {
        NodePositionSeries nps = (NodePositionSeries) _dataVector.elementAt (series);
        return nps.getY (item);
    }

    public class Datapoint
    {
        Double x;
        Double y;
    }

    public class NodePositionSeries
    {
        public NodePositionSeries (String nodeID)
        {
            _id = nodeID;
            _numPoints = 0;
            _data = new Datapoint[200];
        }

        public String getNodeID()
        {
            return _id;
        }

        public int getItemCount()
        {
            return _numPoints;
        }

        public Number getX (int item)
        {
            return _data[item].x;
        }

        public Number getY (int item)
        {
            return _data[item].y;
        }

        public void addDatapoint (double x, double y)
        {
            if (_numPoints == _data.length) {
                Datapoint[] oldData = _data;
                _data = new Datapoint[oldData.length+200];
                System.arraycopy (oldData, 0, _data, 0, oldData.length);
            }
            _data[_numPoints] = new Datapoint();
            _data[_numPoints].x = new Double (x);
            _data[_numPoints].y = new Double (y);
            _numPoints++;
        }

        private String _id;
        private int _numPoints;
        private Datapoint[] _data;

    }

    private Vector _dataVector = new Vector();
}
