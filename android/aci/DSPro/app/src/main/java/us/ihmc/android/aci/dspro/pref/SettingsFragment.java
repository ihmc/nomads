package us.ihmc.android.aci.dspro.pref;

import android.app.AlertDialog;
import android.app.FragmentTransaction;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.support.annotation.StringRes;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.SeekBar;
import android.widget.Toast;

import org.apache.log4j.Logger;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.prefs.PreferenceChangeEvent;
import java.util.prefs.PreferenceChangeListener;

import us.ihmc.android.aci.dspro.R;
import us.ihmc.android.aci.dspro.util.AndroidUtil;
import us.ihmc.android.aci.dspro.util.CSVParser;
import us.ihmc.android.aci.dspro.util.SortedProperties;

/**
 * SettingsFragment.java
 * <p/>
 * Class <code>SettingsFragment</code> extends an Android Fragment and handles the configurations settings of the app.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 * @author Kristyna Rehovicova (krehovicova@ihmc.us)
 */
public class SettingsFragment extends PreferenceFragment implements SharedPreferences.OnSharedPreferenceChangeListener {

    private SortedProperties _config;
    private final static Logger LOG = Logger.getLogger(SettingsFragment.class);
    private final List<String> booleanFields = new ArrayList<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Load the preferences from an XML resource
        addPreferencesFromResource(R.xml.preferences);
        loadPreferenceSummaries();

        if (isDisServiceSelected()) {
            PreferenceCategory prefTransport = (PreferenceCategory) findPreference(getString(R.string
                    .pref_transport_key));

            //remove mockets
            prefTransport.removePreference(findPreference(getString(R.string.aci_dspro_adaptor_mockets_enable)));
            prefTransport.removePreference(findPreference(getString(R.string.aci_dspro_adaptor_mockets_port)));
            prefTransport.removePreference(findPreference(getString(R.string.aci_dspro_adaptor_mockets_peer_addr)));
            //remove TCP
            prefTransport.removePreference(findPreference(getString(R.string.aci_dspro_adaptor_tcp_enable)));
            prefTransport.removePreference(findPreference(getString(R.string.aci_dspro_adaptor_tcp_port)));
            prefTransport.removePreference(findPreference(getString(R.string.aci_dspro_adaptor_tcp_peer_addr)));

            //set DisService always enabled
            findPreference(getString(R.string.aci_dspro_adaptor_disservice_enable)).setEnabled(true);
            findPreference(getString(R.string.aci_dspro_adaptor_disservice_enable)).setSelectable(false);

            PreferenceCategory prefRanking = (PreferenceCategory) findPreference(getString(R.string.pref_ranking_key));
            prefRanking.removeAll();
            getPreferenceScreen().removePreference(prefRanking);

            PreferenceCategory prefNodeContext = (PreferenceCategory) findPreference(getString(R.string
                    .pref_node_context_key));
            prefNodeContext.removeAll();
            getPreferenceScreen().removePreference(prefNodeContext);

            PreferenceCategory prefMisc = (PreferenceCategory) findPreference(getString(R.string.pref_misc_key));
            prefMisc.removePreference(findPreference(getString(R.string.aci_dspro_informationPush_limitPrestagingToLocalData)));
            prefMisc.removePreference(findPreference(getString(R.string.aci_dspro2app_proxy_enable)));
            prefMisc.removePreference(findPreference(getString(R.string.aci_dspro2app_runtime_changes)));
        }

        // Add every boolean field for data type cast
        booleanFields.add(getString(R.string.aci_dspro2app_mutexlogger_enable));
        booleanFields.add(getString(R.string.aci_dspro2app_runtime_changes));
        booleanFields.add(getString(R.string.aci_dspro2app_extraNetIFs_enable));
        booleanFields.add(getString(R.string.aci_dspro_adaptor_disservice_enable));
        booleanFields.add(getString(R.string.aci_dspro_adaptor_mockets_enable));
        booleanFields.add(getString(R.string.aci_dspro_adaptor_tcp_enable));
        booleanFields.add(getString(R.string.aci_dspro2app_deleteStorageOnExit));
        booleanFields.add(getString(R.string.aci_dspro_informationPush_limitPrestagingToLocalData));
        booleanFields.add(getString(R.string.aci_dspro2app_autoRestartService));
        booleanFields.add(getString(R.string.aci_dspro2app_networkReload_enable));
        booleanFields.add(getString(R.string.aci_dspro2app_proxy_enable));
        booleanFields.add(getString(R.string.netlogger_enable));
        booleanFields.add(getString(R.string.aci_dspro_adaptor_nats_enable));
        booleanFields.add(getString(R.string.aci_dspro_adaptor_udp_enable));
    }

    private boolean isDisServiceSelected() {
        SharedPreferences settings = getPreferenceManager().getSharedPreferences();
        String selectedMode = settings.getString(getString(R.string.aci_dspro2app_selectMode), getString(R.string.aci_dspro2app_selectMode_default));
        String[] modes = getResources().getStringArray(R.array.pref_select_mode_array_entries);

        LOG.debug(selectedMode + " selected, removing unnecessary preferences");
        //if DisService selected
        return selectedMode.equals(modes[1]);
    }

    @Override
    public void onPause() {
        super.onPause();
        getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
        savePreferences();
    }

    @Override
    public void onResume() {
        LOG.info("Calling onResume");
        super.onResume();
        loadPreferenceSummaries();
        getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        Preference pref = findPreference(key);
        if (pref == null)
            return;

        String strValue;
        if (isBoolean(key)) {
            Boolean bValue = sharedPreferences.getBoolean(key, false);
            strValue = String.valueOf(bValue);
        } else {
            strValue = sharedPreferences.getString(key, "");
            String summary = strValue;

            if (key.equals(getString(R.string.aci_disService_storageMode))) {
                //translate value to summary
                ListPreference listPref = (ListPreference) findPreference(key);
                summary = listPref.getEntry().toString();
            } else if (key.equals(getString(R.string.aci_disService_networkMessageService_encryptionMode))) {
                //translate value to summary
                switch (strValue) {
                    case "0":
                        _config.remove(getString(R.string.aci_disService_networkMessageService_groupKeyFile));
                        _config.setProperty(getString(R.string.aci_disService_networkMessageService_passphrase_encryption), "false");
                        _config.setProperty(getString(R.string.aci_disService_networkMessageService_authenticator_encryption), "false");
                        break;
                    case "1":
                        _config.remove(getString(R.string.aci_disService_networkMessageService_groupKeyFile));
                        _config.setProperty(getString(R.string.aci_disService_networkMessageService_passphrase_encryption), "true");
                        _config.setProperty(getString(R.string.aci_disService_networkMessageService_authenticator_encryption), "false");
                        break;
                    default:
                        _config.setProperty(getString(R.string.aci_disService_networkMessageService_passphrase_encryption), "false");
                        _config.setProperty(getString(R.string.aci_disService_networkMessageService_authenticator_encryption), "true");
                        _config.setPropertySafely(getString(R.string.aci_disService_networkMessageService_groupKeyFile), getString(R.string.aci_disService_networkMessageService_groupKeyFile_default));
                        break;
                }

                AndroidUtil.changeActionBarColor(((AppCompatActivity)getActivity()), strValue);
                ListPreference listPref = (ListPreference) findPreference(key);
                summary = listPref.getEntry().toString();
            }
            // Set summary to be the user-description for the selected value
            pref.setSummary(summary);
        }
        if (_config != null) {
            LOG.info("Changed " + key + " to value " + strValue);
            _config.setProperty(key, strValue);
        }
    }

    private boolean isBoolean(String key) {
        for (String name : booleanFields) {
            if (key.equals(name))
                return true;
        }
        return false;
    }

    private void loadPreferenceSummaries() {
        //get configuration file path
        String configFilePath = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) + getString(R.string.app_config_file);
        _config = AndroidUtil.readConfigFile(configFilePath);
        if (_config == null) {
            LOG.error("Unable to read configuration file on path:" + configFilePath);
            return;
        }

        setupPreferenceFromConfig(R.string.aci_dspro2app_WiFiIFs);
        setupPreferenceFromConfig(R.string.aci_dspro2app_extraNetIFs_enable);
        setupPreferenceFromConfig(R.string.aci_dspro2app_extraNetIFs);
        setupPreferenceFromConfig(R.string.aci_disService_networkMessageService_port);
        setupPreferenceFromConfig(R.string.aci_disService_networkMessageService_mcastTTL);

        findPreference(getString(R.string.aci_dspro2app_extraNetIFs_enable)).setOnPreferenceChangeListener(new CustomPreferenceChangeListener());
        findPreference(getString(R.string.aci_dspro2app_networkReload_enable)).setOnPreferenceChangeListener(new CustomPreferenceChangeListener());

        AutoCompletePreference prefNodeUUID = (AutoCompletePreference) findPreference(getString(R.string.aci_disService_nodeUUID));
        prefNodeUUID.setSummary(_config.getProperty(getString(R.string.aci_disService_nodeUUID)));
        prefNodeUUID.setAdapter(getAddressBookNames());

        //Multicast Address
        setupPreferenceFromConfig(R.string.nms_transmission_outgoingAddr);
        setupPreferenceFromConfig(R.string.aci_disService_nodeConfiguration_bandwidth);

        //DSPro only
        if (!isDisServiceSelected()) {
            setupPreferenceFromConfig(R.string.aci_dspro_adaptor_mockets_enable);
            setupPreferenceFromConfig(R.string.aci_dspro_adaptor_mockets_port);
            setupPreferenceFromConfig(R.string.aci_dspro_adaptor_mockets_peer_addr);

            setupPreferenceFromConfig(R.string.aci_dspro_adaptor_tcp_enable);
            setupPreferenceFromConfig(R.string.aci_dspro_adaptor_tcp_port);
            setupPreferenceFromConfig(R.string.aci_dspro_adaptor_tcp_peer_addr);

            // * RANKING *
            setupPreferenceFromConfig(R.string.aci_dspro_metadataRanker_coordRankWeight);
            setupPreferenceFromConfig(R.string.aci_dspro_metadataRanker_timeRankWeight);
            setupPreferenceFromConfig(R.string.aci_dspro_metadataRanker_expirationRankWeight);
            setupPreferenceFromConfig(R.string.aci_dspro_metadataRanker_impRankWeight);
            setupPreferenceFromConfig(R.string.aci_dspro_metadataRanker_predRankWeight);
            setupPreferenceFromConfig(R.string.aci_dspro_metadataRanker_targetRankWeight);
            setupPreferenceFromConfig(R.string.aci_dspro_metadataRanker_srcRelRankWeight);
            setupPreferenceFromConfig(R.string.aci_dspro_metadataRanker_infoContentRankWeight);

            // * NODE CONTEXT *
            setupPreferenceFromConfig(R.string.aci_dspro_informationPush_rankThreshold);
            setupPreferenceFromConfig(R.string.aci_dspro_localNodeContext_usefulDistance);
            setupPreferenceFromConfig(R.string.aci_dspro_localNodeContext_usefulDistanceByType);

            findPreference(getString(R.string.aci_dspro2app_proxy_enable)).setOnPreferenceChangeListener(new CustomPreferenceChangeListener());
            findPreference(getString(R.string.aci_dspro2app_runtime_changes)).setOnPreferenceChangeListener(new CustomPreferenceChangeListener());
        }

        setupPreferenceFromConfig(R.string.aci_disService_storageFile);
        setupPreferenceFromConfig(R.string.aci_disService_statusNotifyAddress);

        String strValue = _config.getProperty(getString(R.string.aci_disService_storageMode));
        if (strValue != null)
            findPreference(getString(R.string.aci_disService_storageMode)).setSummary(strValue.equals("0") ? "On memory" : "On file");

        findPreference(getString(R.string.aci_disService_storageMode)).setOnPreferenceChangeListener(new CustomPreferenceChangeListener());

        // * ENCRYPTION *
        strValue = _config.getProperty(getString(R.string.aci_disService_networkMessageService_encryptionMode));
        if (strValue != null) {
            String encSummary;
            switch (strValue) {
                case "0":
                    encSummary = "None";
                    break;
                case "1":
                    encSummary = "Passphrase Encryption";
                    break;
                default:
                    encSummary = "Authenticator Encryption";
                    break;
            }
            findPreference(getString(R.string.aci_disService_networkMessageService_encryptionMode)).setSummary(encSummary);
        }
        findPreference(getString(R.string.aci_disService_networkMessageService_encryptionMode)).setOnPreferenceChangeListener(new CustomPreferenceChangeListener());

        // NetLogger
        setupPreferenceFromConfig(R.string.nats_broker_addr);
        setupPreferenceFromConfig(R.string.nats_broker_port);
        setupPreferenceFromConfig(R.string.aci_dspro_adaptor_udp_peer_addr);
        setupPreferenceFromConfig(R.string.aci_dspro_adaptor_udp_port);
    }

    private void setupPreferenceFromConfig(@StringRes int key) {
        String keyVal = getString(key);
        Preference pref = findPreference(keyVal);
        pref.setSummary(_config.getProperty(keyVal));
        pref.setOnPreferenceChangeListener(new CustomPreferenceChangeListener());
    }

    private class CustomPreferenceChangeListener implements Preference.OnPreferenceChangeListener {
        @Override
        public boolean onPreferenceChange(Preference preference, Object o) {
            if (preference.getKey().equals(getString(R.string.aci_disService_networkMessageService_encryptionMode))) {
                if (o.toString().equals("0")) {
                    _config.remove(getString(R.string.aci_disService_networkMessageService_groupKeyFile));
                    findPreference(getString(R.string.aci_disService_networkMessageService_encryptionMode)).setSummary("None");
                } else if (o.toString().equals("1")) {
                    _config.remove(getString(R.string.aci_disService_networkMessageService_groupKeyFile));
                    findPreference(getString(R.string.aci_disService_networkMessageService_encryptionMode)).setSummary("Passphrase Encryption");
                } else
                    findPreference(getString(R.string.aci_disService_networkMessageService_encryptionMode)).setSummary("Authenticator Encryption");
                AndroidUtil.changeActionBarColor(((AppCompatActivity)getActivity()), o.toString());
                showRestartDialog();
            } else if (preference.getKey().equals(getString(R.string.aci_disService_storageMode))) {
                findPreference(getString(R.string.aci_disService_storageMode)).setSummary(o.toString().equals("0") ? "On memory" : "On file");
            } else {
                preference.setSummary(o.toString());
            }
            _config.setProperty(preference.getKey(), o.toString());
            savePreferences();
            return true;
        }

        private void showRestartDialog() {
            new AlertDialog.Builder(getActivity())
                    .setMessage(R.string.please_restart_app)
                    .setNegativeButton(android.R.string.ok, (dialog, which) -> {})
                    .show();
        }
    }

    private void savePreferences() {
        if (_config == null) {
            LOG.error("Configuration object is null");
        }

        String configFile = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) + getString(R
                .string.app_config_file);

        SharedPreferences settings = getPreferenceManager().getSharedPreferences();
        String netInterface;
        if (settings.getBoolean(getString(R.string.aci_dspro2app_extraNetIFs_enable),
                Boolean.valueOf(getString(R.string.aci_dspro2app_extraNetIFs_enable_default)))) {
            netInterface = settings.getString(getString(R.string.aci_dspro2app_extraNetIFs),
                    getString(R.string.aci_dspro2app_extraNetIFs_default));
            LOG.debug("Using manual config. for interfaces. Setting " + getString(R.string.aci_disService_netIFs)
                    + " to " + netInterface);
        } else {
            netInterface = AndroidUtil.getAllInterfacesDSProFormat(AndroidUtil.getAllAvailableNetworkInterfaces());
            LOG.debug("Using all interfaces available. Setting " + getString(R.string.aci_disService_netIFs) + " to " +
                    netInterface);
        }

        _config.setProperty(getString(R.string.aci_disService_netIFs), netInterface);
        LOG.info("Preferences saved on file " + configFile);
        AndroidUtil.writeConfigFile(_config, configFile);
    }

    public ArrayList<String> getAddressBookNames() {
        ArrayList<String> addressBookNames = new ArrayList<>();
        String path = Environment.getExternalStorageDirectory() + "/ihmc/conf/addressBook.csv";
        File file = new File(path);
        if (!file.exists())
            return addressBookNames;

        CSVParser parser;
        parser = new CSVParser();
        try {
            BufferedReader br = new BufferedReader(new FileReader(file));
            String line;
            while ((line = br.readLine()) != null) {
                String[] row = parser.parseLine(line);
                addressBookNames.add(row[5]);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        // Remove name of the column
        if (addressBookNames.size() > 0)
            addressBookNames.remove(0);

        return addressBookNames;
    }

}
