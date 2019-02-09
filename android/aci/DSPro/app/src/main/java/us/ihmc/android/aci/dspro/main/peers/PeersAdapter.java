package us.ihmc.android.aci.dspro.main.peers;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import java.util.ArrayList;

import us.ihmc.android.aci.dspro.R;

/**
 * Created by nomads on 10/4/17.
 */

public class PeersAdapter extends ArrayAdapter<Peer> {

    private Context _context;

    public PeersAdapter(Context context, ArrayList<Peer> peers) {
        super(context, 0, peers);
        _context = context;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        Peer peer = getItem(position);
        if (convertView == null) {
            convertView = LayoutInflater.from(getContext()).inflate(R.layout.item_peers_info, parent, false);
        }
        TextView peerName = (TextView)convertView.findViewById(R.id.peer_address);
        TextView unicastMessages = (TextView)convertView.findViewById(R.id.peers_unicast_messages);
        TextView multicastMessages = (TextView)convertView.findViewById(R.id.peers_multicast_messages);
        peerName.setText(peer.getPeerName());
        unicastMessages.setText(String.valueOf(peer.getUnicastMessages()));
        multicastMessages.setText(String.valueOf(peer.getMulticastMessages()));
        /*
        ImageView iwUnicast = (ImageView) convertView.findViewById(R.id.iwUnicast);
        ImageView iwMulticast = (ImageView) convertView.findViewById(R.id.iwMulticast);
        if (peer.isUnicast())
            iwUnicast.setImageDrawable(_context.getResources().getDrawable(R.drawable.true_sign));
        else
            iwUnicast.setImageDrawable(_context.getResources().getDrawable(R.drawable.false_sign));
        if (peer.isMulticast())
            iwMulticast.setImageDrawable(_context.getResources().getDrawable(R.drawable.true_sign));
        else
            iwMulticast.setImageDrawable(_context.getResources().getDrawable(R.drawable.false_sign));
            */
       return convertView;
    }
}
