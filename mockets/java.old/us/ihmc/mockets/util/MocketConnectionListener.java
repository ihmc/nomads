package us.ihmc.mockets.util;

public interface MocketConnectionListener
{
    void connectionFailed (String clientIPAddr, String serverIPAddr, int serverPort);
    void connectionEstablished (String ipAddrOne, int portOne, String ipAddrTwo, int portTwo);
    void connectionLost (String ipAddrOne, int portOne, String ipAddrTwo, int portTwo);
    void connectionRestored (String ipAddrOne, int portOne, String ipAddrTwo, int portTwo);
    void connectionClosed (String ipAddrOne, int portOne, String ipAddrTwo, int portTwo);
}
