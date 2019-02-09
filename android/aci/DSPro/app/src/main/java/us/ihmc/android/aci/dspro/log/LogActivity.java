package us.ihmc.android.aci.dspro.log;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.SearchView;
import android.widget.TextView;
import android.widget.ToggleButton;

import org.apache.log4j.Logger;
import us.ihmc.android.aci.dspro.R;
import us.ihmc.android.aci.dspro.util.AndroidUtil;
import us.ihmc.android.aci.dspro.util.StringList;

import java.io.IOException;
import java.util.Properties;

/**
 * LogActivity.java
 * <p/>
 * Activity that reads and displays the DSPro2.item_log.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 * @author Kristyna Rehovicova (krehovicova@ihmc.us)
 */
public class LogActivity extends AppCompatActivity
{
    private final static Logger LOG = Logger.getLogger(LogActivity.class);
    private StringList filteredLog;
    private LogAdapter adapter;
    private TextView emptyText;

    public void onCreate (Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_log);

        Intent intent = getIntent();
        String logLevelStr = intent.getStringExtra(getString(R.string.log_level));
        filteredLog = getFilteredLog(logLevelStr);

        emptyText = findViewById(R.id.empty);

        setRecycler();
        setSearchView();

        /**
         * Setting the color of the actionBar
         */
        String configFilePath = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) + getString(R.string.app_config_file);
        Properties config = AndroidUtil.readConfigFile(configFilePath);
        String encryptionMode = config.getProperty(getString(R.string.aci_disService_networkMessageService_encryptionMode));
        AndroidUtil.changeActionBarColor(this, encryptionMode);
    }

    private void setRecycler() {
        RecyclerView recycler = findViewById(R.id.log_recycle_view);
        adapter = new LogAdapter(filteredLog);
        recycler.setLayoutManager(new LinearLayoutManager(this));
        recycler.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
        recycler.setAdapter(adapter);

        checkEmptyLog();
    }

    private void setSearchView() {
        SearchView search = findViewById(R.id.log_search);
        search.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
            @Override
            public boolean onQueryTextSubmit(String query) {
                filter(query);

                return true;
            }

            @Override
            public boolean onQueryTextChange(String newText) {
                if (newText.length() == 0)
                    adapter.refreshItems(filteredLog);

                adapter.searchText(newText);

                return true;
            }
        });
    }

    private void checkEmptyLog() {
        emptyText.setVisibility(adapter.getItemCount() == 0 ? View.VISIBLE : View.GONE);
    }

    private void filter(String newText) {
        StringList filtered = new StringList();
        if (newText != null && newText.length() > 0) {
            for (String item : filteredLog) {
                if (item.toLowerCase().contains(newText.toLowerCase()))
                    filtered.add(item);
            }
        } else
            filtered = filteredLog;

        adapter.refreshItems(filtered);
        checkEmptyLog();
    }

    private StringList getFilteredLog(String logLevelStr) {
        //read item_log
        StringList log = new StringList();
        LogLevel logLevel = null;
        StringList filteredLog = new StringList();
        try {
            log.read(getLogFilePath());
            logLevel = LogLevel.valueOf(logLevelStr);
            LOG.info("Size of the current item_log: " + log.size());
        }
        catch (IOException e) {
            String strError = "Unable to read DSPro/DisService item_log file";
            LOG.error(strError);
            log.add(0, strError);

        }
        catch (IllegalArgumentException e) {
            e.printStackTrace();
        }


        if (logLevel == null || logLevel.getLevel() == LogLevel.HighDetailDebug.getLevel()) {
            filteredLog = log;
        }
        else {
            int logNum = logLevel.getLevel();
            for (String line : log) {
                for (int i = logNum; i > 0; i--) {
                    if (line.contains(" " + i + " ")) {
                        filteredLog.add(line);
                        break;
                    }
                }
            }
        }

        return filteredLog;
    }

    private String getLogFilePath ()
    {
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        String selectMode = settings.getString(getString(R.string.aci_dspro2app_selectMode),
                getString(R.string.aci_dspro2app_selectMode_default));
        Resources res = getResources();
        String[] modes = res.getStringArray(R.array.pref_select_mode_array_entries);
        String appFolder = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder);
        String logFilePath = selectMode.equalsIgnoreCase(modes[0]) ? appFolder +
                getString(R.string.app_dspro2_log_file) : appFolder + getString(R.string.app_disservice_log_file);
        LOG.info("Log file path is set to: " + logFilePath);
        return logFilePath;
    }
}