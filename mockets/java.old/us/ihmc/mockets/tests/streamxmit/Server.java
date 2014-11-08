import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;

//import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.StreamServerMocket;
import us.ihmc.mockets.StreamMocket;

public class Server
{
    public static void main (String[] args)
    {
        Server s = new Server (9876);
        s.go();
    }

    public Server (int port)
    {
        _port = port;
    }

    public void go()
    {
        try {
            StreamServerMocket sm = new StreamServerMocket (_port);
            while (true) {
                StreamMocket m = sm.accept();
                ClientHandler ch = new ClientHandler (m);
                ch.start();
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

    class ClientHandler extends Thread
    {
        public ClientHandler (StreamMocket m)
        {
            _mocket = m;
        }

        public void run()
        {
            System.out.println ("ClientHandler thread started");
            try {
                InputStream is = _mocket.getInputStream();
                OutputStream os = _mocket.getOutputStream();
                while (true) {
                    int ch = is.read();
                    if (ch < 0) {
                        _mocket.close();
                        break;
                    }
                    int replyCh = Character.toUpperCase((char)ch);
                    os.write (replyCh);
                }
            }
            catch (IOException e) {
                e.printStackTrace();
            }
            System.out.println ("ClientHandler thread terminating");
        }

        private StreamMocket _mocket;
    }

    private int _port;
}
