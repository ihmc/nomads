package us.ihmc.android.aci.dspro.util;

import android.net.DhcpInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.text.Html;
import android.util.Log;
import android.view.Gravity;
import android.widget.ImageView;

import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
import com.google.gson.reflect.TypeToken;

import java.io.*;
import java.lang.reflect.Type;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Map;
import java.util.Properties;

import us.ihmc.android.aci.dspro.R;

/**
 * AndroidUtil.java
 * <p/>
 * Common Android support functions to handle storage, WiFi features and more.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 * @author Kristyna Rehovicova (krehovicova@ihmc.us)
 */
public class AndroidUtil
{
    public static class NetInterface
    {
        public String name;
        public String ip;

        public NetInterface (String name, String ip)
        {
            this.name = name;
            this.ip = ip;

        }

        @Override
        public String toString ()
        {
            return this.name + " " + this.ip;
        }
    }

    private static String LOG_TAG = "AndroidUtil";

    private AndroidUtil ()
    {
        throw new AssertionError();
    }

    /**
     * Get the e.printStackTrace() as a String
     *
     * @param t Throwable e from Exception
     * @return
     */
    public static String getStackTraceAsString (Throwable t)
    {
        final Writer sw = new StringWriter();
        t.printStackTrace(new PrintWriter(sw));
        return sw.toString();
    }

    /**
     * Get the local interface IP Address (if any).
     *
     * @return String representing the local IP Address
     */
    public static String getLocalIPAddress ()
    {
        String localIPAddress = "";

        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface
                    .getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress()) {
                        localIPAddress = inetAddress.getHostAddress();
                    }
                }
            }
        }
        catch (SocketException ex) {
            Log.e(LOG_TAG, ex.toString());
        }

        return localIPAddress;
    }

    public static List<NetInterface> getAllAvailableNetworkInterfaces ()
    {
        List<NetInterface> iFaceList = new ArrayList<NetInterface>();
        //HACK
        iFaceList.add(new AndroidUtil.NetInterface("All available", ""));
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface
                    .getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress()) {

                        String ipAddress = inetAddress.getHostAddress();
                        if (ipAddress.contains(":")) continue;

                        String ifaceName = intf.getName();
                        iFaceList.add(new NetInterface(ifaceName, ipAddress));
                    }
                }
            }
        }
        catch (SocketException ex) {
            Log.e(LOG_TAG, ex.toString());
        }

        return iFaceList;
    }

    public static String getAllInterfacesDSProFormat (List<AndroidUtil.NetInterface> iFaceList)
    {
        StringBuilder sb = new StringBuilder();
        //log all available interfaces
        Log.i(LOG_TAG, "List of available network interfaces");
        for (int i = 0; i < iFaceList.size(); i++) {
            Log.i(LOG_TAG, iFaceList.get(i).toString());
            if (!iFaceList.get(i).name.equals("All available")) {
                sb.append(iFaceList.get(i).ip);
                if (i != iFaceList.size() - 1)
                    sb.append(";");
            }

        }
        return sb.toString();
    }

    /**
     * Get the local WiFi interface IP Address (if any).
     *
     * @param wifiManager representing an object of type (WifiManager) getSystemService(WIFI_SERVICE);
     * @return String representing the local WiFi IP Address
     */
    public static String getWiFiIPAddress (WifiManager wifiManager)
    {

        String wifiIPAddress = "";

        try {
            DhcpInfo dhcpInfo = wifiManager.getDhcpInfo();
            // Get the IP Address
            wifiIPAddress = intToIp((dhcpInfo.ipAddress));
        }
        catch (Exception e) {
            Log.e(LOG_TAG, e.toString());
        }

        return wifiIPAddress;
    }

    /**
     * Method for converting an existing IP in Integer format to String format.
     *
     * @param integerIp The IP Address in Integer format
     * @return A String representation of the IP Address
     */
    public static String intToIp (int integerIp)
    {
        //    return ((integerIp >> 24) & 0xFF) + "." + ((integerIp >> 16) & 0xFF)
        //            + "." + ((integerIp >> 8) & 0xFF) + "." + (integerIp & 0xFF);

        return (integerIp & 0xFF) + "." + ((integerIp >> 8) & 0xFF)
                + "." + ((integerIp >> 16) & 0xFF) + "." + ((integerIp >> 24) & 0xFF);
    }

    /**
     * Method for the validation of the input IP Address.
     *
     * @param ipAddress The IP Address input string
     * @return True if the IP is valid, false otherwise
     */
    public static boolean isValidIPAddress (String ipAddress)
    {
        String[] parts = ipAddress.split("\\.");

        if (parts.length != 4) {
            return false;
        }

        for (String s : parts) {
            int i = Integer.parseInt(s);
            if ((i < 0) || (i > 255)) {
                return false;
            }
        }

        return true;
    }

    public static boolean isValidPort (String port)
    {
        if (port == null || port.equals(""))
            return false;

        try {
            int iPort = Integer.parseInt(port);
            if (iPort < 0 || iPort > 65535) {
                return false;
            }
        }
        catch (NumberFormatException e) {
            return false;
        }

        return true;
    }

    public static SortedProperties readConfigFile (String filePath)
    {

        File configFile = new File(filePath);
        SortedProperties configProperties = new SortedProperties();

        try {
            FileInputStream fileInputStream = new FileInputStream(configFile);
            configProperties.load(fileInputStream);
        }
        catch (FileNotFoundException e) {
            Log.e(LOG_TAG, "Configuration file not found");
            return null;
        }
        catch (IOException e) {
            Log.e(LOG_TAG, "Error reading configuration file");
            return null;

        }

        return configProperties;
    }

    public static boolean writeConfigFile (Properties configProp, String filePath)
    {
        try {
            FileOutputStream fileOutputStream = new FileOutputStream(filePath);
            configProp.store(fileOutputStream, null);
        }
        catch (NullPointerException | IOException ex) {
            Log.e(LOG_TAG, "Error writing config file");
            return false;
        }

        Log.i(LOG_TAG, "Config file written successfully");
        return true;
    }

    public static String getDeviceModel ()
    {
        return Build.MANUFACTURER + "-" + Build.MODEL;
    }

    public static String getExternalStorageDirectory ()
    {

        String sdCardRoot = null;
        String phoneModel = Build.MODEL;
        Log.i("AndroidUtil", "The phone model is: " + phoneModel);

        if (phoneModel.equals("DROID X2")) {
            sdCardRoot = "/sdcard-ext";
        }
        else if (phoneModel.equals("SGH-I987")) {
            sdCardRoot = "/sdcard/external_sd";
        }
        else {
            try {
                sdCardRoot = Environment.getExternalStorageDirectory().getAbsolutePath().toString();
            }
            catch (Exception e) {
                sdCardRoot = "/sdcard";
            }
        }

        Log.i("AndroidUtil", "The SDCard root path is: " + sdCardRoot);
        return sdCardRoot;
    }

    public static boolean deleteStorage (String filePath)
    {
        if (filePath == null)
            return false;

        File dbFile = new File(filePath);

        if (!dbFile.exists()) {
            Log.w(LOG_TAG, "Trying to delete a storage file that doesn't exist");
            return false;
        }

        return dbFile.delete();
    }

    public static void changeActionBarColor(AppCompatActivity activity, String encryptionMode)
    {
        if (encryptionMode == null) {
            encryptionMode = "0";
        }
        ActionBar actionBar = activity.getSupportActionBar();
        if (actionBar == null)
            return;
        String encryptionTitle = null;
        //default color
        String color = null;
        /**
         * Setting the lock_image
         */
        ImageView lockImageView = new ImageView(actionBar.getThemedContext());
        lockImageView.setScaleType(ImageView.ScaleType.CENTER);
        ActionBar.LayoutParams layoutParams = new ActionBar.LayoutParams(
                ActionBar.LayoutParams.WRAP_CONTENT,
                ActionBar.LayoutParams.WRAP_CONTENT, Gravity.RIGHT | Gravity.CENTER_VERTICAL);
        layoutParams.rightMargin = 40;

        switch (encryptionMode) {
            case "0": {
                color = "#009900";
                encryptionTitle = "(No Encryption)";
                lockImageView.setImageResource(R.drawable.lock_open);
                break;
            }
            case "1": {
                color = "#CC0000";
                encryptionTitle = "(Passphrase Encryption)";
                lockImageView.setImageResource(R.drawable.lock_close);
                break;
            }
            case "2": {
                color = "#CC0000";
                encryptionTitle = "(Authenticator Encryption)";
                lockImageView.setImageResource(R.drawable.lock_close_auth);
                break;
            }
            default: {
                color = "#009900";
                encryptionTitle = "(No Encryption)";
                lockImageView.setImageResource(R.drawable.lock_open);
                break;
            }
        }
        /*setting the new color and the image*/
        lockImageView.setLayoutParams(layoutParams);
        actionBar.setCustomView(lockImageView);
        actionBar.setDisplayShowCustomEnabled(true);
        actionBar.setSubtitle(Html.fromHtml("<font color='"+color+"'>"+encryptionTitle+"</font>"));
    }

    public static Map<String, String> jsonToKeyValueMap(String json) throws JsonSyntaxException {
        Type type = new TypeToken<Map<String, String>>(){}.getType();
        return new Gson().fromJson(json, type);
    }

    public static String mapToJson(Map<String, ?> map) {
        return new Gson().toJson(map);
    }

}