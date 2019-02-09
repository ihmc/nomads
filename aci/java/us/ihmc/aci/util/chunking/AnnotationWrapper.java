package us.ihmc.aci.util.chunking;

import us.ihmc.chunking.Interval;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class AnnotationWrapper extends us.ihmc.chunking.AnnotationWrapper {

    public AnnotationWrapper()
    {
        super();
    }

    public void read (CommHelper _commHelper) throws CommException, ProtocolException
    {
        super.setData(_commHelper.receiveBlock());
        byte nIntervals = _commHelper.read8();
        for (byte i = 0; i < nIntervals; i++) {
            us.ihmc.aci.util.chunking.Interval interv = new us.ihmc.aci.util.chunking.Interval();
            interv.read (_commHelper);
            getIntervals().add (interv);
        }
    }
   
    public void write(CommHelper _commHelper) throws CommException, ProtocolException
    {
        _commHelper.sendBlock (getData());
        _commHelper.write8 ((byte)getIntervals().size());
        for (Interval interv : getIntervals()) {
            ((us.ihmc.aci.util.chunking.Interval)interv).write (_commHelper);
        }
    }
}
