package us.ihmc.android.aci.dspro.main;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Properties;

import us.ihmc.android.aci.dspro.R;
import us.ihmc.android.aci.dspro.main.peers.NMSProxyService;
import us.ihmc.android.aci.dspro.main.peers.Peer;
import us.ihmc.android.aci.dspro.main.peers.PeersAdapter;
import us.ihmc.android.aci.dspro.util.AndroidUtil;

/**
 * Created by kristyna on 5/3/18.
 *
 * Activity that checks the current unicast and multicast traffic from the peers
 */
public class PeersFragment extends BaseStatsFragment {

    private ListView lv = null;
    private TextView incomingTraffictv;

    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = null;
        try {
            v = inflater.inflate(R.layout.tab_network_monitor, container, false);
        } catch (InflateException e) {
            e.getStackTrace();
        }

        if (v == null)
            return null;

        /**
         * Setting the color of the actionBar
         */
        String configFilePath = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) + getString(R.string.app_config_file);
        Properties config = AndroidUtil.readConfigFile(configFilePath);
        String encryptionMode = config.getProperty(getString(R.string.aci_disService_networkMessageService_encryptionMode));
        AndroidUtil.changeActionBarColor(mActivity, encryptionMode);
        changedStatus("onCreate()");

        lv = v.findViewById(R.id.peers_info_lv);
        incomingTraffictv = v.findViewById(R.id.incoming_traffic_tv);

        // Display the message No peers found
        ArrayList<Peer> peers = new ArrayList<Peer>();
        updateAdapter(peers);

        return v;
    }

    private Boolean isReady() {
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(mActivity);
        if (settings.getString(getString(R.string.aci_dspro2app_selectMode), getString(R.string.aci_dspro2app_selectMode_default)).equals("DisService")) {
            Toast.makeText(mActivity, "Feature not supported with DisService", Toast.LENGTH_LONG).show();
            return false;
        }

        //check if "delete storage on exit" is checked
        if (!settings.getBoolean(getString(R.string.aci_dspro2app_proxy_enable), Boolean.valueOf(getString(R.string.aci_dspro2app_proxy_enable_default)))) {
            Toast.makeText(mActivity, "Enable \"Attach DSPro proxy\" from Settings menu", Toast.LENGTH_LONG).show();
            return false;
        }

        if (!mActivity.isDSProServiceRunning()) {
            Toast.makeText(mActivity, "DSPro service not running", Toast.LENGTH_SHORT).show();
            return false;
        }

        return true;
    }

    private void updateAdapter (ArrayList<Peer> peers)
    {
        //LOG.debug("updateAdapert() called with parameter: " + peers);
        if (peers == null)
            return;

        //if no peers display the simple peers list with no peers
        if (peers.size() == 0) {
            ArrayList<String> peerStringList = new ArrayList<>();
            peerStringList.add(0, "No available network info yet");
            ArrayAdapter<String> peerStringAdapter = new ArrayAdapter<>(mActivity, R.layout.item_peer, peerStringList);
            lv.setAdapter(peerStringAdapter);
            peerStringAdapter.notifyDataSetChanged();
        } else {
            PeersAdapter peersInfoAdapter = new PeersAdapter(mActivity, peers);
            lv.setAdapter(peersInfoAdapter);
            peersInfoAdapter.notifyDataSetChanged();
        }
    }


    @SuppressLint("HandlerLeak")
    private Handler _activityHandler = new Handler()
    {
        @Override
        public void handleMessage (Message msg)
        {
            int type = msg.arg1;
            //LOG.debug("_activityHandler, received a message of type: " + type);
            Activity activity = getActivity();
            if (activity != null) {
                if(!isAdded()) {
                    return;
                }
            } else {
              return;
            }
            switch (type) {
                case 8: {
                    Bundle bundle = msg.getData();
                    if (bundle == null) {
                        LOG.debug("handleMessage(): bundle is null, returning");
                    }

                    Peer[] peersBundle = (Peer[]) bundle.getParcelableArray(getString(R.string.peers_info));
                    if (peersBundle != null) {
                        ArrayList<Peer> peers = new ArrayList<>(Arrays.asList(peersBundle));
                        updateAdapter(peers);
                    } else
                        LOG.warn("Peer list is null");
                    break;
                }
                case 9: {
                    Bundle bundle = msg.getData();
                    if (bundle == null) {
                        LOG.debug("handleMessage(): bundle is null, returning");
                    }
                    //get Rate per second from the bundle
                    String ratePerSecond = bundle.getString(getString(R.string.incoming_rate_info));
                    if (ratePerSecond != null) {
                        String incomingTrafficValue = getString(R.string.incoming_traffic_info) + " " + ratePerSecond;
                        if (incomingTraffictv == null) {
                            return;
                        }
                        incomingTraffictv.setText(incomingTrafficValue);
                    } else {
                        LOG.debug("handleMessage(): ratePerSecond is null");
                    }
                    break;
                }
            }
        }
    };

    private void bind()
    {
        LOG.info("Binding to " + NMSProxyService.class.getSimpleName());
        Intent i = new Intent(mActivity, NMSProxyService.class);
        mActivity.bindService(i, _connection, Context.BIND_AUTO_CREATE);
    }

    public void unbind ()
    {
        if (mIsBound) {
            LOG.warn("Unbinding " + NMSProxyService.class.getSimpleName());
            mActivity.unbindService(_connection);
            mIsBound = false;
        }
    }

    @Override
    public void onResume()
    {
        changedStatus("onResume()");
        unbind();
        bind();
        super.onResume();
    }

    @Override
    public void onDestroy ()
    {
        changedStatus("onDestroy()");
        unbind();
        super.onDestroy();
    }

    @Override
    public void onDestroyView()
    {
        unbind();
        super.onDestroyView();
    }

    @Override
    public Handler getActivityHandler() {
        return _activityHandler;
    }

    @Override
    public String getSimpleName() {
        return PeersFragment.class.getSimpleName();
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        if (isVisibleToUser && mActivity != null)
            isReady();
    }

}
