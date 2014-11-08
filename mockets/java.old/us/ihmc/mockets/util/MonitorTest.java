package us.ihmc.mockets.util;

public class MonitorTest implements MocketConnectionListener
{
    public static void main (String[] args)
    {
        MocketStatusMonitor msm = new MocketStatusMonitor();
        msm.setMocketConnectionListener (new MonitorTest());
        msm.setDaemon (false);
        msm.start();
    }
    
    public void connectionFailed (String clientIPAddr, String serverIPAddr, int serverPort)
    {
        System.out.println ("Connection failed from " + clientIPAddr  + " to " + serverIPAddr + ":" + serverPort);
    }

    public void connectionClosed (String ipAddrOne, int portOne, String ipAddrTwo, int portTwo)
    {
        System.out.println ("Connection closed between " + ipAddrOne + ":" + portOne + " and " + ipAddrTwo + ":" + portTwo);
    }
    
    public void connectionEstablished (String ipAddrOne, int portOne, String ipAddrTwo, int portTwo)
    {
        System.out.println ("Connection established between " + ipAddrOne + ":" + portOne + " and " + ipAddrTwo + ":" + portTwo);
    }

    public void connectionLost (String ipAddrOne, int portOne, String ipAddrTwo, int portTwo)
    {
        System.out.println ("Connection lost between " + ipAddrOne + ":" + portOne + " and " + ipAddrTwo + ":" + portTwo);
    }

    public void connectionRestored(String ipAddrOne, int portOne, String ipAddrTwo, int portTwo)
    {
        System.out.println ("Connection restored between " + ipAddrOne + ":" + portOne + " and " + ipAddrTwo + ":" + portTwo);
    }    
}
