package us.ihmc.aci.util.dspro.test;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Reader;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.aci.util.dspro.XMLMetadataParser;

/**
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class TCPProxyServer implements Runnable
{
    class ConnHandler extends Thread
    {
        private Socket _sock;
        private ObjectInputStream _in;
        private ObjectOutputStream _out;
        private final TCPProxyServer _svr;
        private final boolean _bReceived;
        private final String _host;
        private final int _port;
        private final LinkedBlockingDeque<Message> _enqueuedMessages  = new LinkedBlockingDeque<Message>();

        ConnHandler (Socket sock, TCPProxyServer svr, boolean bReceived)
        {
            _sock = sock;
            setName (_sock.getRemoteSocketAddress().toString());
            String addr = _sock.getRemoteSocketAddress().toString();
            addr = addr.replace('/', ' ').trim();
            String[] tokens = addr.split(":");
            _host = tokens[0];
            _port = Integer.parseInt (tokens[1]);
            _svr = svr;
            _bReceived = bReceived;
        }

        void init() throws IOException
        {
            init (_sock);
        }

        synchronized void init (Socket sock) throws IOException
        {
            _sock = sock;
            _out = new ObjectOutputStream (_sock.getOutputStream());
            _out.flush();
            _in = new ObjectInputStream (_sock.getInputStream());
        }

        void forward (Message msg) throws IOException
        {
            _enqueuedMessages.addFirst (msg);
        }

        private synchronized void sendMessage (Message msg) throws IOException
        {
            _out.writeObject (msg);
        }

        @Override
        public void run ()
        {
            // Sender thread
            new Thread () {
                public void run ()
                {
                    while (true) {
                        Message msg = null;
                        try {
                            msg = _enqueuedMessages.pollFirst (Long.MAX_VALUE, TimeUnit.DAYS);
                            if (msg != null) {
                                sendMessage (msg);
                            }
                        }
                        catch (InterruptedException ex) {}
                        catch (IOException ex) {
                            _enqueuedMessages.addFirst (msg);
                            try { Thread.sleep (500); }
                            catch (InterruptedException ex1) {}
                        }
                    }
                }
            }.start();
    
            // Receiver thread
            while (true) {
                try {
                    Object obj = _in.readObject();
                    if (obj instanceof Message) {
                        Message msg = (Message) obj;
                        _svr.newMessage(getId(), getName(), msg);
                    }
                }
                catch (Exception ex) {
                    // Logger.getLogger(TCPProxyServer.class.getName()).log(Level.SEVERE, null, ex);
                    System.out.println ("Handler to " + _sock.getRemoteSocketAddress() + " disconnected");
                    if (!_bReceived) {
                        try { _sock.close(); }
                        catch (IOException ex1) {}
                        while (true) {
                            try {
                                System.out.println ("Trying to re-connect to " + _sock.getRemoteSocketAddress());
                                init (new Socket (_host, _port));
                                break;
                            }
                            catch (IOException ex1) {
                                // Logger.getLogger(TCPProxyServer.class.getName()).log(Level.SEVERE, null, ex1);
                                try { Thread.sleep(2000); }
                                catch (InterruptedException ex2) {}
                            }
                        }
                    }
                    break;
                }
            }
        }

        void enqueueMessages (Collection<Message> enqueuedMessages) {
            if (enqueuedMessages != null) {
                System.out.println ("Enqueued " + enqueuedMessages.size() + " messages");
                _enqueuedMessages.addAll (enqueuedMessages);
            }
        }
    }

    private final ServerSocket _socket;
    private final List<ConnHandler> _handlers = new LinkedList<ConnHandler>();
    private final Map<String, Queue<Message>> _enqueuedMessagesByHost = new HashMap<String, Queue<Message>>();
    public static final int TCP_PROXY_SERVER_PORT_NUMBER = 56487;

    public TCPProxyServer (int rcPort) throws IOException
    {
        _socket = new ServerSocket (rcPort);
    }

    public TCPProxyServer() throws IOException
    {
        this (TCP_PROXY_SERVER_PORT_NUMBER);
    }

    public void run()
    {
        System.out.println ("Running TCPProxyServer on port " + _socket.getLocalPort());
        while (true) {
            try {
                createAndAddHandler (_socket.accept(), true);
            }
            catch (IOException ex) {
                Logger.getLogger(TCPProxyServer.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }

    void connectTo (String host, int port) throws IOException
    {
        System.out.println ("Connecting to " + host + ":" + port);
        createAndAddHandler (new Socket (host, port), false);
    }

    private void createAndAddHandler (Socket sock, boolean bReceived) throws IOException
    {
        ConnHandler newHandler = new ConnHandler (sock, this, bReceived);
        synchronized (_handlers) {
            boolean bFound = false;
            for (ConnHandler handler : _handlers) {
                if (handler.getName().compareTo (newHandler.getName()) == 0) {
                    handler.init (sock);
                    bFound = true;
                    System.out.println ("Re-connected to " + handler.getName());
                }
            }
            if (!bFound) {
                newHandler.init();
                newHandler.start();
                _handlers.add (newHandler);
                System.out.println ("Connected to " + newHandler.getName());
            }            
        }
    }
 
    void newMessage (long handlerId, String handlerName, Message msg)
    {
        System.out.println ("Recevied message from " + handlerName + ": " + msg._id + " " + msg._groupName + " " + msg._objectId + " "
                            + msg._instanceId + "\n" + msg._xmlMedatada + " There are " + _handlers.size() + " handlers");

        synchronized (_handlers) {
            for (ConnHandler handler : _handlers) {
                if (handler.getId() != handlerId) {
                    try {
                        System.out.println ("Forwarding message from " + handlerName + " to " + handler.getName());
                        handler.forward (msg);
                    }
                    catch (IOException ex) {
                        Logger.getLogger(TCPProxyServer.class.getName()).log(Level.SEVERE, null, ex);
                    }
                }
            }

            for (Queue<Message> enqueuedMessages : _enqueuedMessagesByHost.values()) {
                enqueuedMessages.add (msg);
            }
        }

    }

    public static HashMap<String, Object> parse (Reader reader) throws IOException
    {
        BufferedReader br = new BufferedReader(reader);
        String line, key, value;
        String[] tokens;
        HashMap<String, Object> properties = new HashMap<String, Object>();

        while ((line = br.readLine()) != null) {
            if (line.length() == 0) {
                continue;
            }
            tokens = line.split (":");
            key = tokens[0].trim();
            value = tokens[1].trim();

            Object o;
            try {
                switch (MetadataElement.valueOf(key)) {
                    case Left_Upper_Latitude:
                    case Right_Lower_Latitude:
                    case Left_Upper_Longitude:
                    case Right_Lower_Longitude: {
                        o = new Float (new Double ((Double.parseDouble(value))).floatValue());
                        break;
                    }
                    default: {
                        o = value;
                    }
                }
                properties.put(key, o);
            }
            catch (IllegalArgumentException ex) {
                // Perform coordinates to bounding box conversion (dspro likes
                // left upper/right lower bounding box...)
                if (key.equalsIgnoreCase("Latitude")) {
                    double lat = Double.parseDouble (value);
                    properties.put (MetadataElement.Left_Upper_Latitude.toString(), new Float(lat + 0.00001));
                    properties.put (MetadataElement.Right_Lower_Latitude.toString(), new Float (lat - 0.00001));
                }
                else if(key.equalsIgnoreCase("Longitude")) {
                    double lon = Double.parseDouble (value);
                    properties.put (MetadataElement.Left_Upper_Longitude.toString(), new Float(lon - 0.00001));
                    properties.put (MetadataElement.Right_Lower_Longitude.toString(), new Float(lon + 0.00001));
                }
                else {
                    // Unknown property
                    System.err.println("Metadata does not have any attribute named " + key);
                }
            }
        }

        return properties;
    }

    public static void main (String args[]) throws IOException
    {
        int port = 56487;
        boolean bSendMessage = false;
        List<String> forwardingSockets = new ArrayList<String>();
        for (int i = 0; i < args.length; i++) {
            if (args[i].compareTo("-port") == 0) {
                port = Integer.parseInt(args[++i]);
            }
            if (args[i].compareTo("-forwardTo") == 0) {
                String[] fwdSocks = args[++i].split(",");
                for (int j = 0; j < fwdSocks.length; j++) {
                    forwardingSockets.add (fwdSocks[j]);
                }
            }
            if (args[i].compareTo("-sendMessage") == 0) {
                bSendMessage = true;
            }
        }
        final TCPProxyServer srv = new TCPProxyServer (port);
        for (String fwdSocket : forwardingSockets) {
            String[] info = fwdSocket.split(":");
            String forwardingHost = info[0];
            int forwardingPort = port;
            if (info.length > 1) {
                forwardingPort = Integer.parseInt (info[1]);
            }
            if (forwardingHost != null) {
                srv.connectTo (forwardingHost, forwardingPort);
            }
        }

        new Thread (srv).start();

        if (bSendMessage) {
            new Thread () {
                @Override
                public void run()
                {
                    int i = 0;
                    while (true) {
                        String id = Integer.toString(i++);
                        String groupName = "grp";
                        String objectId = "objId";
                        String instanceId = "instId";
                        String xmlMedatada = "<xml>\n\t<Metadata>\n\t</Metadata>\n</xml>";
                        byte[] data = null;
                        long expirationTime = 0;
                        Message msg = new Message (id, groupName, objectId, instanceId, xmlMedatada, data, expirationTime);
                        srv.newMessage(-1, "fake", msg);

                        try {
                            Thread.sleep (1000);
                        }
                        catch (InterruptedException ex) {
                            Logger.getLogger(TCPProxyServer.class.getName()).log(Level.SEVERE, null, ex);
                        }
                    }
                }
            }.start();
        }

        final BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        int i = 0;
        for (String input; (input = br.readLine()) != null;) {
            try {
                final String[] inputTokens = input.split ("\\s+");
                final String cmd = inputTokens[0];
                if (("quit".compareToIgnoreCase (cmd) == 0) || ("exit".compareToIgnoreCase (cmd) == 0)) {
                    break;
                }
                else if ("add".compareToIgnoreCase (cmd) == 0) {
                    final String pathToData = inputTokens[1];
                    final String pathToMetadata = pathToData + ".dpmd";

                    final String id = Integer.toString(i++);
                    final String groupName = "grp";
                    final String objectId = "sigActObjId";
                    final String instanceId = "instId";
                    final HashMap metadata = parse (new FileReader (pathToMetadata));
                    final String xmlMedatada = XMLMetadataParser.toXML(metadata);
                    final byte[] data = Files.readAllBytes(Paths.get(pathToData));
                    final long expirationTime = 0;
                    final Message msg = new Message (id, groupName, objectId, instanceId, xmlMedatada, data, expirationTime);
                    srv.newMessage(-1, "fake", msg);
                }
            }
            catch(IOException io){
                Logger.getLogger(TCPProxyServer.class.getName()).log(Level.SEVERE, null, io);
            }
	}
    }
}
