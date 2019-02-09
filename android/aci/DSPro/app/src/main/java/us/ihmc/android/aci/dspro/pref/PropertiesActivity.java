package us.ihmc.android.aci.dspro.pref;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import org.apache.log4j.Logger;
import us.ihmc.android.aci.dspro.R;
import us.ihmc.android.aci.dspro.util.AndroidUtil;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Properties;

/**
 * PropertiesActivity.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 * @author Kristyna Rehovicova (krehovicova@ihmc.us)
 */
public class PropertiesActivity extends AppCompatActivity implements EditPrefListener {

    private final static Logger LOG = Logger.getLogger(PropertiesActivity.class);
    private String propertiesFilePath;
    private TextView emptyText;
    private PrefAdapter adapter;

    public void onCreate (Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_properties);

        propertiesFilePath = AndroidUtil.getExternalStorageDirectory() + getString(R.string.app_folder) + getString(R.string.app_config_file);

        emptyText = findViewById(R.id.empty);
        EditText prefKey = findViewById(R.id.pref_key);
        EditText prefValue = findViewById(R.id.pref_value);
        Button save = findViewById(R.id.pref_save);
        save.setOnClickListener(v -> {
            if (prefKey.getText().toString().trim().length() == 0) {
                Toast.makeText(this, "Can not save property. Key is empty.", Toast.LENGTH_LONG).show();
                return;
            }

            adapter.addProperty(prefKey.getText().toString(), prefValue.getText().toString());
            prefKey.getText().clear();
            prefValue.getText().clear();

            hideKeyboard();
        });


        setRecycler();

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
        adapter = new PrefAdapter(getProperties(), this);
        recycler.setLayoutManager(new LinearLayoutManager(this));
        recycler.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
        recycler.setAdapter(adapter);

        checkEmptyLog();
    }

    private void checkEmptyLog() {
        emptyText.setVisibility(adapter.getItemCount() == 0 ? View.VISIBLE : View.GONE);
    }

    @Override
    public void onPrefClick(String key, String value, int position) {
        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View dialogView = inflater.inflate(R.layout.dialog_pref_edit, null);
        EditText newValue = dialogView.findViewById(R.id.pref_value);
        newValue.setText(value);

        new AlertDialog.Builder(this)
                .setView(dialogView)
                .setTitle("Setting value for:")
                .setMessage(key)
                .setPositiveButton(R.string.save, (dialog, which) -> adapter.updateProperty(key, newValue.getText().toString(), position))
                .setNegativeButton(android.R.string.cancel, (dialog, which) -> {})
                .setOnDismissListener(dialog -> hideKeyboard())
                .show();
    }

    @Override
    public void onPrefLongClick(String key, String value, int position) {
        new AlertDialog.Builder(this)
                .setTitle(R.string.warning)
                .setMessage(Html.fromHtml("Are you sure you want to delete <font color='#0099cc'>" + key + "</font> property?"))
                .setPositiveButton(R.string.delete, (dialog, which) -> adapter.deleteProperty(key, position))
                .setNegativeButton(android.R.string.no, (dialog, which) -> {})
                .show();
    }

    private Properties getProperties() {
        Properties properties = new Properties();
        try {
            FileInputStream fis = new FileInputStream(propertiesFilePath);
            properties.load(fis);
            fis.close();
        } catch (IOException e) {
            LOG.error("dspro.properties file not found");
            e.printStackTrace();
        }

        return properties;
    }

    @Override
    protected void onPause() {
        // Save properties before user navigates from activity
        Properties properties = adapter.getProperties();
        try {
            FileOutputStream fos = new FileOutputStream(propertiesFilePath);
            properties.store(fos, "properties saved from activity");
            fos.close();
        } catch (IOException e) {
            LOG.error("dspro.properties file not found");
            e.printStackTrace();
        }

        super.onPause();
    }

    private void hideKeyboard() {
        View view = this.getCurrentFocus();
        if (view != null) {
            InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }

}
