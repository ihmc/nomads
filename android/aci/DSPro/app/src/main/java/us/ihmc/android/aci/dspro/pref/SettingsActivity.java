package us.ihmc.android.aci.dspro.pref;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.*;
import android.os.*;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;

import org.apache.log4j.Logger;

import java.util.Properties;

import us.ihmc.android.aci.dspro.DSProProxyService;
import us.ihmc.android.aci.dspro.MessageType;
import us.ihmc.android.aci.dspro.R;
import us.ihmc.android.aci.dspro.util.AndroidUtil;


/**
 * SettingsActivity.java
 * <p/>
 * Class <code>SettingsActivity</code> extends an Android Activity and handles the configurations settings of the app.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class SettingsActivity extends AppCompatActivity
{
    Messenger _messenger;
    private boolean _isBound;
    private final static Logger LOG = Logger.getLogger(SettingsActivity.class);

    @Override
    protected void onCreate (Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        /**
         * Setting the color of the actionBar
         */
        String configFilePath = AndroidUtil.getExternalStorageDirectory() +
                getString(R.string.app_folder) + getString(R.string.app_config_file);
        Properties config = AndroidUtil.readConfigFile(configFilePath);
        String encryptionMode = config.getProperty(getString(R.string.aci_disService_networkMessageService_encryptionMode));
        AndroidUtil.changeActionBarColor(this, encryptionMode);
        // Display the fragment as the main content.
        getFragmentManager().beginTransaction().replace(android.R.id.content, new SettingsFragment()).commit();
    }

    private boolean isRuntimeChanges ()
    {
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        boolean isRuntimeChanges = settings.getBoolean(getString(R.string.aci_dspro2app_runtime_changes),
                Boolean.valueOf(getString(R.string.aci_dspro2app_runtime_changes_default)));
        LOG.debug("Is runtime changes enabled? " + isRuntimeChanges);
        return isRuntimeChanges;
    }

    @Override
    public void onPause ()
    {
        changedStatus("onPause()");
        //bind to DSProProxyService if runtime changes are enabled and service is running
        if (isRuntimeChanges() && isDSPro2ProxyServiceRunning())
            bind();
        super.onResume();
    }


    @Override
    public void onResume ()
    {
        changedStatus("onResume()");
        super.onResume();
    }

    @Override
    public void onDestroy ()
    {
        changedStatus("onDestroy()");
        unbind();
        super.onDestroy();
    }

    /**
     * Defines callbacks for service binding, passed to bindService()
     */
    private ServiceConnection _connection = new ServiceConnection()
    {
        @Override
        public void onServiceConnected (ComponentName className,
                                        IBinder service)
        {
            if (service == null) {
                LOG.warn("Service connected but IBinder is null, unable to proceed");
                return;
            }

            String msgService = "Bound to " + DSProProxyService.class.getSimpleName();
            LOG.info(msgService);
            _isBound = true;
            _service = service;

            if (_messenger == null) {
                _messenger = new Messenger(service);
                Message configure = Message.obtain();
                configure.obj = SettingsActivity.class.getSimpleName();
                configure.arg1 = MessageType.CONFIGURE.code();
                configure.replyTo = new Messenger(_activityHandler);

                try {
                    _messenger.send(configure);
                }
                catch (RemoteException e) {
                    LOG.warn("Unable to send initial configure message to Service", e);
                }

            }
        }

        @Override
        public void onServiceDisconnected (ComponentName arg0)
        {
            LOG.debug("Called onServiceDisconnected");
        }

        IBinder _service;
    };

    private Handler _activityHandler = new Handler()
    {
        @Override
        public void handleMessage (Message msg)
        {
            //do nothing
        }
    };

    private void bind ()
    {
        LOG.info("Binding to " + DSProProxyService.class.getSimpleName());
        Intent i = new Intent(this, DSProProxyService.class);
        bindService(i, _connection, Context.BIND_AUTO_CREATE);
    }

    public void unbind ()
    {
        if (_isBound) {
            LOG.warn("Unbinding " + DSProProxyService.class.getSimpleName());
            unbindService(_connection);
            _isBound = false;
        }
    }

    private boolean isDSPro2ProxyServiceRunning ()
    {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (DSProProxyService.class.getName().equals(service.service.getClassName())) {
                LOG.debug("Is DSProProxyService running?: " + true);
                return true;
            }
        }
        LOG.debug("Is DSProProxyService running?: " + false);
        return false;
    }

    public void changedStatus (String statusChanged)
    {
        //log the status change
        LOG.info("Called " + statusChanged);
    }
}
