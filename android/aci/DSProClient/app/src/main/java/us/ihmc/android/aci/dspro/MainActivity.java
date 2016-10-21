/*
 * MainActivity.jaa
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

import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.concurrent.atomic.AtomicBoolean;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button btnConnect = (Button) findViewById(R.id.btnConnect);
        btnConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (!isDSProServiceRunning()) {
                    showDSProAlertDialog();
                } else {
                    if (!_isServiceConnected.get()) bindDSProProxyService();
                }
            }
        });

        Button btnSubmitRoute = (Button) findViewById(R.id.btnSubmitRoute);
        btnSubmitRoute.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (!isDSProServiceRunning()) {
                    showDSProAlertDialog();
                } else {
                    showRouteAlertDialog();
                    if (!_isServiceConnected.get()) bindDSProProxyService();
                }

            }
        });
    }

    private void showDSProAlertDialog() {
        new AlertDialog.Builder(MainActivity.this)
                .setTitle("DSPro not running. Start it now?")
                .setPositiveButton("Start DSPro", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        askSessionKey();
                        dialog.dismiss();
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        dialog.dismiss();
                    }
                }).show();
    }

    private void askSessionKey() {
        final AlertDialog.Builder alertDialog = new AlertDialog.Builder(MainActivity.this);
        alertDialog.setTitle("Session Key");
        alertDialog.setMessage("Enter a key for this DSPro session");

        final EditText input = new EditText(MainActivity.this);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT);
        input.setLayoutParams(lp);


        alertDialog.setView(input);

        alertDialog.setPositiveButton("Submit",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        String sessionKey = input.getText().toString();
                        sendStart(sessionKey);
                        dialog.dismiss();
                    }
                });

        alertDialog.setNegativeButton("Cancel",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.cancel();
                    }
                });

        final AlertDialog dialog = alertDialog.show();
        input.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_ENTER)) {

                    String sessionKey = input.getText().toString();
                    sendStart(sessionKey);
                    dialog.dismiss();
                    return true;
                }
                return false;
            }
        });
    }

    private void sendStart(String sessionKey) {
        Intent launchIntent = getPackageManager().getLaunchIntentForPackage(getString(R.string.app_dspro_package));
        launchIntent.setAction(getString(R.string.intent_start_service));
        launchIntent.putExtra(getString(R.string.intent_session_key), sessionKey);
        startActivity(launchIntent);
    }


    private void showRouteAlertDialog() {
        AlertDialog.Builder adb = new AlertDialog.Builder(MainActivity.this);
        CharSequence items[] = new CharSequence[]{getString(R.string.route_0_descr), getString(R.string.route_1_descr), getString(R.string.route_2_descr)};

        adb.setSingleChoiceItems(items, 0, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Log.d(TAG, "Hit single choice button, which is: " + which);
                sendChoice(which);
                dialog.dismiss();
            }
        });

        adb.setNegativeButton("Cancel", null);
        adb.setTitle("Choose route to submit");
        adb.show();
    }

    private void sendChoice(int which) {
        //send message to service
        Message submitPath = Message.obtain();
        submitPath.obj = MainActivity.class.getSimpleName();
        submitPath.arg1 = which;

        try {
            _messenger.send(submitPath);
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to send initial handshake message to Service", e);
        }
    }


    private void setConnectionStatus(boolean isConnected) {
        TextView connection = (TextView) findViewById(R.id.txtConnection);
        Button btnConnect = (Button) findViewById(R.id.btnConnect);

        if (isConnected) {
            connection.setText(getString(R.string.connection_connected));
            connection.setTextColor(Color.GREEN);
            btnConnect.setClickable(false);
            btnConnect.setEnabled(false);
        } else {
            connection.setText(getString(R.string.connection_not_connected));
            connection.setTextColor(Color.RED);
            btnConnect.setClickable(true);
            btnConnect.setEnabled(true);
        }
    }

    private boolean isDSProServiceRunning() {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (getString(R.string.app_dspro_service_name).equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }

    private void addToCallbacks(String callbackContent) {
        EditText etCallbacks = (EditText) findViewById(R.id.etCallbacks);
        String current = etCallbacks.getText().toString();
        String newCurrent = current + callbackContent + "\n";
        etCallbacks.setText(newCurrent);
    }

    private Handler _activityHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {

            //messages

            switch (msg.arg1) {
                //connected
                case MsgCode.INIT_SUCCESS:
                    setConnectionStatus(true);
                    break;
                case MsgCode.METADATA_ARRIVED:
                    addToCallbacks("Received METADATA_ARRIVED");
                    break;
                case MsgCode.DATA_ARRIVED:
                    addToCallbacks("Received DATA_ARRIVED");
                    break;
                case MsgCode.NEW_NEIGHBOR:
                    addToCallbacks("Received NEW_NEIGHBOR");
                    break;
                case MsgCode.DEAD_NEIGHBOR:
                    addToCallbacks("Received DEAD_NEIGHBOR");
                    break;
                case MsgCode.PATH_REGISTERED:
                    addToCallbacks("Received PATH_REGISTERED");
                    break;
                case MsgCode.POSITION_UPDATED:
                    addToCallbacks("Received POSITION_UPDATED");
                    break;
            }
        }
    };


    private boolean bindDSProProxyService() {

        Intent i = new Intent(this, DSProProxyService.class);
        Log.i(TAG, "Binding " + DSProProxyService.class.getSimpleName());

        return bindService(i, _connection, Context.BIND_AUTO_CREATE);
    }

    /**
     * Defines callbacks for service binding, passed to bindService()
     */
    private ServiceConnection _connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className,
                                       IBinder service) {
            if (service == null) {
                Log.i(TAG, "Service connected but IBinder is null, unable to proceed");
                return;
            }

            String msgService = "Connected to " + DSProProxyService.class.getSimpleName();
            Log.i(TAG, msgService);
            _service = service;
            _isServiceConnected.set(true);
            setConnectionStatus(false);

            //Do the handshake with Service in order to receive messaqges from it
            if (_messenger == null) {
                _messenger = new Messenger(service);
                Message handshake = Message.obtain();
                handshake.obj = MainActivity.class.getSimpleName();
                handshake.arg1 = -1;
                handshake.replyTo = new Messenger(_activityHandler);

                try {
                    _messenger.send(handshake);
                } catch (RemoteException e) {
                    Log.w(TAG, "Unable to send initial handshake message to Service", e);
                }

            }
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            Log.d(TAG, "Called onServiceDisconnected");
        }

        IBinder _service;
    };


    private Messenger _messenger;
    private static final String TAG = "MainActivity";

    private final AtomicBoolean _isServiceConnected = new AtomicBoolean(false);
}
