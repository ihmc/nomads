package us.ihmc.android.aci.dspro;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.*;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.os.*;
import android.preference.PreferenceManager;
import android.support.v4.app.NotificationCompat;
import android.widget.Toast;
import org.apache.log4j.Logger;
import us.ihmc.android.aci.dspro.util.AndroidUtil;

/**
 * DSProService.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DSProService extends Service
{
    private NotificationManager mNM;
    private static DSProService INSTANCE;
    private IntentsReceiver _intentsReceiver;
    private boolean _intentsReceiverRegistered = false;
    private final static int PROXY_SERVER_DEFAULT_PORT = 56487;
    private final static Logger LOG = Logger.getLogger(DSProService.class);

    class IntentsReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive (Context context, Intent intent)
        {
            final String action = intent.getAction();
            LOG.warn("++++++++ Received action: " + action);

            SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
            boolean isNetworkReloadEnabled = settings.getBoolean(getString(R.string.aci_dspro2app_networkReload_enable),
                    Boolean.valueOf(getString(R.string.aci_dspro2app_networkReload_enable_default)));

            if (action != null && action.equals(ConnectivityManager.CONNECTIVITY_ACTION) && isNetworkReloadEnabled) {
                reloadCommAdaptors(); //reload DSPro
                reloadTransmissionService(); //reload DisService
                LOG.warn("++++++++ Reloading TransmissionService and CommAdaptors");
            }
        }
    }

    @Override
    public void onCreate ()
    {
        mNM = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        // Display a notification about us starting.  We put an icon in the status bar.
    }

    @Override
    public int onStartCommand (Intent intent, int flags, int startId)
    {
        LOG.info("Received start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly stopped, so return sticky.

        boolean fromAnotherApp = intent.getBooleanExtra(getString(R.string.intent_sender), false);
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        boolean isServiceSticky = settings.getBoolean(getString(R.string.aci_dspro2app_autoRestartService),
                Boolean.valueOf(getString(R.string.aci_dspro2app_autoRestartService_default)));
        String selectMode = settings.getString(getString(R.string.aci_dspro2app_selectMode),
                getString(R.string.aci_dspro2app_selectMode_default));
        String buildVersion = intent.getStringExtra(getString(R.string.intent_build_version));

        if (INSTANCE == null) {
            LOG.info("Creating first INSTANCE of DSProService");
            LOG.info("Is service sticky? " + isServiceSticky);
            _intentsReceiver = new IntentsReceiver();
            if (!_intentsReceiverRegistered) {
                LOG.info("Registering IntentReceiver");
                IntentFilter filter = new IntentFilter();
                filter.addAction(getString(R.string.intent_start_service));
                filter.addAction(getString(R.string.intent_stop_service));
                filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
                //intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
                //intentFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
                //intentFilter.addAction(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
                registerReceiver(_intentsReceiver, filter);
                _intentsReceiverRegistered = true;
            }

            startTasks(fromAnotherApp, selectMode, buildVersion);
            INSTANCE = this;
        }

        return isServiceSticky ? START_STICKY : START_NOT_STICKY;
    }

    private void startTasks (boolean fromAnotherApp, String selectMode, String buildVersion)
    {
        LOG.info("Starting service tasks");
        Resources res = getResources();
        String[] modes = res.getStringArray(R.array.pref_select_mode_array_entries);


        //start the service in the foreground to avoid being killed by the ActivityManager
        //NotificationType notification = new NotificationType(android.R.drawable.sym_def_app_icon, "Service started",
        //        System.currentTimeMillis());
        Intent notificationIntent = new Intent(this, DSProActivity.class);
        notificationIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP |
                Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
        NotificationCompat.Builder  builder = new NotificationCompat.Builder (this);
        builder.setSmallIcon(android.R.drawable.sym_def_app_icon);
        builder.setTicker("Service started");
        builder.setContentIntent(pendingIntent);
        if (selectMode.equalsIgnoreCase(modes[0])) {
            builder.setContentTitle(getText(R.string.dspro2_notification_title));
            builder.setContentText(getText(R.string.dspro2_notification_message));
            /*notification.setLatestEventInfo(this, getText(R.string.dspro2_notification_title),
                    getText(R.string.dspro2_notification_message), pendingIntent);*/
        }
        else {
            builder.setContentTitle(getText(R.string.disservice_notification_title));
            builder.setContentText(getText(R.string.disservice_notification_message));
            /*notification.setLatestEventInfo(this, getText(R.string.disservice_notification_title),
                    getText(R.string.disservice_notification_message), pendingIntent);*/
        }
        builder.build();
        Notification notification = builder.getNotification();
        notification.flags |= Notification.FLAG_NO_CLEAR;
        startForeground(1, notification);

        if (selectMode.equalsIgnoreCase(modes[0])) {
            LOG.info("From another app is: " + fromAnotherApp);
            startDSProThread(buildVersion); //start DSPro2

            if (fromAnotherApp) {
                LOG.info("Broadcasting " + getString(R.string.intent_service_started));
                Intent i = new Intent(getString(R.string.intent_service_started));
                sendBroadcast(i);
            }
        }
        else {
            startDisServiceThread(buildVersion); //start DisService
        }

    }

    public void startDSProThread (final String buildVersion)
    {
        (new Thread(() -> {
            try {
                LOG.info("DSPro native application started");
                LOG.warn("Broadcasting " + getString(R.string.intent_service_started));
                Intent i = new Intent(getString(R.string.intent_service_started));
                sendBroadcast(i);
                //TODO This is only for this branch, remove it
                int rc = startDSPro(PROXY_SERVER_DEFAULT_PORT, AndroidUtil.getExternalStorageDirectory(),
                        buildVersion);
            }
            catch (Exception e) {
                LOG.warn("Broadcasting " + getString(R.string.intent_service_error));
                Intent i = new Intent(getString(R.string.intent_service_error));
                sendBroadcast(i);
                LOG.error(AndroidUtil.getStackTraceAsString(e));
            }
            LOG.error("DSPro native application thread run over");
        }, "startDSPro")).start();
    }

    public void startDisServiceThread (final String buildVersion)
    {
        (new Thread(() -> {
            try {
                LOG.info("DisService native application started");
                LOG.warn("Broadcasting " + getString(R.string.intent_service_started));
                Intent i = new Intent(getString(R.string.intent_service_started));
                sendBroadcast(i);
                int rc = startDisService(PROXY_SERVER_DEFAULT_PORT, AndroidUtil.getExternalStorageDirectory(),
                        buildVersion);
            }
            catch (Exception e) {
                LOG.error(AndroidUtil.getStackTraceAsString(e));
            }
            LOG.error("DisService native application has quit unexpectedly");
        }, "startDisService")).start();
    }

    @Override
    public void onDestroy ()
    {
        stopService();
    }

    private void stopService ()
    {
        // Cancel the persistent notification.
        if (_intentsReceiverRegistered) {
            LOG.info("Unregistering IntentsReceiver");
            unregisterReceiver(_intentsReceiver);
            _intentsReceiverRegistered = false;
        }

        int NOTIFICATION = R.string.local_service_started;
        mNM.cancel(NOTIFICATION);
        stopDSPro();
        stopForeground(true);
        stopSelf();
        INSTANCE = null;
        //call gc
        System.gc();
        LOG.warn(DSProService.class.getSimpleName() + " is being stopped!");
        LOG.warn("Broadcasting " + getString(R.string.intent_service_stopped));
        Intent i = new Intent(getString(R.string.intent_service_stopped));
        sendBroadcast(i);
        // Tell the user we stopped.
        Toast.makeText(this, R.string.local_service_stopped, Toast.LENGTH_SHORT).show();
    }

    @Override
    public IBinder onBind (Intent intent)
    {
        return null;
    }

    /*
    * A native method that is implemented by the DSPro2 native
    * libraries, which are packaged with this application.
    */
    public native int startDSPro (int ui16Port, String storageDir, String buildVersion);

    public native int startDisService (int ui16Port, String storageDir, String buildVersion);

    public native int reloadTransmissionService ();

    public native int reloadCommAdaptors ();

    public native int stopDSPro ();

    /*
     * This is used to load the 'disservicepro' libraries on
     * application startup.
     */

    static {
        System.loadLibrary("gnustl_shared");
        //System.loadLibrary("ihmcssl");
	    //System.loadLibrary("ihmccrypto");
        //System.loadLibrary("c++_shared");
        System.loadLibrary("voi");
        System.loadLibrary("dali");
        System.loadLibrary("util");
        System.loadLibrary("ihmcmedia");
        System.loadLibrary("chunking");
        System.loadLibrary("nms");
        System.loadLibrary("msgpack");
        System.loadLibrary("jpegdroid");
        System.loadLibrary("sqlite3droid");
        System.loadLibrary("tinyxpath");
        System.loadLibrary("c4.5");
        System.loadLibrary("lcppdc");
        System.loadLibrary("milstd2525");
        System.loadLibrary("disservice");
        System.loadLibrary("mockets");
        System.loadLibrary("nockets");
        System.loadLibrary("natswr");
        System.loadLibrary("cnats");
        //System.loadLibrary("protobuf");
        //System.loadLibrary("mil_navy_nrl_norm");
        System.loadLibrary("dspro2");
        System.loadLibrary("dsproandroid");
    }
}
