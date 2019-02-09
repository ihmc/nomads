package us.ihmc.aci.nodemon.controllers.delivery;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Container;
import us.ihmc.aci.ddam.DataType;
import us.ihmc.aci.ddam.Node;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.Controller;
import us.ihmc.aci.nodemon.controllers.throughput.ThroughputController;

import java.util.Collection;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * DeliveryController.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
abstract class DeliveryController implements Runnable, Controller
{
    @Override
    public boolean isRunning ()
    {
        return _isRunning.get();
    }

    @Override
    public void start ()
    {
        (new Thread(this, _clazz.getSimpleName())).start();
    }

    @Override
    public void stop ()
    {
        _isRunning.set(false);
    }

    //to be subclassed
    protected Collection<Container> getData (Node node)
    {
        return _data;
    }

    //to be subclassed
    protected DataType getType ()
    {
        return _type;
    }

    @Override
    public void run ()
    {
        log.debug(_clazz.getSimpleName() + "Thread started, delivering every " + _deliveryInterval + "msecs");
        _intervalsCounter = 0;
        _isRunning.set(true);
        while (_isRunning.get()) {

            try {
                //always deliver the fist 2 messages after an internal less or equal than 10000 msecs
                if (_intervalsCounter <= 2) {
                    long initialDeliveryInterval = 10000;
                    if (_deliveryInterval < initialDeliveryInterval) {
                        initialDeliveryInterval = _deliveryInterval;
                    }
                    TimeUnit.MILLISECONDS.sleep(initialDeliveryInterval);
                }
                else {
                    TimeUnit.MILLISECONDS.sleep(_deliveryInterval);
                }
                _intervalsCounter++;

                if (_deliveryInterval == 0) {
                    continue;
                }

                for (Container cm : getData(_nodeMon.getWorldState().getLocalNodeCopy())) {
                    //proxy and node to node with different throughput policies
                    for (ThroughputController tc : _throughputControllers) {
                        tc.addMessage(cm);
                    }
                }

            }
            catch (Exception e) {
                log.error("Error while delivering: ", e);
            }

        }
    }

    private long _intervalsCounter;
    protected AtomicBoolean _isRunning;
    protected NodeMon _nodeMon;
    protected List<ThroughputController> _throughputControllers;
    protected DataType _type;
    protected Collection<Container> _data;
    protected long _deliveryInterval;
    protected int _timeWindow;
    protected Class _clazz;

    protected boolean _isMaster;
    protected boolean _isMasterForwardingLocals;
    protected String _localGroup;
    protected String _mastersGroup;

    protected static Logger log = Logger.getLogger(DeliveryController.class);
}
