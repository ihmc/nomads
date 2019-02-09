package us.ihmc.android.aci.dspro;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.*;
import android.os.*;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.view.ViewGroup;
import android.widget.*;

import us.ihmc.android.aci.dspro.log.LogActivity;
import us.ihmc.android.aci.dspro.log.LogLevel;
import us.ihmc.android.aci.dspro.main.BasicStatsFragment;
import us.ihmc.android.aci.dspro.main.MoreStatsFragment;
import us.ihmc.android.aci.dspro.main.PeersFragment;
import us.ihmc.android.aci.dspro.main.other.ViewPagerAdapter;
import us.ihmc.android.aci.dspro.main.peers.Peer;
import us.ihmc.android.aci.dspro.pref.PropertiesActivity;
import us.ihmc.android.aci.dspro.pref.SettingsActivity;
import us.ihmc.android.aci.dspro.util.AndroidUtil;
import us.ihmc.android.aci.dspro.util.SortedProperties;
import de.mindpipe.android.logging.log4j.LogConfigurator;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

/**
 * DSProActivity.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DSProActivity extends AppCompatActivity {

    private SortedProperties _config;
    private Messenger _messenger;
    private IntentsReceiver _intentsReceiver;
    private boolean _intentsReceiverRegistered = false;
    private boolean _isBound = false;
    private final static Logger LOG = Logger.getLogger(DSProActivity.class);
    private static SharedPreferences.OnSharedPreferenceChangeListener listener = null;
    private Timer timer = new Timer();
    private Timer peerTimer = null;


    class IntentsReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            LOG.info("onReceive(): ***IntentsReceiver - received intent with action: " + action);
            /*if (action != null && (action.equalsIgnoreCase(getString(R.string.intent_start_service)))) {
                boolean isBackground = action.equalsIgnoreCase(getString(R.string.intent_start_service_background));
                String sentSessionKey = intent.getStringExtra(getString(R.string.intent_session_key));
                if (sentSessionKey != null) {
                    LOG.info("onReceive(): Extra desired session key: " + sentSessionKey);
                    _config.setProperty(getString(R.string.aci_dspro_sessionKey), sentSessionKey);
                    final EditText etSessionKey = findViewById(R.id.etSessionKey);
                    etSessionKey.setText(sentSessionKey);
                }
                if (isDSProServiceRunning()) {
                    stopServices();
                }
                String buildVersion = SimpleDateFormat.getInstance().format(new Date(BuildConfig.TIMESTAMP));
                setSelectedMode(getResources().getStringArray(R.array.pref_select_mode_array_entries)[0]);
                start(true, buildVersion);
                if (isBackground) {
                    finish();
                }
            }*/
        }
    }

    /**
     * Defines callbacks for service binding, passed to bindService()
     */
    private ServiceConnection _connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            if (service == null) {
                LOG.warn("Service connected but IBinder is null, unable to proceed");
                return;
            }

            String msgService = "Connected to " + DSProProxyService.class.getSimpleName();
            LOG.info(msgService);
            _service = service;
            _isBound = true;

            //Do the handshake with Service in order to receive messages from it
            if (_messenger == null) {
                _messenger = new Messenger(service);
                Message handshake = Message.obtain();
                handshake.obj = DSProActivity.class.getSimpleName();
                handshake.arg1 = MessageType.HANDSHAKE.code();
                handshake.replyTo = new Messenger(_activityHandler);

                try {
                    _messenger.send(handshake);
                } catch (RemoteException e) {
                    LOG.warn("onServiceConnected(): Unable to send initial handshake message to Service", e);
                }

            }
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            LOG.debug("Called onServiceDisconnected");
        }

        IBinder _service;
    };

    @SuppressLint("HandlerLeak")
    private Handler _activityHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            int type = msg.arg1;
            //LOG.debug("handleMessage: Received message of type " + type);
            Bundle bundle = msg.getData();
            switch (type) {
                case 7:
                    //Received peerList update()
                    if (bundle != null) {
                        ArrayList<String> peers = bundle.getStringArrayList(getString(R.string.peers));
                        if (peers != null) {
                            updateAdapter(peers);
                        }
                    }
                    break;

                case 99:
                    //Received peerList update()
                    if (bundle != null) {
                        String hash = bundle.getString(getString(R.string.sp_encryption_key_hash));
                        TextView tv = (TextView)findViewById(R.id.tvEncryptionHashValue);
                        tv.setText(hash);
                    }
                    break;

                case 10:
                    if (bundle != null) {
                        String sessionKey = bundle.getString(getString(R.string.session_key));
                        if (sessionKey != null) {
                            EditText etSessionKey = (EditText)findViewById(R.id.etSessionKey);
                            if (etSessionKey != null) {
                                etSessionKey.setText(sessionKey);
                            }
                        }
                    }
                    break;
            }
        }
    };

    /**
     * Prints the current memory usage of the process.
     * <p>
     * Total private dirty memory:  is basically the amount of RAM inside the process that can not be paged to disk
     * (it is not backed by the same data on disk), and is not shared with any other processes.
     * Another way to look at this is the RAM that will become available to the system when that process goes away
     * (and probably quickly subsumed into caches and other uses of it).
     * <p>
     * Total shared dirty memory:  The memory that is currently shared across multiple processes of the system.
     * <p>
     * Total PSS memory: The Pss number is a metric the kernel computes that takes into account memory sharing.
     * Each page of RAM in a process is scaled by a ratio of the number of other processes also using that page.
     * This way you can add up the pss across all processes to see the total RAM they are using, and compare pss
     * between processes to get a rough idea of their relative weight.
     */
    class MemoryStatsTask extends AsyncTask<String, Integer, String> {

        @Override
        protected String doInBackground(String... params) {
            while (true) {
                //get and print memory usage information
                Debug.MemoryInfo dbm = new Debug.MemoryInfo();
                Debug.getMemoryInfo(dbm);
                int privateMem = dbm.getTotalPrivateDirty() / 1024;
                int sharedMem = dbm.getTotalSharedDirty() / 1024;
                int totalMem = dbm.getTotalPss() / 1024;
                publishProgress(privateMem, sharedMem, totalMem);
                try {
                    Thread.sleep(3000);
                } catch (InterruptedException e) {
                    LOG.error(AndroidUtil.getStackTraceAsString(e));
                }
            }
        }

        protected void onProgressUpdate(Integer... progress) {
            //LOG.debug("MemoryTask::onProgressUpdate(): Mem consumption " +  progress[2]);
            final TextView tvTotalMem = findViewById(R.id.tvTotalMemValue);
            tvTotalMem.setText(" " + progress[2] + " Mb");
        }

        protected void onPostExecute(String result) {
            //LOG.warn("MemoryTask::onPostExectute(): MemoryStatsTask completed");
        }
    }


    class PeersTimerTask extends TimerTask {

        @Override
        public void run() {
            if (isDSProServiceRunning()) {
                Message updatePeers = Message.obtain();
                updatePeers.arg1 = MessageType.PEER_LIST.code();
                updatePeers.obj = DSProActivity.class.getName();
                updatePeers.replyTo = new Messenger(_activityHandler);
                if (_messenger != null) {
                    try {
                        _messenger.send(updatePeers);
                    }catch (RemoteException e) {
                        LOG.debug("PeersTimerTask::send impossible to send updated peer list");
                    }
                }
            }
        }
    }

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        ConfigureLog4J.configure(this.getBaseContext());

        //load configuration
        loadConfig();

        //create receiver
        _intentsReceiver = new IntentsReceiver();

        //never put the WiFi to sleep
        setWiFiSleepPolicy();

        super.onCreate(savedInstanceState);
        setContentView(R.layout.fragment_main);

        //main configuration
        setNodeUUID();
        setNetworkInterface();

        setupViewPager();
        setupViews();

        //Start memory stats thread and peersUpdaterTask
        new MemoryStatsTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        if (peerTimer != null) {
            peerTimer.cancel();
            peerTimer = null;
        }
        peerTimer = new Timer();
        peerTimer.schedule(new PeersTimerTask(), 1000, 2000);
    }

    private void setupViews() {
        final Spinner logLevel = findViewById(R.id.spnLogLevel);
        List<String> levels = new ArrayList<>();
        for (LogLevel l : LogLevel.values()) {
            levels.add(l.toString());
        }
        ArrayAdapter logLevelAdapter = new ArrayAdapter<>(this, R.layout.item_spinner, levels);
        logLevel.setAdapter(logLevelAdapter);
        //set default log to info
        logLevel.setSelection(LogLevel.Info.getLevel());

        //build version
        final String buildVersion = SimpleDateFormat.getInstance().format(new Date(BuildConfig.TIMESTAMP));
        final TextView tvBuildVersion = findViewById(R.id.tvBuildVersion);
        tvBuildVersion.append(buildVersion);

        //load previous mode selected
        String modeSelected = _config.getProperty(getString(R.string.aci_dspro2app_selectMode));

        final Spinner selectMode = findViewById(R.id.spnSelectMode);
        selectMode.setAdapter(ArrayAdapter.createFromResource(this, R.array.pref_select_mode_array_entries, R.layout.item_spinner));
        if (modeSelected != null) {
            String[] selectModeEntries = getResources().getStringArray(R.array.pref_select_mode_array_entries);
            selectMode.setSelection(modeSelected.equals(selectModeEntries[0]) ? 0 : 1);
        }

        selectMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String modeSelected = selectMode.getSelectedItem().toString();
                setSelectedMode(modeSelected);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        //hide progress bar by default
        final ProgressBar progressBar = findViewById(R.id.progressBar);
        progressBar.setVisibility(View.INVISIBLE);

        //Max memory allowed
        final TextView tvMaxMemory = findViewById(R.id.tvMaxMemValue);
        ActivityManager am = (ActivityManager) getSystemService(Activity.ACTIVITY_SERVICE);
        tvMaxMemory.setText(" " + am.getMemoryClass() + " Mb");

        final Button btnStart = findViewById(R.id.btnStart);
        btnStart.setOnClickListener(view -> {
            btnStart.setEnabled(false);
            btnStart.setTextColor(getResources().getColor(android.R.color.darker_gray));
            start(false, buildVersion);
        });


        final Button btnExit = findViewById(R.id.btnExit);
        btnExit.setOnClickListener(view -> {
            btnStart.setEnabled(true);
            btnStart.setTextColor(getResources().getColor(R.color.colorAccent));
            Toast.makeText(DSProActivity.this, "Service stopped. Exit...",
                    Toast.LENGTH_SHORT).show();
            exit();
        });

        final Button btnShowLog = findViewById(R.id.btnShowLog);
        btnShowLog.setOnClickListener(view -> {
            Intent intent = new Intent(getBaseContext(), LogActivity.class);
            intent.putExtra(getString(R.string.log_level), logLevel.getSelectedItem().toString());
            startActivity(intent);
        });

        final Button btnShowProperties = findViewById(R.id.btnShowProperties);
        btnShowProperties.setOnClickListener(v -> {
            Intent intent = new Intent(getBaseContext(), PropertiesActivity.class);
            startActivity(intent);
        });


        String sessionKey = (_config != null) ? _config.getProperty(getString(R.string.aci_dspro_sessionKey)) : null;
        final EditText etSessionKey = findViewById(R.id.etSessionKey);

        if (!(sessionKey == null || sessionKey.equals(""))) {
            etSessionKey.setText(sessionKey);
        }

        etSessionKey.setOnKeyListener((v, keyCode, event) -> {
            if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                    (keyCode == KeyEvent.KEYCODE_ENTER)) {

                String sessionKey1 = etSessionKey.getText().toString();
                LOG.debug("onCreate(): Setting Session Key to:  " + sessionKey1);
                if (_config != null)
                    _config.setProperty(getString(R.string.aci_dspro_sessionKey), sessionKey1);
                return true;
            }
            return false;
        });

        TextView tvEncyptionHash = findViewById(R.id.tvEncryptionHashValue);
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        LOG.debug("onCreate: About to register listener");
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                if (isDSProServiceRunning()) {
                        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
                        String hash = sp.getString(getString(R.string.sp_encryption_key_hash), "");
                        if (hash.isEmpty()) {
                            LOG.info("HashKeyListener: hash is empty");
                            hash = getString(R.string.empty_encryption_key_hash);
                        } else {
                            LOG.info("HashKeyListener: hash retrieved correctly");
                        }
                        Message message = Message.obtain();
                        Bundle bundle = new Bundle();
                        message.arg1 = 99;
                        bundle.putString(getString(R.string.sp_encryption_key_hash), hash);
                        //LOG.debug("Retrieved hash: " + hash );
                        message.setData(bundle);
                        _activityHandler.sendMessage(message);
                }
            }
        }, 0, 3000);

        //evaluate
        Intent intent = getIntent();
        LOG.info("onCreate(): received intent with action: " + intent.getAction());
        String serviceAction = intent.getAction();
        LOG.info("onCreate: Extra service action is: " + serviceAction);
        if (serviceAction != null && (serviceAction.equalsIgnoreCase(getString(R.string.intent_start_service))
                || serviceAction.equalsIgnoreCase(getString(R.string.intent_start_service_background)))) {

            boolean isBackground = serviceAction.equalsIgnoreCase(getString(R.string.intent_start_service_background));

            String sentSessionKey = intent.getStringExtra(getString(R.string.intent_session_key));
            if (sentSessionKey != null) {
                LOG.info("onCreate(): Extra desired session key: " + sentSessionKey);
                _config.setProperty(getString(R.string.aci_dspro_sessionKey), sentSessionKey);
                etSessionKey.setText(sentSessionKey);
            }

            //temporary hack - force DSPro
            setSelectedMode(getResources().getStringArray(R.array.pref_select_mode_array_entries)[0]);
            start(true, buildVersion);
            if (isBackground) {
                finish();
            }
        }
//        if (serviceAction != null && serviceAction.equalsIgnoreCase(getString(R.string.intent_restart_service))) {
//            restart(intent);
//        }
        if (serviceAction != null && serviceAction.equalsIgnoreCase(getString(R.string.intent_stop_service))) {
            exit();
        }
    }

    private void setupViewPager() {
        ViewPager viewpager = findViewById(R.id.view_pager);
        TabLayout tablayout = findViewById(R.id.tab_layout);

        List<Fragment> fragments = new ArrayList<>();
        fragments.add(new BasicStatsFragment());
        fragments.add(new MoreStatsFragment());
        fragments.add(new PeersFragment());

        List<String> titles = new ArrayList<>();
        titles.add("Basic Stats");
        titles.add("KAM/MFR/DT");
        titles.add("Network Monitor");

        viewpager.setOffscreenPageLimit(2);
        viewpager.setAdapter(new ViewPagerAdapter(getSupportFragmentManager(), fragments, titles));
        tablayout.setupWithViewPager(viewpager);
    }

    private void setSelectedMode(String modeSelected) {
        final Spinner selectMode = findViewById(R.id.spnSelectMode);
        String[] selectModeEntries = getResources().getStringArray(R.array.pref_select_mode_array_entries);
        selectMode.setSelection(modeSelected.equals(selectModeEntries[0]) ? 0 : 1);

        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        LOG.info("setSelectedMode(): Setting selected mode to: " + modeSelected);
        settings.edit().putString(getString(R.string.aci_dspro2app_selectMode), modeSelected).apply();

        if (_config != null)
            _config.setProperty(getString(R.string.aci_dspro2app_selectMode), modeSelected);
    }

    private void start(boolean fromAnotherApp, String buildVersion) {
        WifiManager wifi = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        if (wifi == null) {
            String wifiWarn = "WiFi connection unavailable";
            Toast.makeText(this, wifiWarn, Toast.LENGTH_SHORT).show();
            LOG.warn(wifiWarn);
            return;
        }

        //input validation
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        if (!settings.getBoolean(getString(R.string.aci_dspro_adaptor_mockets_enable),
                Boolean.valueOf(getString(R.string.aci_dspro_adaptor_mockets_enable))) &&
                !settings.getBoolean(getString(R.string.aci_dspro_adaptor_disservice_enable),
                        Boolean.valueOf(getString(R.string.aci_dspro_adaptor_disservice_enable_default))) &&
                !settings.getBoolean(getString(R.string.aci_dspro_adaptor_tcp_enable),
                        Boolean.valueOf(getString(R.string.aci_dspro_adaptor_tcp_enable_default)))) {
            Toast.makeText(this, "Transport not selected. Unable to start DSPro2",
                    Toast.LENGTH_SHORT).show();
            return;
        }

        if (!AndroidUtil.isValidPort(settings.getString(getString(R.string
                .aci_disService_networkMessageService_port), getString(R.string
                .aci_disService_networkMessageService_port_default)))) {
            Toast.makeText(this, "DisService port should be between 0 and 65535",
                    Toast.LENGTH_SHORT).show();
            return;
        }

        if (!AndroidUtil.isValidPort(settings.getString(getString(R.string
                .aci_dspro_adaptor_mockets_port), getString(R.string
                .aci_dspro_adaptor_mockets_port_default)))) {
            Toast.makeText(this, "Mockets should be between 0 and 65535",
                    Toast.LENGTH_SHORT).show();
            return;
        }

        final EditText etSessionKey = (EditText) findViewById(R.id.etSessionKey);
        String sessionKey = etSessionKey.getText().toString();
        if (sessionKey.trim().length() == 0) {
            LOG.debug("start(): Session key is empty, disabled");
            _config.setProperty(getString(R.string.aci_dspro_sessionKey), "");
        } else {
            LOG.debug("start(): Setting Session key to:  " + sessionKey);
            _config.setProperty(getString(R.string.aci_dspro_sessionKey), sessionKey);
        }

        writeConfig();
        LOG.info("start(): Session key is set to: " + _config.getProperty(getString(R.string.aci_dspro_sessionKey)));

        WifiManager.WifiLock wifiLock = wifi.createWifiLock(getString(R.string.app_wifi_lock));
        wifiLock.acquire();
        LOG.info("start(): Acquired WiFiLock: " + getString(R.string.app_wifi_lock));

        MulticastLock mcLock = wifi.createMulticastLock(getString(R.string.app_multicast_lock));
        mcLock.acquire();
        LOG.info("start(): Acquired MulticastLock: " + getString(R.string.app_multicast_lock));
        startServices(fromAnotherApp, buildVersion);

        //check if start of MutexLogger is required
        if (settings.getBoolean(getString(R.string.aci_dspro2app_mutexlogger_enable),
                Boolean.valueOf(getString(R.string.aci_dspro2app_mutexlogger_enable_default)))) {
            startMutexLoggerService();
        }

        //disable UI controls
        setUIBehavior(true);
        String selectModeStr = settings.getString(getString(R.string.aci_dspro2app_selectMode),
                getString(R.string.aci_dspro2app_selectMode_default));

        if (fromAnotherApp) LOG.info("start(): DSPro was started from another app");

        Toast.makeText(this, selectModeStr + " service started" + (fromAnotherApp ? ". Press back to return to the " +
                "previous app." : "."), Toast.LENGTH_LONG).show();
    }

    /**
     * Update peers in the main Activity
     *
     * @param peers
     */
    private void updateAdapter(ArrayList<String> peers) {
        //LOG.debug("updateAdapter called");

        if (peers == null) {
            //LOG.info("updateAdapter(): Peer list null");
            return;
        }

        if (peers.size() == 0) {
            peers.add(0, "No peers found.");
        }
        final TextView textView = findViewById(R.id.tvPeerList);
        if (textView != null) {
            textView.setText(getString(R.string.sPeerList));
            textView.setVisibility(View.VISIBLE);
        }
        ArrayAdapter peersAdapter = new ArrayAdapter<>(this, R.layout.item_peer, peers);
        final ListView sv = findViewById(R.id.peers_scroller);
        sv.refreshDrawableState();
        sv.setAdapter(peersAdapter);
        setListViewHeightBasedOnChildren(sv);
    }


    private void exit() {
        //input validation
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        //check if "delete storage on exit" is checked
        if (settings.getBoolean(getString(R.string.aci_dspro2app_deleteStorageOnExit),
                Boolean.valueOf(getString(R.string.aci_dspro2app_deleteStorageOnExit_default)))) {

            String dbPath = settings.getString(getString(R.string.aci_disService_storageFile),
                    getString(R.string.aci_disService_storageFile_default));
            AndroidUtil.deleteStorage(dbPath);
            AndroidUtil.deleteStorage(dbPath + "-journal");
        }

        //clean stop of the service
        stopServices();

        //send broadcast that service has been stopped
        LOG.info("exit(): Broadcasting " + getString(R.string.intent_service_stopped));
        Intent i = new Intent(getString(R.string.intent_service_stopped));
        sendBroadcast(i);

        //explicitly call garbage collector
        System.gc();
        //kill process
        LOG.info("exit(): Killing process PID: " + android.os.Process.myPid());
        android.os.Process.killProcess(android.os.Process.myPid());
    }

    private void loadConfig() {
        //get default preferences
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        PreferenceManager.setDefaultValues(this, R.xml.preferences, true);

        //get configuration file path
        String configFilePath = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) + getString(R.string.app_config_file);

        File configFile = new File(configFilePath);
        if (!configFile.exists()) {
            LOG.warn("loadConfig(): Unable to find configuration file, creating default file");
            File configDir = new File(AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) + "conf/");
            if (!configDir.exists())
                configDir.mkdir();
            AndroidUtil.writeConfigFile(getDefaultProperties(), configFilePath);
        }

        _config = AndroidUtil.readConfigFile(configFilePath);
        if (_config == null) {
            LOG.error("loadConfig(): Unable to find preference file " + configFile);
            return;
        }
        LOG.info("loadConfig(): Read preferences from file " + configFile);

        int synced = syncWithDefault(_config);
        LOG.debug("loadConfig(): Added default properties not in config file" + synced);

        // updating preferences
        // NOTE: You can't remove all preferences and make everything a String. Preference screen is expecting Boolean.
        // NOTE: If this needs to be uncommented find a way how to store Boolean as well
//        for (Map.Entry<Object, Object> entry : _config.entrySet()) {
//            LOG.trace("loadConf(): Found pref: " + entry.getKey() + ": " + entry.getValue().toString());
//            sharedPref.edit().putString(entry.getKey().toString(), entry.getValue().toString()).apply();
//        }

        // Checking Encryption mode in order to set the color of the App bar
        String encryptionMode = _config.getProperty(getString(R.string.aci_disService_networkMessageService_encryptionMode));
        AndroidUtil.changeActionBarColor(this, encryptionMode);
        Map<String, ?> keys = sharedPref.getAll();
        LOG.debug("loadConfig(): Number of saved preferences: " + keys.entrySet().size());
        for (Map.Entry<String, ?> entry : keys.entrySet()) {
            LOG.trace(entry.getKey() + ": " + entry.getValue().toString());
        }
    }

    private void setNodeUUID() {
        final TextView tvNodeUUID = findViewById(R.id.tvNodeUUIDValue);
        String nodeUUID = _config.getProperty(getString(R.string.aci_disService_nodeUUID));

        if (nodeUUID.equals(getString(R.string.aci_disService_nodeUUID_default))) {
            nodeUUID = Build.MODEL;
            _config.setProperty(getString(R.string.aci_disService_nodeUUID), Build.MODEL);
        }

        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        settings.edit().putString(getString(R.string.aci_disService_nodeUUID), nodeUUID).apply();
        tvNodeUUID.setText(" " + nodeUUID);
    }

    private String setNetworkInterface() {
        if (_config == null) {
            throw new IllegalStateException("Unable to read from the config file, check SDCard");
        }

        final Spinner spinSelect = findViewById(R.id.spnSelectInterface);
        final List<AndroidUtil.NetInterface> iFaceList = AndroidUtil.getAllAvailableNetworkInterfaces();
        //final TextView tvIPValue = (TextView) findViewById(R.id.tvIfaceValue);

        ArrayAdapter<AndroidUtil.NetInterface> netInterfaceAdapter = new ArrayAdapter<AndroidUtil.NetInterface>(this, R.layout.item_spinner, iFaceList);
        spinSelect.setAdapter(netInterfaceAdapter);

        //WiFi
        String wiFi = AndroidUtil.getWiFiIPAddress((WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE));
        _config.setProperty(getString(R.string.aci_dspro2app_WiFiIFs), wiFi);

        //Default selected
        String defaultInterface = getDefaultInterface(spinSelect, iFaceList);
        LOG.debug("setNetworkInterface(): Finding initial interface, resulting default interface: " + defaultInterface);
        updateInterface(defaultInterface);

        spinSelect.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String defaultInterface = getDefaultInterface(spinSelect, iFaceList);
                LOG.debug("setNetworkInterface(): Called onItemSelected, resulting default interface: " + defaultInterface);
                updateInterface(defaultInterface);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                //spinSelect.setSelection(iFaceList.size() - 1);
            }
        });

        return defaultInterface;
    }

    private String getDefaultInterface(Spinner selectInterface, List<AndroidUtil.NetInterface> iFaceList) {
        String defaultInterface;
        AndroidUtil.NetInterface netInterface = (AndroidUtil.NetInterface) selectInterface.getSelectedItem();
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(this);

        if (settings.getBoolean(getString(R.string.aci_dspro2app_extraNetIFs_enable), Boolean.valueOf(getString(R.string.aci_dspro2app_extraNetIFs_enable_default)))) {
            defaultInterface = settings.getString(getString(R.string.aci_dspro2app_extraNetIFs), getString(R.string.aci_dspro2app_extraNetIFs_default));
            LOG.debug("getDefaultInterface(): Using manual config. for interfaces. Setting " + getString(R.string.aci_disService_netIFs) + " to " + defaultInterface);
        } else if (netInterface.name.equals("All available")) {
            defaultInterface = AndroidUtil.getAllInterfacesDSProFormat(iFaceList);
        } else {
            defaultInterface = netInterface.ip;
        }

        return defaultInterface;
    }

    private void updateInterface(String defaultInterface) {
        LOG.debug("updateInterface(): Property " + getString(R.string.aci_disService_netIFs) + " was set to: " + _config.getProperty(getString(R.string.aci_disService_netIFs)));
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        settings.edit().putString(getString(R.string.aci_disService_netIFs), defaultInterface).apply();
        _config.setProperty(getString(R.string.aci_disService_netIFs), defaultInterface);
        LOG.debug("updateInterface(): Property " + getString(R.string.aci_disService_netIFs) + " is now set to: " + _config.getProperty(getString(R.string.aci_disService_netIFs)));
        writeConfig();
    }

    private void writeConfig() {
        String configFilePath = AndroidUtil.getExternalStorageDirectory() +
                getString(R.string.app_folder) + getString(R.string.app_config_file);
        AndroidUtil.writeConfigFile(_config, configFilePath);
    }

    private SortedProperties getDefaultProperties() {
        SortedProperties defaultPref = new SortedProperties();
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        //properties already present in Settings
        Map<String, ?> keys = sharedPref.getAll();
        for (Map.Entry<String, ?> entry : keys.entrySet()) {
            defaultPref.setProperty(entry.getKey(), entry.getValue().toString());
        }

        //extra properties not included in settings
        defaultPref.setProperty(getString(R.string.aci_disService_replication_ack),
                getString(R.string.aci_disService_replication_ack_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_replication_manager_mode),
                getString(R.string.aci_dspro_replication_manager_mode_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_scheduler_metadata_mutator),
                getString(R.string.aci_dspro_scheduler_metadata_mutator_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_scheduler_metadata_mutator_prevMsg),
                getString(R.string.aci_dspro_scheduler_metadata_mutator_prevMsg_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_metadataRanker_timeRank_considerFutureSegments),
                getString(R.string.aci_dspro_metadataRanker_timeRank_considerFutureSegments_default));
        defaultPref.setProperty(getString(R.string.aci_disService_forwarding_probability),
                getString(R.string.aci_disService_forwarding_probability_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_metadataRanker_targetStrict),
                getString(R.string.aci_dspro_metadataRanker_targetStrict_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_informationPush_rankThreshold),
                getString(R.string.aci_dspro_informationPush_rankThreshold_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_informationPull_rankThreshold),
                getString(R.string.aci_dspro_informationPull_rankThreshold_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_localNodeContext_pszTeamID),
                getString(R.string.aci_dspro_localNodeContext_pszTeamID_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_localNodeContext_pszMissionID),
                getString(R.string.aci_dspro_localNodeContext_pszMissionID_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_localNodeContext_pszRole),
                getString(R.string.aci_dspro_localNodeContext_pszRole_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_topologyExchange_enable),
                getString(R.string.aci_dspro_topologyExchange_enable_default));
        defaultPref.setProperty(getString(R.string.aci_disService_forwarding_mode),
                getString(R.string.aci_disService_forwarding_mode_default));
        defaultPref.setProperty(getString(R.string.aci_disService_propagation_targetFiltering),
                getString(R.string.aci_disService_propagation_targetFiltering_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_dsprorepctrl_prestaging_enabled),
                getString(R.string.aci_dspro_dsprorepctrl_prestaging_enabled_default));
        defaultPref.setProperty(getString(R.string.aci_disService_networkMessageService_delivery_async),
                getString(R.string.aci_disService_networkMessageService_delivery_async_default));
        defaultPref.setProperty(getString(R.string.aci_dspro_metadataRanker_timeRankWeight),
                getString(R.string.aci_dspro_metadataRanker_timeRankWeight_default));
        //Multicast address property
        defaultPref.setProperty(getString(R.string.nms_transmission_outgoingAddr),
                getString(R.string.nms_transmission_outgoingAddr_default));
        defaultPref.setProperty(getString(R.string.aci_dspro2app_proxy_enable),
                getString(R.string.aci_dspro2app_proxy_enable));

        return defaultPref;
    }

    private int syncWithDefault(SortedProperties properties) {
        int props = 0;
        LOG.debug("syncWithDefault(): Making sure that found config file is aligned with default");
        SortedProperties defaultProp = getDefaultProperties();
        for (Object key : defaultProp.keySet()) {
            String strKey = (String) key;
            if (!properties.containsKey(strKey)) {
                properties.setProperty(strKey, defaultProp.getProperty(strKey));
                props++;
            }
        }

        return props;
    }

    private void setWiFiSleepPolicy() {
        try {
            Settings.System.putInt(getContentResolver(), Settings.System.WIFI_SLEEP_POLICY, Settings.System.WIFI_SLEEP_POLICY_NEVER);
        } catch (SecurityException e) {
            LOG.warn("setWifiSleepPolicy(): Unable to set WIFI_SLEEP_POLICY to WIFI_SLEEP_POLICY_NEVER");
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Configure the application's options menu using the XML file res/menu/settings.xml.
        this.getMenuInflater().inflate(R.menu.settings, menu);

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.settings:
                startActivity(new Intent(this, SettingsActivity.class));
                return super.onOptionsItemSelected(item);
            default:
                return super.onOptionsItemSelected(item);
        }
    }


    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        bindDSPro2ProxyService();
    }

    private boolean bindDSPro2ProxyService() {
        //re-bind service
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        boolean startProxy = settings.getBoolean(getString(R.string.aci_dspro2app_proxy_enable),
                Boolean.valueOf(getString(R.string.aci_dspro2app_proxy_enable_default)));

        if (startProxy) {
            Intent i = new Intent(this, DSProProxyService.class);
            LOG.warn("Binding " + DSProProxyService.class.getSimpleName());
            return bindService(i, _connection, Context.BIND_AUTO_CREATE);
        }

        return false;
    }

    @Override
    public void onResume() {
        changedStatus("onResume()");
        loadConfig();
        setNodeUUID();
        setNetworkInterface();
        setUIBehavior(isDSProServiceRunning()); //restore UI state if the service is still running
        setIntentFilter();
        unbind();
        if (isDSProServiceRunning())
            bind();
        Intent intent = getIntent();
        LOG.info("onResume(): received intent with action: " + intent.getAction());
        String serviceAction = intent.getAction();
        LOG.info("onResume(): Extra service action is: " + serviceAction);
//        if (serviceAction != null && serviceAction.equalsIgnoreCase(getString(R.string.intent_restart_service))) {
//            restart(intent);
//        }
        if (serviceAction != null && serviceAction.equalsIgnoreCase(getString(R.string.intent_stop_service))) {
            exit();
        }
        super.onResume();
    }

    private void setIntentFilter() {
        if (!_intentsReceiverRegistered) {
            LOG.info("setIntentFilter(): Registering IntentReceiver");
            IntentFilter filter = new IntentFilter();
            filter.addAction(getString(R.string.intent_start_service));
            filter.addAction(getString(R.string.intent_stop_service));
            filter.addAction(getString(R.string.intent_restart_service));
            registerReceiver(_intentsReceiver, filter);
            _intentsReceiverRegistered = true;
        }
    }

    public void bind() {
        _isBound = bindDSPro2ProxyService();
    }

    public void unbind() {
        if (_isBound) {
            LOG.warn("unbind(): Unbinding " + DSProProxyService.class.getSimpleName());
            unbindService(_connection);
            _isBound = false;
        }
    }


    @Override
    public void onBackPressed() {
        LOG.debug("onBackPressed(): returning to the previous application");
        moveTaskToBack(true);
    }

    @Override
    public void onDestroy() {
        changedStatus("onDestroy()");
        unbind();
        if (_intentsReceiverRegistered) {
            LOG.info("onDestroy(): Unregistering IntentsReceiver");
            unregisterReceiver(_intentsReceiver);
            _intentsReceiverRegistered = false;
        }
        super.onDestroy();
    }

    @Override
    public void onPause() {
        changedStatus("onPause()");
        super.onPause();
    }

    @Override
    public void onStop() {
        changedStatus("onStop()");
        super.onStop();
    }

    public void startServices(boolean fromAnotherApp, String buildVersion) {
        LOG.info("startServices(): Starting DSProService");
        Intent i = new Intent(this, DSProService.class);
        i.putExtra(getString(R.string.intent_sender), fromAnotherApp);
        i.putExtra(getString(R.string.intent_build_version), buildVersion);
        startService(i);
        bindDSPro2ProxyService();
    }

    public void startMutexLoggerService() {
        LOG.info("startMutexLoggerService(): Starting MutexLoggerService");
        startService(new Intent(this, MutexLoggerService.class));
    }

    public void stopServices() {
        Intent i = new Intent(this, DSProService.class);
        stopService(i);
        setUIBehavior(false);
        LOG.info("stopServices(): Stopping DSProService from DSProActivity");
    }

    public void setUIBehavior(boolean isServiceRunning) {
        final Button btnStart = findViewById(R.id.btnStart);
        btnStart.setEnabled(!isServiceRunning);
        final ProgressBar progressBar = findViewById(R.id.progressBar);
        progressBar.setVisibility(isServiceRunning ? View.VISIBLE : View.INVISIBLE);
        //disable session key selection and toggle
        final EditText etSessionKey = findViewById(R.id.etSessionKey);
        etSessionKey.setEnabled(!isServiceRunning);
        final Spinner selectInterface = findViewById(R.id.spnSelectInterface);
        selectInterface.setEnabled(!isServiceRunning);
        final Spinner selectMode = findViewById(R.id.spnSelectMode);
        selectMode.setEnabled(!isServiceRunning);
    }

    public void changedStatus(String statusChanged) {
        LOG.info("Called " + statusChanged);
    }

    public boolean isDSProServiceRunning() {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (DSProService.class.getName().equals(service.service.getClassName())) {
                LOG.debug("isDSProServiceRunning(): Is DSProService running?: " + true);
                return true;
            }
        }
        LOG.debug("Is DSProService running?: " + false);
        return false;
    }

    /**
     * Class <code>ConfigureLog4J</code> configures the log4j logger for Android.
     */
    public static class ConfigureLog4J {
        public static void configure(Context context) {
            final LogConfigurator logConfigurator = new LogConfigurator();
            //logConfigurator.setFileName(Environment.getExternalStorageDirectory() + context.getString(R.string
            //        .app_folder) + context.getString(R.string.app_log4j_file));
            logConfigurator.setFileName(Environment.getExternalStorageDirectory().getPath() + context.getString(R.string
                    .app_folder) + context.getString(R.string.app_log4j_file));
            logConfigurator.setRootLevel(Level.DEBUG);
            logConfigurator.configure();
        }
    }

    public static void setListViewHeightBasedOnChildren(ListView listView) {
        ListAdapter listAdapter = listView.getAdapter();
        if (listAdapter == null)
            return;
        int desiredWidth = listView.getWidth();//View.MeasureSpec.makeMeasureSpec(listView.getWidth(), View.MeasureSpec.UNSPECIFIED);
        int totalHeight = 0;
        View view = null;
        for (int i = 0; i < listAdapter.getCount(); i++) {
            view = listAdapter.getView(i, view, listView);
            if (i == 0)
                view.setLayoutParams(new ViewGroup.LayoutParams(desiredWidth, android.support.v7.app.ActionBar.LayoutParams.WRAP_CONTENT));
            view.measure(desiredWidth, View.MeasureSpec.UNSPECIFIED);
            totalHeight += view.getMeasuredHeight();
        }
        //add one more
        totalHeight += view.getMeasuredHeight();
        ViewGroup.LayoutParams params = listView.getLayoutParams();
        params.height = totalHeight + (listView.getDividerHeight() * (listAdapter.getCount() - 1));
        //LOG.debug("Setting parameters for the peerList");
        listView.setLayoutParams(params);
    }


    /**
     * U N U S E D  C O D E
     */

    /**
     * Get the last build timestamp of the app.
     *
     * @return a <code>String</code> containing the last build timestamp of the app.
     */
    /*
    public String getLastBuildTimestamp() {
        try {
            ApplicationInfo ai = getPackageManager().getApplicationInfo(getPackageName(), 0);
            ZipFile zf = new ZipFile(ai.sourceDir);
            ZipEntry ze = zf.getEntry("classes.dex");
            long time = ze.getTime();
            return SimpleDateFormat.getInstance().format(new java.util.Date(time));
        } catch (Exception e) {
            LOG.error("getLastBuildTimestamp(): Unable to retrieve last build timestamp", e);
            return "Undefined";
        }
    }

    private void restart(Intent intent) {
        stopServices();
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
        intent.putExtra(getString(R.string.intent_service_action), getString(R.string.intent_start_service));
        finish();

        startActivity(intent);
        overridePendingTransition(0, 0);
    }

    /**
     * Thread to request the peers Update to DSProProxy Server
     * Keep this code and evaluate of using this Thread instead of the AsyncTask
     */
    /*
    Thread updatePeersThread = new Thread(new Runnable() {
        @Override
        public void run() {
            while (true) {
                if (_connection != null) {
                    try {
                        Message updatePeers = Message.obtain();
                        updatePeers.arg1 = MessageType.PEER_LIST.code();
                        updatePeers.obj = DSProActivity.class.getName();
                        updatePeers.replyTo = new Messenger(_activityHandler);
                        if (_messenger != null) {
                            LOG.debug("Sending peers request");
                            _messenger.send(updatePeers);
                        } else {
                            LOG.debug("messenger is null");
                        }
                        Thread.sleep(2000);
                    } catch (InterruptedException e) {
                        LOG.debug("InterruptedException in the peersUpdater Thread");
                    } catch (RemoteException e) {
                        LOG.debug("Impossible to send the message");
                        e.printStackTrace();
                    }
                }
            }
        }
    });
    */

}
