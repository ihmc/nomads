package us.ihmc.android.aci.dspro.main.other;

import android.app.Service;
import android.content.Intent;
import android.os.*;

import org.apache.log4j.Logger;
import us.ihmc.aci.disService.monitor.DisServiceMonitor;
import us.ihmc.aci.disService.monitor.DisServiceStatus;
import us.ihmc.aci.disService.monitor.DisServiceStatusListener;
import us.ihmc.android.aci.dspro.ActivityHandler;
import us.ihmc.android.aci.dspro.MessageType;
import us.ihmc.android.aci.dspro.R;

/**
 * MonitorService.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class MonitorService extends Service implements DisServiceStatusListener
{
    private final static Logger LOG = Logger.getLogger(MonitorService.class);
    //DSPro monitor
    private  DisServiceMonitor _monitor;

    private ActivityHandler _activityHandler = new ActivityHandler();
    Messenger _serviceMessenger = new Messenger(_activityHandler.fromActivity);

    @Override
    public IBinder onBind (Intent intent)
    {
        changedStatus("onBind()");
        startMonitor(2400);
        LOG.debug("onBind(): Monitor started");
        return _serviceMessenger.getBinder();
    }

    @Override
    public boolean onUnbind (Intent intent)
    {
        changedStatus("onUnbind()");
        _monitor = null;
        //_activityHandler.clearActivities();

        return false;
    }

    public void startMonitor (int udpPort)
    {
        LOG.debug("Called startMonitor  " + _monitor );
        if (_monitor != null) {
            //LOG.debug("Monitor is already running");
            return;
        } else  {
            LOG.debug("startMonitor(): The Monitor was not running");
        }

        try {
            _monitor = new DisServiceMonitor(udpPort);
            _monitor.setListener(this);
            _monitor.start();
            //LOG.debug("DSPro Monitor started");
        }
        catch (Exception e) {
            LOG.error("startMonitor(): Error starting DSPro Monitor");
            e.printStackTrace();
        }
    }

    public void changedStatus (String statusChanged)
    {
        //log the status change
        LOG.info("Called " + statusChanged);
    }

    void sendMessage (Bundle bundle, MessageType type)
    {
        Message msg = Message.obtain();
        msg.obj = type;
        msg.setData(bundle);
        _activityHandler.toActivity.sendMessage(msg);
    }

    @Override
    public void basicStatisticsInfoUpdateArrived (DisServiceStatus.DisServiceBasicStatisticsInfo dsbsi)
    {
        //LOG.debug("=== basicStatisticsInfoUpdateArrived===");
        Bundle bundle = new Bundle();
        bundle.putLong(getString(R.string.messagesReceived), dsbsi.dataMessagesReceived);
        bundle.putLong(getString(R.string.bytesReceived), dsbsi.dataBytesReceived);
        bundle.putLong(getString(R.string.fragmentsReceived), dsbsi.dataFragmentsReceived);
        bundle.putLong(getString(R.string.fragmentBytesReceived), dsbsi.dataFragmentBytesReceived);

        bundle.putLong(getString(R.string.kamSent), dsbsi.keepAliveMessagesSent);
        bundle.putLong(getString(R.string.kamReceived), dsbsi.keepAliveMessagesReceived);

        bundle.putLong(getString(R.string.mfrsSent), dsbsi.missingFragmentRequestMessagesSent);
        bundle.putLong(getString(R.string.mfrsBytesSent), dsbsi.missingFragmentRequestBytesSent);
        bundle.putLong(getString(R.string.mfrsReceived), dsbsi.missingFragmentRequestMessagesReceived);
        bundle.putLong(getString(R.string.mfrsBytesReceived), dsbsi.missingFragmentRequestBytesReceived);

        sendMessage(bundle, MessageType.BASIC_STATS);
    }

    @Override
    public void overallStatsInfoUpdateArrived (DisServiceStatus.DisServiceStatsInfo dssi)
    {
        //LOG.debug("===overallStatsInfoUpdateArrived===");
        Bundle bundle = new Bundle();
        bundle.putLong(getString(R.string.messagesPushed), dssi.clientMessagesPushed);
        bundle.putLong(getString(R.string.bytesPushed), dssi.clientBytesPushed);
        bundle.putLong(getString(R.string.fragmentsPushed), dssi.fragmentsPushed);
        bundle.putLong(getString(R.string.fragmentBytesPushed), dssi.fragmentBytesPushed);
        bundle.putLong(getString(R.string.odFragmentsSent), dssi.onDemandFragmentsSent);
        bundle.putLong(getString(R.string.odFragmentBytesSent), dssi.onDemandFragmentBytesSent);

        sendMessage(bundle, MessageType.OVERALL_STATS);
    }

    @Override
    public void duplicateTrafficStatisticInfoUpdateArrived (DisServiceStatus.DisServiceDuplicateTrafficInfo dsdti)
    {
        //LOG.debug("===duplicateTrafficStatisticInfoUpdateArrived===");
        Bundle bundle = new Bundle();
        bundle.putLong(getString(R.string.dtRecNoTarget), dsdti.overheardDuplicateTraffic);
        bundle.putLong(getString(R.string.dtRecTargetNode), dsdti.targetedDuplicateTraffic);

        sendMessage(bundle, MessageType.DUPLICATE_TRAFFIC_STATS);
    }

    @Override
    public void peerGroupStatisticInfoUpdateArrived (final DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsip)
    {
        //LOG.debug("===peerGroupStatisticInfoUpdateArrived===");
        Bundle bundle = new Bundle();
        bundle.putString(getString(R.string.peerId), dsbsip.peerId);
        bundle.putLong(getString(R.string.kamReceived), dsbsip.keepAliveMessagesReceived);
        bundle.putLong(getString(R.string.mfrsReceived), dsbsip.missingFragmentRequestMessagesReceived);
        bundle.putLong(getString(R.string.mfrsBytesReceived), dsbsip.missingFragmentRequestBytesReceived);
        bundle.putLong(getString(R.string.messagesReceived), dsbsip.dataMessagesReceived);
        bundle.putLong(getString(R.string.bytesReceived), dsbsip.dataBytesReceived);
        bundle.putLong(getString(R.string.fragmentsReceived), dsbsip.dataFragmentsReceived);
        bundle.putLong(getString(R.string.fragmentBytesReceived), dsbsip.dataFragmentBytesReceived);

        sendMessage(bundle, MessageType.PEER_GROUP_STATS);

    }
}
