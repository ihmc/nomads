package us.ihmc.android.aci.dspro.log;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.List;

import us.ihmc.android.aci.dspro.R;

/**
 * Created on 5/14/18
 *
 * @author Kristyna Rehovicova (krehovicova@ihmc.us).
 */

public class LogAdapter extends RecyclerView.Adapter<ItemHolder> {

    private List<String> items;
    private String searchText = "";

    public LogAdapter(List<String> items) {
        this.items = items;
    }

    @Override
    public ItemHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_log, parent, false);
        return new ItemHolder(view);
    }

    @Override
    public void onBindViewHolder(ItemHolder holder, int position) {
        holder.bindLog(items.get(position), searchText);
    }

    @Override
    public int getItemCount() {
        return items.size();
    }

    void refreshItems(List<String> items) {
        this.searchText = "";
        this.items = items;
        notifyDataSetChanged();
    }

    void searchText(String searchText) {
        if (searchText.length() < 3 && this.searchText.length() < 3) {
            this.searchText = searchText;
            return;
        }

        this.searchText = searchText;
        notifyDataSetChanged();
    }

}
