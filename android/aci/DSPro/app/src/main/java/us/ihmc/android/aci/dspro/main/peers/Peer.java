package us.ihmc.android.aci.dspro.main.peers;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Created by Filippo Poltronieri on 10/4/17.
 * Filippo Poltronieri <fpoltronieri@ihmc.us>
 *
 */

public class Peer implements Parcelable {

    private String _peerName;
    private boolean _bUnicast;
    private boolean _bMulticast;
    private long _unicastMessages;
    private long _multicastMessages;

    public Peer (String peerName)
    {
        _peerName = peerName;
        _bUnicast = false;
        _bMulticast = false;
    }

    public Peer (String peerName, boolean unicast, boolean multicast)
    {
        _peerName  = peerName;
        _bUnicast = unicast;
        _bMulticast = multicast;
    }

    public Peer (Parcel parcel)
    {
        String[] data = new String[3];
        parcel.readStringArray(data);
        _peerName = data[0];
        _unicastMessages = Long.valueOf(data[1]);
        _multicastMessages = Long.valueOf(data[2]);

    }


    public boolean isMulticast() {
        return _bMulticast;
    }

    public void setMulticast(boolean bMulticast) {
        this._bMulticast = bMulticast;
    }

    public boolean isUnicast() {
        return _bUnicast;
    }

    public void setUnicast(boolean bUnicast) {
        this._bUnicast = bUnicast;
    }

    public String getPeerName() {
        return _peerName;
    }

    public void setPeerName(String peerName) {
        this._peerName = peerName;
    }

    public void incrementMulticastTraffic()
    {
        _multicastMessages += 1;
    }

    public void incrementUnicastTraffic()
    {
        _unicastMessages += 1;
    }

    public long getUnicastMessages()
    {
        return _unicastMessages;
    }

    public long getMulticastMessages()
    {
        return _multicastMessages;
    }

    public void setMulticastMessages(long multicastMessages) { _multicastMessages = multicastMessages; }

    public void setUnicastMessages(long unicastMessages) { _unicastMessages = unicastMessages; }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeStringArray(new String[] {this._peerName,
                String.valueOf(this._unicastMessages),
                String.valueOf(this._multicastMessages)});
    }

    public static final Parcelable.Creator CREATOR = new Parcelable.Creator() {
        public Peer createFromParcel(Parcel parcel) {
            return new Peer(parcel);
        }

        public Peer[] newArray(int size) {
            return new Peer[size];
        }
    };

    public String toString()
    {
        return _peerName + " " + "Unicast msg sent: " + _unicastMessages + " Multicast msg sent: " + _multicastMessages;
    }

}
