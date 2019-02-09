package us.ihmc.android.aci.dspro;

import android.Manifest;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.widget.Toast;

import us.ihmc.android.aci.dspro.util.AndroidUtil;

public class DSProPermissionActivity extends Activity {

    private static final int REQUEST_WP_CODE = 0x11;
    private static final String TAG = DSProPermissionActivity.class.getSimpleName();
    private boolean _intentsReceiverRegistered = false;
    private IntentsReceiver _intentsReceiver;
    /**
     * Verify and request the permission to write on the external storage
     * @param activity the activity who requested to write on the external Storea
     */
    public static boolean verifyExternalStoragePermissions(Activity activity) {
        // Check if we have write permission
        int permission = ContextCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (permission != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                    activity,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_WP_CODE);
            return false;
        } else {
            //the application has already the permissions
            return true;
        }
    }


    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case REQUEST_WP_CODE : {
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    //permission granted switch to DSProActivity
                    Log.d(TAG, "onRequestPermissionsResult(): " +
                            "Permission granted, starting DSProActivity");
                    openDSProMainActivity();
                } else {
                    Log.d(TAG, "onRequestPermissionsResult(): " +
                            "Pemission denied, closing the Application");
                    Toast.makeText(getApplicationContext(), "PERMISSION_DENIED", Toast.LENGTH_SHORT).show();
                    System.exit(-1);
                }
            }
        }
    }

    class IntentsReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "***IntentsReceiver - received intent with action: " + action);
        }
    }


    public void openDSProMainActivity()
    {
        Intent receivedIntent = getIntent();
        if (receivedIntent == null) {
            Log.d(TAG, "The receivedIntent is null");
        }
        Intent intent = new Intent(this, DSProActivity.class);
        if (intent != null) {
            String action = receivedIntent != null ? receivedIntent.getAction() : null;
            if (action != null) {
                intent.setAction(action);
            }
            String sessionKey = receivedIntent != null ?
                    receivedIntent.getStringExtra(getString(R.string.intent_session_key)) : null;
            if (sessionKey != null ) {
                intent.putExtra(getString(R.string.intent_session_key), sessionKey);
            }
            Log.d(TAG, "Starting activity");
            finish();
            startActivity(intent);
        } else {
            Log.d(TAG, "Impossible to start the new Activity");
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dspro_permission);
        _intentsReceiver = new IntentsReceiver();
        Intent intent = getIntent();
        if (intent.getAction() != null && intent.getAction().equals(getString(R.string.intent_exit_dspro))) {
            Log.d(TAG, "Received intent to terminate DSPro");
            exit();
            return;
        }
        if (verifyExternalStoragePermissions(this)) {
            //open the activity
            Log.d(TAG, "Permission ok, starting the activity");
            openDSProMainActivity();
        }

    }

    @Override
    public void onResume()
    {
        super.onResume();
        setIntentFilter();
        //if (verifyExternalStoragePermissions(this)) {
            //open the activity
        //    Log.d(TAG, "Permission ok, starting the activity");
            //openDSProMainActivity();
        //}
    }

    private void exit() {
        //input validation
        //explicitly call garbage collector
        System.gc();
        onDestroy();
        //android.os.Process.killProcess(android.os.Process.myPid());
    }

    public void stopServices() {
        Intent i = new Intent(this, DSProService.class);
        stopService(i);
        Log.d(TAG, "Stopping DSProService from DSProPermissionActivity");
    }

    private void setIntentFilter() {
        if (!_intentsReceiverRegistered) {
            Log.d(TAG,"Registering IntentReceiver");
            IntentFilter filter = new IntentFilter();
            filter.addAction(getString(R.string.intent_start_service));
            filter.addAction(getString(R.string.intent_stop_service));
            filter.addAction(getString(R.string.intent_restart_service));
            registerReceiver(_intentsReceiver, filter);
            _intentsReceiverRegistered = true;
        }
    }

    @Override
    public void onDestroy() {
        if (_intentsReceiverRegistered) {
            Log.d(TAG, "onDestory(): Unregistering IntentsReceiver");
            unregisterReceiver(_intentsReceiver);
            _intentsReceiverRegistered = false;
        }
        super.onDestroy();

        finishAffinity();
    }
}
