package us.ihmc.android.aci.dspro.main.peers;

import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.preference.PreferenceManager;
import android.support.annotation.Nullable;
import android.util.Log;

import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import us.ihmc.aci.dspro2.AsyncDSProProxy;
import us.ihmc.android.aci.dspro.ActivityHandler;
import us.ihmc.android.aci.dspro.DSProActivity;
import us.ihmc.android.aci.dspro.MessageType;
import us.ihmc.android.aci.dspro.R;
import us.ihmc.android.aci.dspro.util.AndroidUtil;
import us.ihmc.comm.CommException;
import us.ihmc.nms.MessageArrived;
import us.ihmc.nms.NMSProxy;
import us.ihmc.nms.NetworkMessageServiceListener;

/**
 * Created by Filippo Poltronieri on 10/4/17.
 * NMSProxyService used to collect data from peers
 * <fpoltronieri@ihmc.us>
 */

public class NMSProxyService extends Service {

    private ActivityHandler _activityHandler = new ActivityHandler();
    Messenger _serviceMessenger = new Messenger(_activityHandler.fromActivity);

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        startMonitor();
        storeHash();
        return _serviceMessenger.getBinder();
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Log.d(TAG, "onUnbind()");
        return false;
    }

    public void startMonitor() {
        String methodName = "startMonitor(): ";
        try {
            if (_nmsProxy == null) {
                new Thread(new NMSProxyRunnable(1000)).start();
            } else {
                Log.d(TAG, "NMS monitor already running");
            }
        } catch (Exception e) {
            Log.d(TAG, methodName + "Exception caugh, printing the StacKTrace");
            e.printStackTrace();
        }
    }


    private TimerTask task = new TimerTask() {
        @Override
        public void run() {
            Message message = Message.obtain();
            message.arg1 = MessageType.PEER_TRAFFIC_INFO.code();
            Peer[] peersArray = new Peer[peers.size()];
            peers.values().toArray(peersArray);
            Bundle bundle = new Bundle();
            bundle.putParcelableArray(getString(R.string.peers_info), peersArray);
            message.setData(bundle);
            _activityHandler.toActivity.sendMessage(message);
        }
    };

    private TimerTask trafficTask = new TimerTask() {
        @Override
        public void run() {
            long receivedTrafficLastWindow = mincomingTrafficRate;
            mincomingTrafficRate = 0;
            Message message = Message.obtain();
            message.arg1 = MessageType.INCOMING_RATE_INFO.code();
            Bundle bundle = new Bundle();
            bundle.putString(getString(R.string.incoming_rate_info), calculateTrafficRate(receivedTrafficLastWindow));
            message.setData(bundle);
            _activityHandler.toActivity.sendMessage(message);
        }
    };


    /**
     * This function will return the incoming traffic received within the last second
     * in bit/s
     * @param trafficSize
     * @return
     */
    private static String calculateTrafficRate(long trafficSize) {
        //from bytes to bit
        long trafficNumericValue = trafficSize * 8;
        if (trafficNumericValue <= 1024) {
            return trafficNumericValue + "/s";
        } else if (trafficNumericValue > 1024 && trafficNumericValue <= 1048576) {
            return (trafficNumericValue / 1024 ) + "K/s";
        } else if (trafficNumericValue > 1048576 && trafficNumericValue <= 1073741824) {
            return (trafficNumericValue / 1048576) + "M/s";
        } else {
            return (trafficNumericValue / 1073741824) + "G/s";
        }
    }


    class NMSProxyRunnable implements Runnable, NetworkMessageServiceListener {

        private int _delay;

        public NMSProxyRunnable(int delay) {
            _delay = delay;
        }

        public void run() {
            Log.d(TAG, "NMSProxyRunnable::run(): NMSProxy thread started");
            try {
                Thread.sleep(_delay);
            } catch (InterruptedException e) {
                Log.d(TAG, AndroidUtil.getStackTraceAsString(e));
            }
            boolean initialized = false;
            while (!initialized) {
                //initialize proxy
                _nmsProxy = new NMSProxy(getString(R.string.localhost), NMSProxy.DEFAULT_PORT);
                try {
                    if (_nmsProxy.init() == 0) {
                        initialized = true;
                        try {
                            _nmsProxy.registerNetworkMessageServiceListener((byte) 'd',
                                    this);
                            timer.schedule(task, 0, 1000);
                            //calculate the traffic within the last second
                            timer.schedule(trafficTask, 1000, 1000);
                        } catch (CommException e) {
                            e.printStackTrace();
                            initialized = false;
                        }
                    } else {
                        Log.d(TAG, "NMSProxyRunnable::run(): Error while initializing NMSProxy");
                        try {
                            Thread.sleep(3000);
                        } catch (InterruptedException e) {
                            Log.d(TAG, AndroidUtil.getStackTraceAsString(e));
                        }
                    }
                } catch (Exception e) {
                    Log.d(TAG, AndroidUtil.getStackTraceAsString(e));
                    try {
                        Thread.sleep(3000);
                    } catch (InterruptedException ex) {
                       Log.d(TAG, AndroidUtil.getStackTraceAsString(ex));
                   }
                }
            }
        }


        @Override
        public void messageArrived(MessageArrived messageArrived) {
            //messageArrived, collecting the status
            boolean isUnicast = messageArrived.isUnicast();
            String peerName = messageArrived.getSrcIPAddr();
            Peer peer;
            if (!peers.containsKey(peerName)) {
                peer = new Peer(peerName);
                peers.put(peerName, peer);
            } else {
                peer = peers.get(peerName);
            }
            if (isUnicast)
                peer.setUnicastMessages(messageArrived.getGroupMsgCount());
            else
                peer.setMulticastMessages(messageArrived.getGroupMsgCount());
            peers.put(peerName, peer);
            mincomingTrafficRate += messageArrived.getData().length;
        }

    }

    private void storeHash() {
        Log.d(TAG, "StoreHash: Started");
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                if (_nmsProxy != null) {
                    try {
                        String hash = _nmsProxy.getEncryptionKeyHash();
                        SharedPreferences.Editor editor = sp.edit();
                        editor.putString(getString(R.string.sp_encryption_key_hash), hash).apply();
                        editor.commit();
                        Log.d(TAG, "StoreHash: Hash set to " + hash);
                    } catch (Exception e) {
                        SharedPreferences.Editor editor = sp.edit();
                        editor.putString(getString(R.string.sp_encryption_key_hash), "").apply();
                        editor.commit();
                        Log.d(TAG, "StoreHash: Hash set to empty string");
                    }
                } else {
                    Log.d(TAG, "StoreHash: NMSProxy is null");
                }
            }
        }, 0, 3000);

    }

    private Timer timer = new Timer();
    private static ConcurrentHashMap<String, Peer> peers = new ConcurrentHashMap<>();
    private final static String TAG = NMSProxyService.class.getSimpleName();
    private long mincomingTrafficRate = 0;
    private static NMSProxy _nmsProxy = null;
}
