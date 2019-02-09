package us.ihmc.android.aci.dspro;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Environment;
import android.os.IBinder;
import android.widget.Toast;
import org.apache.log4j.Logger;
import us.ihmc.android.aci.dspro.util.AndroidUtil;
import us.ihmc.netutils.MutexLogger;

import java.io.FileNotFoundException;

/**
 * MutexLoggerService.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class MutexLoggerService extends Service
{
    private NotificationManager mNM;
    private final static Logger LOG = Logger.getLogger(MutexLoggerService.class);

    @Override
    public void onCreate ()
    {
        //create a new Android Util
        mNM = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
    }

    @Override
    public int onStartCommand (Intent intent, int flags, int startId)
    {
        LOG.info("Received start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        startTasks();
        return START_NOT_STICKY;
    }

    private void startTasks ()
    {
        //start the service in the foreground to avoid being killed by the ActivityManager
        Notification notification = new Notification(android.R.drawable.sym_def_app_icon, "Service started",
                System.currentTimeMillis());
        Intent notificationIntent = new Intent(this, DSProActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
        /*notification.setLatestEventInfo(this, getText(R.string.mutex_notification_title),
                getText(R.string.mutex_notification_message), pendingIntent);*
        startForeground(2, notification);*/
        startMutexLogger();
    }

    private void startMutexLogger ()
    {
        String sdCardPath = AndroidUtil.getExternalStorageDirectory() + "/";
        try {
            MutexLogger mutexLogger = new MutexLogger(sdCardPath + getString(R.string.mutex_raw_file),
                    Environment.getExternalStorageDirectory() + getString(R.string
                            .app_folder) + getString(R.string.mutex_log_file));
            LOG.info("Mutex raw file initialization successful");
            //start MutexLogger
            mutexLogger.start();
        }
        catch (FileNotFoundException e) {
            e.printStackTrace();
            LOG.error("Unable to read Mutex raw file");
        }
    }


    public IBinder onBind (Intent intent)
    {
        return null;
    }

    @Override
    public void onDestroy ()
    {
        // Cancel the persistent notification.
        int NOTIFICATION = R.string.mutex_service_started;
        mNM.cancel(NOTIFICATION);

        //call gc
        System.gc();

        // Tell the user we stopped.
        Toast.makeText(this, R.string.mutex_service_stopped, Toast.LENGTH_SHORT).show();
    }
}
