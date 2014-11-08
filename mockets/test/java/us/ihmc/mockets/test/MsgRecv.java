package us.ihmc.mockets.test;
import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.ServerMocket;

public class MsgRecv
{
    public static void main (String[] args) throws Exception
    {
    System.loadLibrary("mocketsjavawrapper");
        if (args.length != 0) {
            System.out.println ("usage: java MsgRecv");
            System.exit (1);
        }

        // Create ServerMocket and wait for connection
        System.out.println ("MsgRecv: creating ServerMocket");
        ServerMocket servMocket = new ServerMocket (LOCAL_PORT);
        
        byte[] msg = new byte [MAX_MESSAGE_SIZE];

        Mocket mocket = servMocket.accept();
        System.out.println ("MsgRecv: ServerMocket accepted a connection");

        while (true) {
            int msgSize = mocket.receive (msg);
            System.out.println ("MsgRecv: Receive returned " + msgSize);
            if (msgSize <= 0) {
                break;
            }
        }
        mocket.close();
    }

    private static final int LOCAL_PORT = 4000;
    private static final int MAX_MESSAGE_SIZE = 102400;
}
