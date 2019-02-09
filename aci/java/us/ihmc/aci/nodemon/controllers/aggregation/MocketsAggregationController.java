package us.ihmc.aci.nodemon.controllers.aggregation;

import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.Controller;
import us.ihmc.aci.nodemon.data.process.MocketsContent;
import us.ihmc.aci.nodemon.data.process.Process;
import us.ihmc.aci.nodemon.data.process.ProcessStats;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.util.Config;

import java.util.Objects;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * MocketsAggregationController.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class MocketsAggregationController implements Runnable, Controller
{
    public MocketsAggregationController (NodeMon nodeMon)
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _isRunning = new AtomicBoolean(false);
    }

    @Override
    public void start ()
    {
        (new Thread(this, "MocketsAggregationController")).start();
    }

    @Override
    public boolean isRunning ()
    {
        return _isRunning.get();
    }

    @Override
    public void stop ()
    {
        _isRunning.set(false);
    }

    @Override
    public void run ()
    {
        _isRunning.set(true);
        while (_isRunning.get()) {

            try {
                TimeUnit.MILLISECONDS.sleep(Config.getIntegerValue(Conf.NodeMon.AGGREGATION_INTERVAL,
                        DefaultValues.NodeMon.AGGREGATION_INTERVAL));
            }
            catch (InterruptedException e) {
                e.printStackTrace();
            }

            try {

                if (_nodeMon.getWorldState().getLocalNode() == null ||
                        _nodeMon.getWorldState().getLocalNode().getInfo() == null ||
                        _nodeMon.getWorldState().getLocalNode().getInfo().getNicsList() == null ||
                        _nodeMon.getWorldState().getLocalNode().getInfo().getNicsCount() == 0) {
                    log.trace("$$ Info not detected on local node yet, unable to aggregate");
                    continue;
                }

                ProcessStats ps = _nodeMon.getWorldState().getProcessStatsCopy();

                if (ps.get().size() == 0) {
                    log.trace("$$ ProcessStats not detected on local node yet, unable to aggregate");
                    continue;
                }

                for (Process p : ps.get().values()) {

                    if (!p.getType().equals(Process.ProcessContentType.MOCKETS)) {
                        continue;
                    }

                    MocketsContent mc = (MocketsContent) p.getContent();
                    log.debug("$$ Found Mockets data for link (hostnames): " + mc.getLocalAddr() + " -> " + mc.getRemoteAddr());
                    log.debug("$$ Found Mockets data for link (long ip): " + mc.getLocalAddrLong()
                            + " -> " + mc.getRemoteAddrLong());
                    log.debug("$$ Found Mockets data for link (long converted): " + Utils.convertIPToString((int) mc.getLocalAddrLong())
                            + " -> " + Utils.convertIPToString((int) mc.getRemoteAddrLong()));
                    double RTT = mc.getEstimatedRTT();
                    log.debug("$$ Found estimated RTT: " + mc.getEstimatedRTT());


                    Integer localAddr = Utils.convertIPToInteger(mc.getLocalAddr());
                    if (localAddr == null) {
                        continue;
                    }
                    Integer remoteAddr = Utils.convertIPToInteger(mc.getRemoteAddr());
                    if (remoteAddr == null) {
                        continue;
                    }

                    Source source = _nodeMon.getWorldState().getLocalNodeCopy().getTraffic().getSources().get(localAddr);
                    Link link = null;
                    if (source != null) {
                        //get current link
                        link = source.getDestinations().get(remoteAddr);
                        log.debug("$$ Source (" + mc.getLocalAddr() +  ") is: " + JsonFormat.printer().print(source));
                        if (link == null) {
                            link = Link.newBuilder().build();
                            log.debug("$$ Destination not found. Creating new link: " + mc.getLocalAddr() + " -> " + mc.getRemoteAddr());
                        }
                        else {
                            log.debug("$$ Found link already present, merging: " + mc.getLocalAddr() + " -> " + mc.getRemoteAddr());
                        }
                    }
                    else {
                        link = Link.newBuilder().build();
                        log.debug("$$ Source not found. Creating new link: " + mc.getLocalAddr() + " -> " + mc.getRemoteAddr());
                    }

                    Link newLink = Link.newBuilder(link)
                            .setDescription(
                                    Description.newBuilder(link.getDescription())
                                            .setLatency((int) RTT / 2)
                                            //.setLatency(50)
                                            .build())
                            .setTimestamp(TimeUtil.getCurrentTime())
                            .build();

                    Container c = ProtoUtils.toContainer(DataType.LINK, _nodeMon.getWorldState().getLocalNodeId(), newLink);

                    _nodeMon.getWorldState().updateData(_nodeMon.getWorldState().getLocalNodeId(), c);
                }

            }
            catch (Exception e) {
                log.error("$$ Error while aggregating Mockets statistics", e);
            }
        }
    }

    private final NodeMon _nodeMon;
    private final AtomicBoolean _isRunning;
    private static final Logger log = Logger.getLogger(MocketsAggregationController.class);
}
