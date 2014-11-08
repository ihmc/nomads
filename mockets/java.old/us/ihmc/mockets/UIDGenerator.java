package us.ihmc.mockets;

import java.util.logging.Logger;

/**
 * The UIDGenerator class is a simple Unique ID generator for mockets.
 *
 * @author Mauro Tortonesi
 */
class UIDGenerator
{
    private static long _counter = 0;
    private static Logger _logger = null;
    
    static long getUID () {
        if (_logger == null) {
            assert _counter == 0;
            _logger = Logger.getLogger ("us.ihmc.mockets");
        }
        return _counter++;
    }
}
/*
 * vim: et ts=4 sw=4
 */
