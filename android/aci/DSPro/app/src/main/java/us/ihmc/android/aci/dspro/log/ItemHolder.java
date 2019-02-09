package us.ihmc.android.aci.dspro.log;

import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.TextView;

import us.ihmc.android.aci.dspro.R;

/**
 * Holds log and pref item in RecyclerView.
 * Created on 5/14/18
 *
 * @author Kristyna Rehovicova (krehovicova@ihmc.us).
 */

public class ItemHolder extends RecyclerView.ViewHolder {

    public ItemHolder(View itemView) {
        super(itemView);
    }

    void bindLog(String text, String searchText) {
        TextView item = itemView.findViewById(R.id.log);
        item.setText(text);

        if (searchText.length() > 2 && text.toLowerCase().contains(searchText.toLowerCase()))
            item.setBackgroundResource(R.drawable.shape_item_highlighted);
        else
            item.setBackground(null);
    }

    public void bindPref(String text) {
        TextView item = itemView.findViewById(R.id.log);
        item.setText(text);
    }

}
