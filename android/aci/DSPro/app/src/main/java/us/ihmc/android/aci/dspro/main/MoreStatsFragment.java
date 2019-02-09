package us.ihmc.android.aci.dspro.main;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.content.res.ColorStateList;
import android.content.res.XmlResourceParser;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TableRow;
import android.widget.TextView;

import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import us.ihmc.android.aci.dspro.DSProActivity;
import us.ihmc.android.aci.dspro.MessageType;
import us.ihmc.android.aci.dspro.R;
import us.ihmc.android.aci.dspro.wear.NotificationType;
import us.ihmc.android.aci.dspro.wear.Notifier;

/**
 * Created by kristyna on 5/3/18.
 */

public class MoreStatsFragment extends BaseStatsFragment {

    //initial gaps to trigger notification of bad network
    long DT_COUNT_GAP_FOR_NOTIFICATION = 10;
    long MFR_COUNT_GAP_FOR_NOTIFICATION = 10;
    // For the peer statistic table
    private Map<String, Integer> mAlreadyRecordedPeers;
    private String lnSep = System.getProperty("line.separator");

    private TextView kamSent;
    private TextView kamReceived;
    private TextView mfrsSent;
    private TextView mfrsBytesSent;
    private TextView mfrsReceived;
    private TextView mfrsBytesReceived;
    private TextView dtRecNoTarget;
    private TextView dtRecTargetNode;

    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = null;
        try {
            v = inflater.inflate(R.layout.tab_more_stats, container, false);
        } catch (InflateException e) {
            e.getStackTrace();
        }

        if (v == null)
            return null;

        mAlreadyRecordedPeers = new HashMap<>();

        kamSent = v.findViewById(R.id.kamSent);
        kamReceived = v.findViewById(R.id.kamReceived);
        mfrsSent = v.findViewById(R.id.mfrsSent);
        mfrsBytesSent = v.findViewById(R.id.mfrsBytesSent);
        mfrsReceived = v.findViewById(R.id.mfrsReceived);
        mfrsBytesReceived = v.findViewById(R.id.mfrsBytesReceived);
        dtRecNoTarget = v.findViewById(R.id.dtRecNoTarget);
        dtRecTargetNode = v.findViewById(R.id.dtRecTargetNode);

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
                        kamSent.setText(String.valueOf(bundle.getLong(getString(R.string.kamSent), 0)));
                        kamReceived.setText(String.valueOf(bundle.getLong(getString(R.string.kamReceived), 0)));


                        long mfrS = bundle.getLong(getString(R.string.mfrsSent), 0);
                        long mfrBS = bundle.getLong(getString(R.string.mfrsBytesSent), 0);
                        long mfrR = bundle.getLong(getString(R.string.mfrsReceived), 0);
                        long mfrBR = bundle.getLong(getString(R.string.mfrsBytesReceived), 0);

                        long mfrCount = mfrS + mfrR;
                        if (mfrCount > MFR_COUNT_GAP_FOR_NOTIFICATION) {
                            Notifier.INSTANCE.notify(mActivity.getApplicationContext(),
                                    NotificationType.NETWORK_STATUS_BAD.code(),
                                    "Bad network, high value of Missing Fragment Requests",
                                    "Missing Fragment Requests have reached " + mfrCount);
                            MFR_COUNT_GAP_FOR_NOTIFICATION = mfrCount * 2;
                        }

                        mfrsSent.setText(String.valueOf(mfrS));
                        mfrsBytesSent.setText(String.valueOf(mfrBS));
                        mfrsReceived.setText(String.valueOf(mfrR));
                        mfrsBytesReceived.setText(String.valueOf(mfrBR));
                        break;

                    case DUPLICATE_TRAFFIC_STATS:

                        long dtRNT = bundle.getLong(getString(R.string.dtRecNoTarget), 0);
                        long dtRTN = bundle.getLong(getString(R.string.dtRecTargetNode), 0);

                        long dtCount = dtRNT + dtRTN;
                        if (dtCount > DT_COUNT_GAP_FOR_NOTIFICATION) {
                            Notifier.INSTANCE.notify(mActivity.getApplicationContext(),
                                    NotificationType.NETWORK_STATUS_BAD.code(),
                                    "Bad network, high value of Duplicated Traffic",
                                    "Duplicated Traffic has reached " + dtCount);
                            DT_COUNT_GAP_FOR_NOTIFICATION = dtCount * 2;
                        }

                        dtRecNoTarget.setText(String.valueOf(dtRNT));
                        dtRecTargetNode.setText(String.valueOf(dtRTN));
                        break;

                    case PEER_GROUP_STATS:

                        String peerId = bundle.getString(getString(R.string.peerId));
                        String kamRec = String.valueOf(bundle.getLong(getString(R.string.kamReceived), 0));
                        String mfrmRec = String.valueOf(bundle.getLong(getString(R.string.mfrsReceived), 0));
                        String mfrbRec = String.valueOf(bundle.getLong(getString(R.string.mfrsBytesReceived), 0));
                        String dmRec = String.valueOf(bundle.getLong(getString(R.string.messagesReceived), 0));
                        String dbRec = String.valueOf(bundle.getLong(getString(R.string.bytesReceived), 0));
                        String dfRec = String.valueOf(bundle.getLong(getString(R.string.fragmentsReceived), 0));
                        String dfbRec = String.valueOf(bundle.getLong(getString(R.string.fragmentBytesReceived), 0));

                        if (!mAlreadyRecordedPeers.containsKey(peerId)) {
                            mAlreadyRecordedPeers.put(peerId, mAlreadyRecordedPeers.size());
                            addRowSummaryTable(peerId, kamRec, mfrmRec, mfrbRec, dmRec, dbRec, dfRec, dfbRec);
                        } else {
                            updateRowSummaryTable(peerId, kamRec, mfrmRec, mfrbRec, dmRec, dbRec, dfRec, dfbRec);
                        }
                        break;
                }
            }
        }
    };

    // Method variable names make more sense in context of peerGroupStatisticInfoUpdateArrived
    protected void addRowSummaryTable (final String peerId, final String kamRec, final String mfrmRec,
                                       final String mfrbRec, final String dmRec, final String dbRec,
                                       final String dfRec, final String dfbRec)
    {
        if (mActivity != null) {
            TableRow summaryTableRow = new TableRow(mActivity.getApplicationContext());
            TextView peerIdTv = new TextView(mActivity.getApplicationContext());

            // TODO what is that? why is color being passed?
            XmlResourceParser parser = getResources().getXml(R.color.label);
            peerIdTv.setTextSize(20);
            try {
                peerIdTv.setTextColor(ColorStateList.createFromXml(getResources(), parser));
            } catch (XmlPullParserException | IOException e) {
                e.printStackTrace();
            }

        }
    }

    protected void updateRowSummaryTable (final String peerId, final String kamRec, final String mfrmRec,
                                          final String mfrbRec, final String dmRec, final String dbRec,
                                          final String dfRec, final String dfbRec)
    {
        if (mActivity != null) {
            TableRow summaryTableRow = new TableRow(mActivity.getApplicationContext());
            summaryTableRow.setId(mAlreadyRecordedPeers.get(peerId));

            View.OnClickListener dialogListener = getOnClickListener(peerId, kamRec, mfrmRec, mfrbRec, dmRec, dbRec, dfRec, dfbRec);
            summaryTableRow.setOnClickListener(dialogListener);
        }
    }

    private View.OnClickListener getOnClickListener (final String peerId, final String kamRec, final String mfrmRec,
                                                     final String mfrbRec, final String dmRec, final String dbRec,
                                                     final String dfRec, final String dfbRec)
    {
        return v -> {
            if (mActivity != null) {
                AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(mActivity.getApplicationContext());
                dialogBuilder.setTitle(peerId);
                dialogBuilder.setMessage("KAM Rec: " + kamRec + lnSep +
                        "MFRM Rec: " + mfrmRec + lnSep +
                        "MFRB Rec: " + mfrbRec + lnSep +
                        "DM Rec: " + dmRec + lnSep +
                        "DB Rec: " + dbRec + lnSep +
                        "DF Rec: " + dfRec + lnSep +
                        "DFB Rec: " + dfbRec + lnSep)
                        .setCancelable(true)
                        .setNeutralButton("Close", (dialog, which) -> dialog.cancel());

                dialogBuilder.show();
            }
        };
    }

    @Override
    public Handler getActivityHandler() {
        return _activityHandler;
    }

    @Override
    public String getSimpleName() {
        return MoreStatsFragment.class.getSimpleName();
    }

}
