package us.ihmc.android.aci.dspro.main;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.support.v4.app.Fragment;

import org.apache.log4j.Logger;

import us.ihmc.android.aci.dspro.DSProActivity;
import us.ihmc.android.aci.dspro.DSProProxyService;
import us.ihmc.android.aci.dspro.MessageType;
import us.ihmc.android.aci.dspro.main.other.IHandler;
import us.ihmc.android.aci.dspro.main.other.MonitorService;

/**
 * Created by kristyna on 5/3/18.
 */

public abstract class BaseStatsFragment extends Fragment implements IHandler {

    protected DSProActivity mActivity;
    protected boolean mIsBound;
    protected Messenger mMessenger;
    protected final static Logger LOG = Logger.getLogger(BaseStatsFragment.class);

    /**
     * Defines callbacks for service binding, passed to bindService()
     */
    protected ServiceConnection _connection = new ServiceConnection()
    {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service)
        {
            //LOG.debug("onServiceConnected called");
            if (service == null) {
                LOG.warn("onServiceConnected(): Service connected but IBinder is null, unable to proceed");
                return;
            }

            String msgService = "Bound to " + DSProProxyService.class.getSimpleName();
            LOG.info(msgService);
            mIsBound = true;
            _service = service;

            //Do the handshake with Service in order to receive messages from it
            if (mMessenger == null) {
                mMessenger = new Messenger(service);
                Message handshake = Message.obtain();
                handshake.obj = getSimpleName();
                handshake.arg1 = MessageType.HANDSHAKE.code();
                handshake.replyTo = new Messenger(getActivityHandler());

                try {
                    mMessenger.send(handshake);
                }
                catch (RemoteException e) {
                    LOG.warn("onServiceConnected(): Unable to send initial handshake message to Service", e);
                }

            }
        }

        @Override
        public void onServiceDisconnected (ComponentName arg0)
        {
            LOG.debug("onServiceDisconnected(): Called onServiceDisconnected");
        }

        IBinder _service;
    };

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mActivity = (DSProActivity) context;
    }

    private boolean bind()
    {
        Intent i = new Intent(getActivity(), MonitorService.class);
        LOG.warn("bind(): Binding " + MonitorService.class.getSimpleName());
        return mActivity.bindService(i, _connection, Context.BIND_AUTO_CREATE);
    }

    public void unbind()
    {
        if (mIsBound) {
            LOG.warn("unbind(): Unbinding " + MonitorService.class.getSimpleName());
            mActivity.unbindService(_connection);
            mIsBound = false;
        } else {
            LOG.debug("unbind(): The service is not bounded");
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

    public void changedStatus (String statusChanged)
    {
        LOG.info("Called " + statusChanged);
    }

}
