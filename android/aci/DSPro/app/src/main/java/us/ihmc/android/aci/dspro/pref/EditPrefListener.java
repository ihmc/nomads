package us.ihmc.android.aci.dspro.pref;

/**
 * Created on 5/15/18
 *
 * @author Kristyna Rehovicova (krehovicova@ihmc.us).
 */

interface EditPrefListener {

    void onPrefClick(String key, String value, int position);
    void onPrefLongClick(String key, String value, int position);

}
