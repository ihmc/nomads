package us.ihmc.android.aci.dspro.main;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import us.ihmc.android.aci.dspro.DSProActivity;
import us.ihmc.android.aci.dspro.MessageType;
import us.ihmc.android.aci.dspro.R;

/**
 * Created by kristyna on 5/3/18.
 */

public class BasicStatsFragment extends BaseStatsFragment {

    private TextView messagesPushed;
    private TextView bytesPushed;
    private TextView fragmentsPushed;
    private TextView fragmentBytesPushed;
    private TextView odFragmentsSent;
    private TextView odFragmentBytesSent;
    private TextView messagesReceived;
    private TextView bytesReceived;
    private TextView fragmentsReceived;
    private TextView fragmentBytesReceived;

    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = null;
        try {
            v = inflater.inflate(R.layout.tab_basic_stats, container, false);
        } catch (InflateException e) {
            e.getStackTrace();
        }

        if (v == null)
            return null;

        messagesPushed = v.findViewById(R.id.messagesPushed);
        bytesPushed = v.findViewById(R.id.bytesPushed);
        fragmentsPushed = v.findViewById(R.id.fragmentsPushed);
        fragmentBytesPushed = v.findViewById(R.id.fragmentBytesPushed);
        odFragmentsSent = v.findViewById(R.id.odFragmentsSent);
        odFragmentBytesSent = v.findViewById(R.id.odFragmentBytesSent);
        messagesReceived = v.findViewById(R.id.messagesReceived);
        bytesReceived = v.findViewById(R.id.bytesReceived);
        fragmentsReceived = v.findViewById(R.id.fragmentsReceived);
        fragmentBytesReceived = v.findViewById(R.id.fragmentBytesReceived);

        return v;
    }

    @SuppressLint("HandlerLeak")
    private Handler _activityHandler = new Handler()
    {
        @Override
        public void handleMessage (Message msg)
        {
            MessageType type = (MessageType) msg.obj;
            if (type == null) {
                return;
            }
            Bundle bundle = msg.getData();
            mActivity = (DSProActivity) getActivity();
            if (isAdded() && mActivity != null) {
                switch (type) {
                    case BASIC_STATS:
                        messagesReceived.setText(String.valueOf(bundle.getLong(getString(R.string.messagesReceived), 0)));
                        bytesReceived.setText(String.valueOf(bundle.getLong(getString(R.string.bytesReceived), 0)));
                        fragmentsReceived.setText(String.valueOf(bundle.getLong(getString(R.string.fragmentsReceived), 0)));
                        fragmentBytesReceived.setText(String.valueOf(bundle.getLong(getString(R.string.fragmentBytesReceived), 0)));

                        break;

                    case OVERALL_STATS:
                        messagesPushed.setText(String.valueOf(bundle.getLong(getString(R.string.messagesPushed), 0)));
                        bytesPushed.setText(String.valueOf(bundle.getLong(getString(R.string.bytesPushed), 0)));
                        fragmentsPushed.setText(String.valueOf(bundle.getLong(getString(R.string.fragmentsPushed), 0)));
                        fragmentBytesPushed.setText(String.valueOf(bundle.getLong(getString(R.string.fragmentBytesPushed), 0)));
                        odFragmentsSent.setText(String.valueOf(bundle.getLong(getString(R.string.odFragmentsSent), 0)));
                        odFragmentBytesSent.setText(String.valueOf(bundle.getLong(getString(R.string.odFragmentBytesSent), 0)));
                        break;
                }
            }
        }
    };

    @Override
    public Handler getActivityHandler() {
        return _activityHandler;
    }

    @Override
    public String getSimpleName() {
        return BasicStatsFragment.class.getSimpleName();
    }

}
