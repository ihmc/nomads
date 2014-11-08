/**
 * The ACKManager class takes care of ACK management for message mockets.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;

class ACKManager
{
    ACKManager (long initialRelSeqTSN, long initialControlTSN, 
                long initialRelUnseqID) 
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        _controlManager  = new FlowACKManager (SequentialArithmetic.subtract (initialControlTSN, 1), true, false);
        _relSeqManager   = new FlowACKManager (SequentialArithmetic.subtract (initialRelSeqTSN, 1), false, true);
        _relUnseqManager = new FlowACKManager (SequentialArithmetic.subtract (initialRelUnseqID, 1), false, false);

        _lastUpdateTimeStamp = 0;
    }

    synchronized boolean haveNewInformationSince (long time) 
    {
        return (time < _lastUpdateTimeStamp ? true : false);
    }
    
    synchronized boolean receivedReliableSequencedPacket (long seqNum) 
    {
        if (_relSeqManager.receivedPacket(seqNum)) {
            _lastUpdateTimeStamp = System.currentTimeMillis();
            return true;
        }
        return false;
    }
    
    synchronized boolean receivedReliableUnsequencedPacket (long seqNum) 
    {
        if (_relUnseqManager.receivedPacket(seqNum)) {
            _lastUpdateTimeStamp = System.currentTimeMillis();
            return true;
        }
        return false;
    }
    
    synchronized boolean receivedControlPacket (long seqNum) 
    {
        if (_controlManager.receivedPacket(seqNum)) {
            _lastUpdateTimeStamp = System.currentTimeMillis();
            return true;
        }
        return false;
    }
    
    synchronized SACKInformation getSACKInformation()
    {
        ACKInformation ai = new ACKInformation();

        _controlManager.fillACKInformation (ai);
        _relSeqManager.fillACKInformation (ai);
        _relUnseqManager.fillACKInformation (ai);
        
        return new SACKInformation (_controlManager.getLastCumulativeAck(),
                                    _relSeqManager.getLastCumulativeAck(),
                                    _relUnseqManager.getLastCumulativeAck(),
                                    ai);
    }

    private Logger _logger;
    private FlowACKManager _controlManager;
    private FlowACKManager _relSeqManager;
    private FlowACKManager _relUnseqManager;
    private long _lastUpdateTimeStamp;
}

/*
 * vim: et ts=4 sw=4
 */
