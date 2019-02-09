package us.ihmc.android.aci.dspro;

import android.os.Handler;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import org.apache.log4j.Logger;

import java.util.ArrayList;

/**
 * ActivityHandler.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ActivityHandler
{
    private final static Logger LOG = Logger.getLogger(ActivityHandler.class);

    public final static ArrayList<Messenger> activities = new ArrayList<Messenger>();
    public final ToActivity toActivity;
    public final FromActivity fromActivity;

    public ActivityHandler ()
    {
        toActivity = new ToActivity();
        fromActivity = new FromActivity();
    }

    public void clearActivities ()
    {
        activities.clear();
    }

    public class FromActivity extends Handler
    {
        @Override
        public void handleMessage (Message msg)
        {
            String msgSource = (String) msg.obj;
            int type = msg.arg1;
            LOG.debug("Received message of type: " + msg.arg1 + " from " + msgSource);

            switch (type) {
                case 0:
                    if (!activities.contains(msg.replyTo)) {
                        activities.add(msg.replyTo);
                        LOG.debug("Added " + msgSource + " to activities");
                    }
                    break;
            }
        }
    }

    public class ToActivity extends Handler
    {

        @Override
        public void handleMessage (Message msg)
        {
            Message wrap = null;

            if (activities.isEmpty()) {
                //LOG.warn("No activities found yet");
                //this.sendMessageDelayed(wrap, 500);
                return;
            }

            //LOG.debug("Activities list size: " + activities.size());
            for (Messenger msgr : activities) {
                try {
                    wrap = Message.obtain();
                    wrap.copyFrom(msg);
                    msgr.send(wrap);    //the AndroidRuntime does not support duplicated messages
                }
                catch (RemoteException e) {
                    LOG.warn("handleMessage(): Unable to send message to client, reprocessing message");
                    this.sendMessageDelayed(wrap, 500);
                }
            }
        }
    }


}
