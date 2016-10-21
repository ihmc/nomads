/*
 * DSProProxyService.jaa
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 *
 * Author: Enrico Casini           (ecasini@ihmc.us)
 */

package us.ihmc.android.aci.dspro;

import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import us.ihmc.aci.disServiceProProxy.util.NodePathParser;
import us.ihmc.aci.dspro2.AsyncDSProProxy;
import us.ihmc.aci.dspro2.DSProProxy;
import us.ihmc.aci.dspro2.DSProProxyListener;
import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.aci.util.dspro.XMLMetadataParser;
import us.ihmc.comm.CommException;

/**
 * DSProProxyService.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DSProProxyService extends Service {

    @Override
    public IBinder onBind(Intent intent) {

        hack();
        String dsproHost = intent.getAction();
        if (_asyncDsPro == null) {
            (new Thread(new ProxyRunnable(dsproHost, 3000), "ProxyRunnable")).start();
        }

        return _serviceMessenger.getBinder();
    }

    @Override
    public boolean onUnbind(Intent intent) {
        if (_activities != null)
            _activities.clear();

        return false;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "Received start id " + startId + ": " + intent);

        return START_NOT_STICKY;
    }

    Messenger _serviceMessenger = new Messenger(new Handler() {
        @Override
        public void handleMessage(Message msg) {

            String msgSource = (String) msg.obj;
            Log.d(TAG, "Received message of type: " + msg.arg1 + " from " + msgSource);

            if (msg.arg1 == -1) {
                if (!_activities.contains(msg.replyTo)) {
                    _activities.add(msg.replyTo);
                    Log.i(TAG, "Found HANDSHAKE. Added " + msgSource + " to activities");
                }
                return;
            }

            NodePath np = null;
            try {
                switch (msg.arg1) {
                    case 0:
                        np = NodePathParser.parse(getAssets().open(getString(R.string.routes_0)));
                        Log.d(TAG, "Read CompanyA correctly");
                        break;
                    case 1:
                        np = NodePathParser.parse(getAssets().open(getString(R.string.routes_1)));
                        Log.d(TAG, "Read CompanyB correctly");
                        break;
                    case 2:
                        np = NodePathParser.parse(getAssets().open(getString(R.string.routes_2)));
                        Log.d(TAG, "Read CompanyC correctly");
                        break;
                }

                if (np == null) {
                    Log.e(TAG, "NodePath is null, unable to continue");
                    return;
                }

                //update the position
                (new Thread(new PositionUpdater(np), "DSProPositionUpdater")).start();

            } catch (IOException e) {
                Log.e(TAG, "Unable to read NodePath file");
                return;
            }
        }
    });


    class PositionUpdater implements Runnable {

        public PositionUpdater(NodePath np) {
            _np = np;
        }

        @Override
        public void run() {
            String nodeId;
            try {

                if (_asyncDsPro == null) {
                    Log.d(TAG, "DSProProxy is null, unable to register path");
                    return;
                }

                Log.d(TAG, "Before registering path");
                _asyncDsPro.registerPath(_np);
                Log.d(TAG, "Registered path");
                //set the path to current
                _asyncDsPro.setCurrentPath(_np.getPathID());
                Log.d(TAG, "Set current path to: " + _np.getPathID());
                nodeId = _asyncDsPro.getNodeId();
                Log.d(TAG, "NodeId is: " + nodeId);
            } catch (CommException e) {
                e.printStackTrace();
                Log.e(TAG, "Unable to register path");
                return;
            }

            long defaultSleep = 3000;
            long sleepTime;
            int wayPointIndex = 0;

            do {
                sleepTime = 0;

                // Update Way Point
                if ((_np.getType() != NodePath.FIXED_LOCATION)) {
                    if (wayPointIndex < _np.getLength()) {
                        try {
                            _asyncDsPro.setCurrentPosition(_np.getLatitude(wayPointIndex),
                                    _np.getLongitude(wayPointIndex),
                                    _np.getAltitude(wayPointIndex),
                                    _np.getLocation(wayPointIndex),
                                    _np.getNote(wayPointIndex));

                            Log.d(TAG, "The current way point is: <" + wayPointIndex + ">");
                            if (wayPointIndex < (_np.getLength() - 1)) {
                                sleepTime = getTimeAtWayPoint(_np, wayPointIndex + 1) - getTimeAtWayPoint(_np,
                                        wayPointIndex);
                                if (sleepTime < 0) {
                                    Log.d(TAG, "***\n*** Sleep time = " + sleepTime + " and the index is " +
                                            wayPointIndex + "\n***");
                                    sleepTime = 30000;
                                }
                            }
                        } catch (Exception e) {
                            Log.e(TAG, e.getMessage());
                        }
                        wayPointIndex++;
                    } else {
                        //exit loop
                        break;
                    }
                }

                // Sleep
                Log.d(TAG, "***\n***Sleep time is <" + sleepTime + ">\n***");
                try {
                    TimeUnit.MILLISECONDS.sleep((sleepTime <= 0 ? defaultSleep : sleepTime));
                } catch (Exception e) {
                    Log.e(TAG, "");
                }
            }
            while (true);
            //ready to submit another path
        }


        private final NodePath _np;
    }

    private long getTimeAtWayPoint(NodePath path, int index) throws Exception {
        return (new GregorianCalendar(path.getTime(index, Calendar.YEAR),
                path.getTime(index, Calendar.MONTH),
                path.getTime(index, Calendar.DAY_OF_MONTH),
                path.getTime(index, Calendar.HOUR_OF_DAY),
                path.getTime(index, Calendar.MINUTE),
                path.getTime(index, Calendar.SECOND))
        ).getTimeInMillis();
    }

    Handler _activityHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            Message wrap = null;

            if (_activities.isEmpty()) {
                Log.w(TAG, "No activities found yet");
                //this.sendMessageDelayed(wrap, 500);
                return;
            }

            Log.d(TAG, "Activities list size: " + _activities.size());
            for (Messenger msgr : _activities) {
                try {
                    wrap = Message.obtain();
                    wrap.copyFrom(msg);
                    msgr.send(wrap);    //the AndroidRuntime does not support duplicated messages
                } catch (RemoteException e) {
                    Log.w(TAG, "Unable to send message to client, reprocessing message");
                    this.sendMessageDelayed(wrap, 500);
                }
            }

            //clear activities
            //_activities.clear();
        }
    };


    class ProxyRunnable implements Runnable, DSProProxyListener {
        private final String _dsproHost;
        private final int _delay;

        public ProxyRunnable(String dsProHost, int delay) {
            _dsproHost = dsProHost;
            _delay = delay;
        }

        public void run() {
            Log.i(TAG, "DSPro2 proxy thread started");
//            try {
//                Thread.sleep(_delay);
//            }
//            catch (InterruptedException e) {
//                Log.e(TAG, "Error ", e);
//            }

            boolean initialized = false;
            while (!initialized) {
                //initialize proxy
                _asyncDsPro = new AsyncDSProProxy((short) 0, "127.0.0.1", DSProProxy
                        .DIS_SVC_PROXY_SERVER_PORT_NUMBER);
                try {
                    if (_asyncDsPro.init() == 0) {
                        _asyncDsPro.registerDSProProxyListener(this);
                        (new Thread(_asyncDsPro)).start();
                        Log.i(TAG, "AsyncDSProProxy initialized correctly");
                        initialized = true;

                        Message msg = new Message();
                        msg.arg1 = MsgCode.INIT_SUCCESS; //successful init
                        _activityHandler.sendMessage(msg);
                    } else {
                        Log.w(TAG, "Error while initializing AsyncDSProProxy");
                        Thread.sleep(_delay);
                    }
                } catch (InterruptedException e) {
                    Log.e(TAG, "Error ", e);
                } catch (CommException e) {
                    Log.e(TAG, "Error ", e);
                }

            }
        }

        public boolean pathRegistered(NodePath path, String nodeId, String teamId, String mission) {
            Log.d(TAG, "pathRegistered for nodeId: " + nodeId);

            Message msg = new Message();
            msg.arg1 = MsgCode.PATH_REGISTERED; //successful init
            _activityHandler.sendMessage(msg);


            return false;
        }

        public boolean positionUpdated(float latitude, float longitude, float altitude, String nodeId) {
            Log.d(TAG, "positionUpdated " + latitude + ", " + longitude + " for nodeId: " + nodeId);

            Message msg = new Message();
            msg.arg1 = MsgCode.POSITION_UPDATED;

            _activityHandler.sendMessage(msg);


            return false;
        }

        public void newNeighbor(String peerID) {
            Log.d(TAG, "newNeighbor peerId: " + peerID);

            Message msg = new Message();
            msg.arg1 = MsgCode.NEW_NEIGHBOR;
            _activityHandler.sendMessage(msg);

        }

        public void deadNeighbor(String peerID) {
            Log.d(TAG, "deadNeighbor peerId: " + peerID);

            Message msg = new Message();
            msg.arg1 = MsgCode.DEAD_NEIGHBOR;
            _activityHandler.sendMessage(msg);
        }


        public boolean dataArrived(String id, String groupName, String objectId, String instanceId, String
                annotatedObjMsgId, String mimeType, byte[] data, short chunkNumber, short totChunksNumber, String
                                           queryId) {
            Log.d(TAG, "dataArrived for"
                    + "\n MessageId: " + id
                    + "\n objectId: " + objectId
                    + "\n mimeType: " + mimeType
                    + "\n chunkNumber: " + chunkNumber
                    + "\n totChunksNumber: " + totChunksNumber
                    + "\n annotatedObjMsgId: " + annotatedObjMsgId);


            Message msg = new Message();
            msg.arg1 = MsgCode.DATA_ARRIVED;
            Bundle b = new Bundle();
            _activityHandler.sendMessage(msg);


            //get file name
            return false;
        }

        public boolean metadataArrived(String id, String groupName, String referredDataObjectId, String
                referredDataInstanceId, String XMLMetadata, String referredDataId, String queryId)

        {
            Log.d(TAG, "metadataArrived for"
                    + "\n MessageId: " + id
                    + "\n objectId: " + referredDataId);

            Message msg = new Message();
            msg.arg1 = MsgCode.METADATA_ARRIVED;
            _activityHandler.sendMessage(msg);


            Map<String, Object> metadata = null;
            try {
                metadata = XMLMetadataParser.parse(XMLMetadata);

            } catch (Exception e) {
                Log.e(TAG, "Error while parsing XML metadata", e);
                return false;
            }

            if (metadata == null) {
                Log.e(TAG, "Metadata is null");
                return false;
            }

            Object mimeTypeObj = metadata.get(MetadataElement.Data_Format.toString());
            if (mimeTypeObj == null) {
                Log.e(TAG, "MetadataElement.Data_Format is NULL, wrong data from DSPro");
                return false;
            }

            Object fileNameObj = metadata.get(MetadataElement.Data_Content.toString());
            if (fileNameObj == null) {
                Log.e(TAG, "MetadataElement.Data_Content is NULL, wrong data from DSPro");
                return false;
            }
            Object latitudeObj = metadata.get(MetadataElement.Left_Upper_Latitude.toString());
            if (latitudeObj == null) {
                Log.e(TAG, "MetadataElement.Left_Upper_Latitude is NULL, wrong data from DSPro");
                return false;
            }
            Object nodeTypeObj = metadata.get(MetadataElement.Node_Type.toString());
            if (nodeTypeObj == null) {
                Log.e(TAG, "MetadataElement.Node_Type is NULL, wrong data from DSPro");
                return false;
            }
            Object longitudeObj = metadata.get(MetadataElement.Left_Upper_Longitude.toString());
            if (longitudeObj == null) {
                Log.e(TAG, "MetadataElement.Left_Upper_Longitude is NULL, wrong data from DSPro");
                return false;
            }
            Object sequenceObj = metadata.get(MetadataElement.Source_Time_Stamp.toString());
            if (sequenceObj == null) {
                Log.e(TAG, "MetadataElement.Source_Time_Stamp is NULL, wrong data from DSPro");
                return false;
            }

            String mimeType = String.valueOf(mimeTypeObj);
            String fileName = String.valueOf(fileNameObj);
            //save file name
            _metadataToFileName.put(referredDataId, fileName);
            double latitude = Double.parseDouble(latitudeObj.toString());
            double longitude = Double.parseDouble(longitudeObj.toString());
            String milcode = String.valueOf(nodeTypeObj);
            long seq = Long.valueOf(String.valueOf(sequenceObj));

            return false;

        }
    }

    public static void hack() {
        //***
        // Configure which implementation of StAX that will be used by the
        // application. This is a hack to use the same XML processor that
        // IHMC uses in their code.
        //***
        System.setProperty("us.ihmc.stream.XMLInputFactory",
                "us.ihmc.com.sun.xml.stream.ZephyrParserFactory");
        System.setProperty("us.ihmc.stream.XMLOutputFactory",
                "us.ihmc.com.sun.xml.stream.ZephyrWriterFactory");
        System.setProperty("us.ihmc.stream.XMLEventFactory",
                "us.ihmc.com.sun.xml.stream.events.ZephyrEventFactory");
    }


    private final static String TAG = "DSProProxyService";
    private Map<String, Long> _trackSequenceIDs = new HashMap<String, Long>();
    private Map<String, String> _metadataToFileName = new HashMap<String, String>();
    private AsyncDSProProxy _asyncDsPro;
    //private CotServiceRemote _remote;
    private static ArrayList<Messenger> _activities = new ArrayList<Messenger>();

}
