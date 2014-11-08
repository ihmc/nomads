/**
 * The FlowACKManager class takes care of per-flow ACK management for message 
 * mockets.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;

import java.util.logging.Logger;

class FlowACKManager
{
    FlowACKManager (long lastCumulativeAck, boolean control, boolean sequenced) 
    {
        // make sure sequenced is false if control is true
        assert !control || !sequenced;

        _logger = Logger.getLogger ("us.ihmc.mockets");

        _control = control;
        _sequenced = sequenced;
        _tsnRangeHandler = new TSNRangeHandler (control, sequenced);

        _lastCumulativeAck = lastCumulativeAck;
    }

    long getLastCumulativeAck()
    {
        return _lastCumulativeAck;
    }

    /* returns true if the packet was processed, false if it was simply
     * discarded because already present */
    synchronized boolean receivedPacket (long seqNum) 
    {
        // THIS IS JUST A SANITY CHECK, SINCE THE RECEIVER SHOULD DISCARD
        // DUPLICATED PACKETS *BEFORE* THE NOTIFICATION TO ACKMANAGER
        // notice that we CANNOT use subtract here because the normalization
        // performed by that method completely breaks the comparison
        if (SequentialArithmetic.greaterThanOrEqual (_lastCumulativeAck, seqNum)) {
            // this is a duplicate packet, so just silently ignore it
            // we don't need to notify the mocket of the duplicated packet
            // because that's one of the tasks accomplished by the receiver 
            _logger.info ("FLOWACKMANAGER: received packet already accounted 1");
            return false;
        }
        
        boolean ret = _tsnRangeHandler.packetProcessed (seqNum);

        // update _lastCumulativeAck if needed
        if (ret) {
            _logger.info ("FLOWACKMANAGER: accounting received packet: " + seqNum);
            TSNRangeHandler.Range r = _tsnRangeHandler.peek();
            if (r.begin() == SequentialArithmetic.add(_lastCumulativeAck, 1)) {
                _logger.info ("FLOWACKMANAGER: cumulative TSN updated to: " + r.end() + " was: " + _lastCumulativeAck);
                _lastCumulativeAck = r.end();
                _tsnRangeHandler.removeFirstRange();
            }
        }
        else 
            _logger.info ("FLOWACKMANAGER: received packet already accounted 2");

        return ret;
    }
    
    synchronized void fillACKInformation (ACKInformation ai) 
    {
        _tsnRangeHandler.fillACKInformation (ai);
    }

    void _dump (PrintStream os)
    {
        os.println ("_lastCumulativeAck: " + _lastCumulativeAck);
        _tsnRangeHandler._dump (os);
    }
    
    private Logger _logger;
    private long _lastCumulativeAck;
    private boolean _control;
    private boolean _sequenced;
    private TSNRangeHandler _tsnRangeHandler;
}

/*
 * vim: et ts=4 sw=4
 */

