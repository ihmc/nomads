package us.ihmc.android.aci.dspro;

import android.annotation.SuppressLint;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.*;

import com.google.gson.JsonSyntaxException;

import org.apache.log4j.Logger;
import org.json.JSONException;
import org.json.JSONObject;

import us.ihmc.aci.dspro2.AsyncDSProProxy;
import us.ihmc.aci.dspro2.DSProProxyBuilder;
import us.ihmc.aci.dspro2.DSProProxyListener;
import us.ihmc.aci.dspro2.QueuedDSProProxy;
import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.aci.util.dspro.soi.ApplicationMetadataFormat;
import us.ihmc.android.aci.dspro.main.PeersFragment;
import us.ihmc.android.aci.dspro.util.AndroidUtil;
import us.ihmc.android.aci.dspro.util.SortedProperties;
import us.ihmc.android.aci.dspro.wear.NotificationType;
import us.ihmc.android.aci.dspro.wear.Notifier;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

/**
 * DSProProxyService.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DSProProxyService extends Service
{
    @Override
    public IBinder onBind (Intent intent)
    {
        changedStatus("onBind()");

        //(new Thread(_toActivity, "ActivityMessenger")).start();
        if (_asyncDSProProxy == null) {
            LOG.info("Instantiating a new DSProProxy");
            (new Thread(new DSProProxy(this, 3000), "DSProProxy")).start();
        }

        return _serviceMessenger.getBinder();
    }

    @Override
    public boolean onUnbind (Intent intent)
    {
        changedStatus("onUnbind()");
        if (_activities != null)
            _activities.clear();

        return false;
    }

    @Override
    public int onStartCommand (Intent intent, int flags, int startId)
    {
        LOG.info("onStartCommand(): Received start id " + startId + ": " + intent);
        return START_NOT_STICKY;
    }

    @SuppressLint("HandlerLeak")
    Messenger _serviceMessenger = new Messenger(new Handler()
    {
        @Override
        public void handleMessage (Message msg)
        {
            String msgSource = (String) msg.obj;
            int type = msg.arg1;
            //LOG.debug("Received message of type: " + msg.arg1 + " from " + msgSource);
            switch (type) {
                case 0:
                    if (!_activities.contains(msg.replyTo)) {
                        _activities.add(msg.replyTo);
                        if (msgSource.equals(PeersFragment.class.getSimpleName())) {
                            if (!checkDSProProxy()) return;
                            new PeerListTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                        }
                    }
                    break;
                case 1:
                    if (!_activities.contains(msg.replyTo)) {
                        _activities.add(msg.replyTo);
                    }
                    if (!checkDSProProxy()) {
                        return;
                    }
                    new PeerListTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                    break;
                case 2:
                    if (!checkDSProProxy()) return;
                    new ConfigureTask().execute("ConfigureTask");
                    break;
            }
        }
    });

    private boolean checkDSProProxy ()
    {
        if (_asyncDSProProxy == null) {
            LOG.info("checkDSProProxy(): Instantiating a new DSProProxy");
            (new Thread(new DSProProxy(this, 0), "DSProProxy")).start();
        }
        return true;
    }

    Handler _activityHandler = new Handler()
    {
        @Override
        public void handleMessage (Message msg)
        {
            Message wrap = null;

            if (_activities.isEmpty()) {
                LOG.warn("No activities found yet");
                //this.sendMessageDelayed(wrap, 500);
                return;
            }
            for (Messenger msgr : _activities) {
                try {
                    wrap = Message.obtain();
                    wrap.copyFrom(msg);
                    msgr.send(wrap);    //the AndroidRuntime does not support duplicated messages
                }
                catch (RemoteException e) {
                    LOG.warn("Unable to send message to client, reprocessing message");
                    this.sendMessageDelayed(wrap, 500);
                }
            }
            //clear activities
            //_activities.clear();
        }
    };


    class PeerListTask extends AsyncTask<String, Void, Message>
    {

        @Override
        protected Message doInBackground (String... params)
        {
            Message msg = Message.obtain();
            msg.obj = DSProProxyService.class.getSimpleName();
            msg.arg1 = MessageType.PEER_LIST_REFRESH.code();
            Bundle bundle = new Bundle();
            if (_asyncDSProProxy != null && _asyncDSProProxy.isInitialized()) {
                try {
                	if (_asyncDSProProxy.getPeerList() != null) {
                	    bundle.putStringArrayList(getString(R.string.peers), new ArrayList<>(_asyncDSProProxy.getPeerList()));
                	} else {
                	    bundle.putStringArrayList(getString(R.string.peers), new ArrayList<>());
                	}
		    } catch (Exception e) {
                	LOG.error("PeerListTask::doInBackground():" +
                        	"CommException  ", e);
		    }
            } else {
                bundle.putStringArrayList(getString(R.string.peers), new ArrayList<>());
            }
            msg.setData(bundle);
            return msg;
        }

        protected void onPostExecute (Message msg)
        {
            _activityHandler.sendMessage(msg);
        }
    }

    class ResetSessionTask extends AsyncTask<String, Void, Message>
    {

        @Override
        protected Message doInBackground (String... params)
        {
            Message msg = Message.obtain();
            String sessionKey = params[0];
            msg.obj = DSProProxyService.class.getSimpleName();
            msg.arg1 = MessageType.RESET_SESSION_INFO.code();
            Bundle bundle = new Bundle();
            bundle.putString(getString(R.string.session_key), sessionKey);
            msg.setData(bundle);
            return msg;
        }

        protected void onPostExecute (Message msg)
        {
            _activityHandler.sendMessage(msg);
        }
    }

    class ConfigureTask extends AsyncTask<String, Void, Boolean>
    {
        @Override
        protected Boolean doInBackground (String... params)
        {
            String configFilePath = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) +
                    getString(R.string.app_config_file);
            SortedProperties config = AndroidUtil.readConfigFile(configFilePath);
            if (config == null) {
                LOG.error("ConfigureTask::doInBackground(): Unable to read configuration file on path:" + configFilePath);
                return false;
            }
            try {
                if (_asyncDSProProxy.configureProperties(config))
                    LOG.info("ConfigureTask::doInBackground(): Runtime update " +
                            "of configuration properties through DSProProxy successful");
                else
                    LOG.warn("ConfigureTask::doInBackground(): Unable " +
                            "to update configuration properties at runtime");
            }
            catch (CommException e) {
                LOG.error("ConfigureTask::doInBackground():" +
                        "Unable to communicate with the DSProProxy", e);
                return false;
            }
            return true;
        }
    }

    class DSProProxy implements Runnable, DSProProxyListener
    {
        private final int _delay;
        private final Context _context;

        DSProProxy(Context context, int delay)
        {
            _context = context;
            _delay = delay;
        }

        public void run ()
        {
            LOG.info("DSProProxy::run(): DSPro2 proxy thread started");
            try {
                Thread.sleep(_delay);
            }
            catch (InterruptedException e) {
                LOG.error(AndroidUtil.getStackTraceAsString(e));
            }

            boolean initialized = false;
            while (!initialized) {
                //initialize proxy
                us.ihmc.aci.dspro2.DSProProxy dspro = new DSProProxyBuilder((short)0)
                        .setPort(us.ihmc.aci.dspro2.DSProProxy.DIS_SVC_PROXY_SERVER_PORT_NUMBER)
                        .setHost(getString(R.string.localhost))
                        .build();

                _asyncDSProProxy = new AsyncDSProProxy(new QueuedDSProProxy(dspro));

                if (_asyncDSProProxy.init() == 0) {

                    (new Thread(_asyncDSProProxy)).start();
                    LOG.info("AsyncDSProProxy initialized correctly");
                    initialized = true;

                    try {
                        _asyncDSProProxy.registerDSProProxyListener(this);
                    }
                    catch (CommException e) {
                        e.printStackTrace();
                    }
                }
                else {
                    LOG.warn("DSProProxy::run(): Error while initializing AsyncDSProProxy");
                    try {
                        Thread.sleep(3000);
                    }
                    catch (InterruptedException e) {
                        LOG.error(AndroidUtil.getStackTraceAsString(e));
                    }
                }
            }
        }

        @Override
        public boolean pathRegistered (NodePath path, String nodeId, String teamId, String mission)
        {
            return false;
        }

        @Override
        public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId)
        {
            //do not log the position updates
            //LOG.debug("Called POSITION_UPDATED with " + latitude + ", " + longitude + " and peerId " + nodeId);
            Notifier.INSTANCE.notify(_context, NotificationType.POSITION_UPDATED.code(), "Position update",
                    "Peer " + nodeId + " moved to " + latitude + ", " + longitude);
            return false;
        }


        @Override
        public void newNeighbor (String peerID)
        {
            //LOG.debug("Called NEW_NEIGHBOR with peerId " + peerID);
            Notifier.INSTANCE.notify(_context, NotificationType.NEW_NEIGHBOR.code(), "New Peer", peerID);
        }

        @Override
        public void deadNeighbor (String peerID)
        {
            //LOG.debug("Called DEAD_NEIGHBOR with peerId " + peerID);
            Notifier.INSTANCE.notify(_context, NotificationType.DEAD_NEIGHBOR.code(), "Dead Peer", peerID);
        }


        /**
         * Callback function that is invoked when new data arrives.
         *
         * @param dsproId - id of the arrived metadata message.
         * @param groupName - the name of the group the metadata message belongs to.
         * @param objectId
         * @param instanceId
         * @param annotatedObjMsgId
         * @param mimeType
         * @param data - the data that was received.
         * @param chunkNumber - the number of chunks.
         * @param totChunksNumber - the total number of chunks
         * @param callbackParameters
         * @return
         */
        public boolean dataArrived (String dsproId, String groupName, String objectId, String instanceId, String annotatedObjMsgId, String mimeType, byte[] data, short chunkNumber, short totChunksNumber, String callbackParameters) {
            return false;
        }

        /**
         * Callback function that is invoked when new metadata arrives.
         *
         * @param id             - id of the arrived metadata message.
         * @param jsonMetadata   - the json metadata.
         * @param referredDataId - referred data id of the message.
         * @return
         * @groupName - the name of the group the metadata message belongs to.
         */
        public boolean metadataArrived (String id, String groupName, String referredDataObjectId, String referredDataInstanceId, String jsonMetadata, String referredDataId, String queryId)
        {
            LOG.debug("DSProProxy::metadataArrived(): Called metadataArrived, id is " + id);

            Map<String, String> metadata;
            try {
                metadata = AndroidUtil.jsonToKeyValueMap(jsonMetadata);
            }
            catch (JsonSyntaxException e) {
               LOG.warn("DSProProxy::metadataArrived(): Unable to parse this metadata");
               return false;
            }

            if (metadata == null){
                LOG.debug("DSProProxy::metadataArrived(): metadata null");
                return false;
            }
            LOG.debug("DSProProxy::metadataArrived(): metadata is " + metadata.toString());

            String mimeType = metadata.get(MetadataElement.dataFormat.toString());
            if (mimeType == null) {
                LOG.error("DSProProxy::metadataArrived(): MetadataElement.Data_Format is null, wrong data from DSPro");
                return false;
            }

            LOG.debug("DSProProxy::metadataArrived(): mimeType " + mimeType);

            //if mimetype is not equal to reset then exit
            if (!mimeType.equals("x-dspro/x-soi-reset"))
            {
                return false;
            }

            String appMetadata = metadata.get(MetadataElement.applicationMetadata.toString());
            String appMetadataFormat = metadata.get(MetadataElement.applicationMetadataFormat.toString());
            byte[] appMetadataBuff = null;
            if (appMetadata != null) {
                if (appMetadataFormat == null) {
                    return false;
                }
                if (appMetadataFormat.equals(ApplicationMetadataFormat.Linguafranca_Base64.toString())
                        || appMetadataFormat.equals(ApplicationMetadataFormat.Phoenix_Base64.toString())
                        || appMetadataFormat.equals(ApplicationMetadataFormat.Soigen2_Json_Base64.toString())) {
                    //decode base 64
                    try {
                        appMetadataBuff = android.util.Base64.decode(appMetadata, android.util.Base64.DEFAULT);
                    }catch (Exception e) {
                        LOG.debug("DSProProxy::metadataArrived: exception in decoding appMetadataFormat");
                        return false;
                    }
                } else {
                    appMetadataBuff = appMetadata.getBytes();
                }
                JSONObject resetMessage = null;
                try {
                    resetMessage = new JSONObject(new String(appMetadataBuff));
                    LOG.debug("DSProProxy::metadatataArrived received reset message: " + resetMessage.toString());
                    if (resetMessage != null) {
                        if (resetMessage.has("step")) {
                            String stepString = resetMessage.getString("step");
                            if (stepString == null) {
                                return false;
                            }
                            if (stepString.equals("change_session_key") || stepString.equals("start")
                                    || stepString.equals("reset")) {
                                if (resetMessage.has("sessionKey")) {
                                    String sessionKey = resetMessage.getString("sessionKey");
                                    LOG.debug("DSProProxy::metadatataArrived retrieved sessionKey " + sessionKey);
                                    if (sessionKey != null) {
                                        resetSession(sessionKey);
                                    }
                                }
                            }
                        }
                    }
                } catch (JSONException e) {
                    LOG.warn("DSProProxy::metadataArrived");
                }
            }

            return false;
        }

        @Override
        public boolean dataAvailable(String s, String s1, String s2, String s3, String s4, String s5, byte[] bytes, String s6) {
            return false;
        }

    }

    public void changedStatus (String statusChanged)
    {
        //log the status change
        LOG.info("changedStatus(): Called " + statusChanged);
    }

    public void stopServices() {
        Intent i = new Intent(this, DSProService.class);
        stopService(i);
        LOG.info("stopServices(): Stopping DSProService from DSProActivity");
    }

    public void startServices(boolean fromAnotherApp, String buildVersion) {
        LOG.info("startServices(): Starting DSProService");
        Intent i = new Intent(this, DSProService.class);
        i.putExtra(getString(R.string.intent_sender), fromAnotherApp);
        i.putExtra(getString(R.string.intent_build_version), buildVersion);
        startService(i);
    }

    public void restartDSPro(String sessionKey)
    {
        String configFilePath = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) +
                getString(R.string.app_config_file);
        SortedProperties config = AndroidUtil.readConfigFile(configFilePath);
        config.put("aci.dspro.sessionKey", sessionKey);
        AndroidUtil.writeConfigFile(config, configFilePath);
        stopServices();
        new Timer().schedule(new TimerTask() {
            @Override
            public void run() {
                LOG.debug("DSProProxy::metadataArrived: sending start to DSPro");
                String buildVersion = SimpleDateFormat.getInstance().format(new Date(BuildConfig.TIMESTAMP));
                Intent i = new Intent("us.ihmc.android.aci.dspro");
                i.setAction("us.ihmc.android.aci.dspro.START_SERVICE");
                i.putExtra("us.ihmc.android.aci.dspro.SESSION_KEY", sessionKey);
                sendBroadcast(i);
                //startServices(false, buildVersion);
            }
        }, 3000);

    }

    public void resetSession(String sessionKey)
    {
        String configFilePath = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) +
                getString(R.string.app_config_file);
        SortedProperties config = AndroidUtil.readConfigFile(configFilePath);
        config.put("aci.dspro.sessionKey", sessionKey);
        AndroidUtil.writeConfigFile(config, configFilePath);
        Message msg = Message.obtain();
        LOG.debug("resetSession: Before sending reset message");
        msg.obj = DSProProxyService.class.getSimpleName();
        msg.arg1 = MessageType.RESET_SESSION_INFO.code();
        Bundle bundle = new Bundle();
        bundle.putString(getString(R.string.session_key), sessionKey);
        msg.setData(bundle);
        boolean sent = _activityHandler.sendMessage(msg);
        LOG.debug("resetSession: After sending reset message sent: " + sent);
        //new ResetSessionTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sessionKey);
    }

    private AsyncDSProProxy _asyncDSProProxy;
    //private final ActivityMessenger _toActivity = new ActivityMessenger();
    private static ArrayList<Messenger> _activities = new ArrayList<Messenger>();
    private final static Logger LOG = Logger.getLogger(DSProProxyService.class);
}
