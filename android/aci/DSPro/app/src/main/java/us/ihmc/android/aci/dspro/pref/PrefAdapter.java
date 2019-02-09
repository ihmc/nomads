package us.ihmc.android.aci.dspro.pref;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Properties;

import us.ihmc.android.aci.dspro.R;
import us.ihmc.android.aci.dspro.log.ItemHolder;

/**
 * Created on 5/14/18
 *
 * @author Kristyna Rehovicova (krehovicova@ihmc.us).
 */

public class PrefAdapter extends RecyclerView.Adapter<ItemHolder> {

    private final Properties properties;
    private final List<String> propertyNames;
    private final EditPrefListener listener;

    PrefAdapter(Properties properties, EditPrefListener listener) {
        this.properties = properties;
        this.propertyNames = new ArrayList<>();
        propertyNames.addAll(properties.stringPropertyNames());
        Collections.sort(propertyNames);
        this.listener = listener;
    }

    @Override
    public ItemHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_log, parent, false);

        return new ItemHolder(view);
    }

    @Override
    public void onBindViewHolder(ItemHolder holder, int position) {
        String key = propertyNames.get(position);
        String value = properties.getProperty(key);

        holder.itemView.setOnClickListener(v -> {
            listener.onPrefClick(key, value, position);
        });

        holder.itemView.setOnLongClickListener(v -> {
            listener.onPrefLongClick(key, value, position);
            return true;
        });

        holder.bindPref(key + "=" + value);
    }

    @Override
    public int getItemCount() {
        return properties.size();
    }

    void updateProperty(String key, String value, int position) {
        properties.setProperty(key, value);
        notifyItemChanged(position);
    }

    void addProperty(String key, String value) {
        properties.setProperty(key, value);
        propertyNames.add(key);
        notifyItemInserted(properties.size()-1);
    }

    void deleteProperty(String key, int position) {
        properties.remove(key);
        propertyNames.addAll(properties.stringPropertyNames());
        notifyItemRemoved(position);
    }

    public Properties getProperties() {
        return properties;
    }

}
