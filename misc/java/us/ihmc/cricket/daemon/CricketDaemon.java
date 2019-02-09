package cricketdaemon;

import cricketdaemon.clientlib.data.CricketData;
//import cricketdaemon.clientlib.parse.*;
//import cricketdaemon.clientlib.sim.*;
import cricketdaemon.position.PositionData;
import cricketdaemon.position.PositionSolver;
//import cricketdaemon.reader.*;
//import cricketdaemon.reader.sim.*;
import cricketdaemon.server.*;
//import cricketdaemon.Util;
import java.util.*;
import java.net.*;
import java.io.*;

public class CricketDaemon
{
    /*public static final boolean debugMode = false;
    public static final int MIN_DB = 100;
    public static final int MAX_DB = 200;
    private double x[] = {101, 108, 28, 61, 40};
    private double y[] = {112, 229, 228, 170, 110};
    private double z[] = {0, 0, 0, 0, 0};
    private String space[] = {"Beacon1", "Beacon2", "Beacon3", "Beacon5", "Beacon4"};*/

    public String filePath = "/home/pim/Desktop/Cricket2.0/src/cricketdaemon/beacons.conf";
    private int x[] = null;
    private int y[] = null;
    private int z[] = null;
    private String space[] = null;
    public boolean debugMode = false;
    public int MIN_DB=0;
    public int MAX_DB=0;
    public int sockport=0;    // Port where the position updater is listening
    public String address = null;

    public static final double K_NOMINAL = UnitConv.cricketDistToCM(1);
    public static final String REGISTER_TOKEN = ServerThread.REGISTER_TOKEN;
    public static final int PORT = 2947;
    public static final int TIMEOUT = 1000;
    public static final long SLEEP_TIME = 100;
    public static final int MAX_MESSAGE_SIZE = 100;

    private InputStreamReader netInReader;
    private OutputStreamWriter netOutWriter;
	private CricketData curData = null;
    private CricketData cricketDataArray[];
    private BeaconInfo beaconInfo;
    private PositionSolver positionSolver;
    
    // Socket to send the position to the position server
    private DatagramSocket dsock;

    public CricketDaemon()
    {
        readConfigFile();
        if (connect() != 0) {
    	    System.out.println("[CricketDaemon]: connect() returned with an exception; terminating...");
            System.exit(-1);
        }

        // Create the datagram socket
        try {
            dsock = new DatagramSocket();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
        
        int count = 0;
        char message[] = new char[MAX_MESSAGE_SIZE];
        beaconInfo = new BeaconInfo();
        positionSolver = new PositionSolver(K_NOMINAL, 10);
        
        while (true) {
            try {
		        while(!netInReader.ready()) {
                    if (debugMode) {
                        System.out.println("[CricketDaemon]: waiting for data...");
                    }
                    try {
                        Thread.sleep(SLEEP_TIME);
                    }
                    catch(InterruptedException e) { }
                }

                int temp = netInReader.read();

                if(temp == 13) {
                    if (count < 60) {
                        if(debugMode) {
                            System.out.print("[CricketDaemon]: message too short: discarded; was: ");
                        }
                    }
                    else {
                        processMessage(message);                        
                    }
                }
                else if (temp != -1) {
                    if ((char)temp != '\n') {
                        message[count] = (char)temp;
                        count++;
                    }

                    if (count < MAX_MESSAGE_SIZE - 1) {
                        continue;
                    }
                    else {
                        if (debugMode) {
                            System.out.print("[CricketDaemon]: message too long: discarded; was: ");
                        }
                    }
                }
                if (debugMode) {
                    System.out.println(message);
                }
                message = new char[MAX_MESSAGE_SIZE];
                count = 0;
            }
            catch (IOException e) {
        		System.out.println("[CricketDaemon]: IOException: " + e + "; terminating");
                System.exit(-4);
    	    }
        }
    }

    private int connect()
    {
	    try {
            Socket socket = new Socket(InetAddress.getByName("localhost"), PORT);
	        socket.setTcpNoDelay(true);
	        socket.setSoTimeout(TIMEOUT);

            netInReader = new InputStreamReader(socket.getInputStream());
	        netOutWriter = new OutputStreamWriter(socket.getOutputStream(), ServerModule.DEFAULT_ENCODING);

    	    netOutWriter.write(REGISTER_TOKEN + "\n");
	        netOutWriter.flush();
        }
        catch (IOException e) {
    	    System.out.println("[CricketDaemon.connect]: IOException: " + e.toString());
    	    return -1;
    	}
        return 0;
    }

    private void readConfigFile()
    {
        try {
            BufferedReader reader = new BufferedReader (new FileReader(filePath));
            String str = null;
            int i=0;
            while ((str = reader.readLine()) != null) {
                //System.out.println ("**line: " + str);
                int index = str.indexOf ("=");
                int numOfBeacons = 0;
                if (str.indexOf ("NumberOfB") >= 0) {
                    numOfBeacons = Integer.parseInt (str.substring (index + 1, str.length()));
                    x = new int [numOfBeacons];                    //{101, 108, 28, 61, 40};
                    y = new int [numOfBeacons];                    //{112, 229, 228, 170, 110};
                    z = new int [numOfBeacons];                  //{0, 0, 0, 0, 0};
                    space= new String [numOfBeacons];
                }
                if (str.indexOf ("Beacon") >= 0) {
                    String name = str.substring (0, index);
                    space [i] = name;
                    String value = str.substring (index + 1, str.length());
                    Vector v = getVector (value);
                    x [i] = Integer.parseInt (v.get(1).toString());
                    y [i] = Integer.parseInt (v.get(2).toString());
                    z [i] = Integer.parseInt (v.get(3).toString());
                    i++;
                }
                else if (str.indexOf ("DEBUG") >= 0) {
                    //System.out.println ("**line DEBUG: " + str.substring (index + 1, str.length()));
                    debugMode = Boolean.parseBoolean (str.substring (index + 1, str.length())) ;
                    //System.out.println ("**line DEBUG: " + DEBUG);
                }
                else if (str.indexOf ("MIN_DB") >= 0) {
                    //System.out.println ("**line MIN_DB: " + str.substring (index + 1, str.length()));
                    MIN_DB = Integer.parseInt (str.substring (index + 1, str.length()));
                }
                else if (str.indexOf ("MAX_DB") >= 0) {
                    MAX_DB = Integer.parseInt (str.substring (index + 1, str.length()));
                }
                else if (str.indexOf ("SockPort") >= 0) {
                    System.out.println ("**SockPort: " + str.substring (index + 1, str.length()));
                    sockport = Integer.parseInt (str.substring (index + 1, str.length()));
                    System.out.println ("**sockport: " + sockport);
                }
                else if (str.indexOf ("Address") >= 0) {
                    address = str.substring (index + 1, str.length());
                }
            }
            /*for (int j=0; j<x.length; j++) {
                System.out.println ("**Arrayx: " + x[j]);
            }
            for (int j2=0; j2<y.length; j2++) {
                System.out.println ("**Arrayy: " + y[j2]);
            }
            for (int k=0; k<space.length; k++) {
                System.out.println ("**space: " + space[k]);
            }*/

            reader.close();
        }
        catch (FileNotFoundException e) {
            System.out.println ("Warning; The file " + filePath + " is not found");
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

    public Vector getVector (String str)
    {
        Vector v = new Vector();
        StringTokenizer st = new StringTokenizer (str, " ");
        while (st.hasMoreTokens()) {
            String value = st.nextToken();
            v.addElement (value);
        }
        //System.out.println ("**vector" + v);
        return v;
    }

    private void processMessage(char[] message)
    {
        StringTokenizer st1 = new StringTokenizer(String.valueOf(message), "=");
        st1.nextToken();
        st1.nextToken();
        st1.nextToken();
        StringTokenizer st2 = new StringTokenizer(st1.nextToken(), ",");
        String spaceID = st2.nextToken();

        if (debugMode) {
            System.out.println("[CricketDaemon.processMessage]: spaceID: " + spaceID);
        }
        st2 = new StringTokenizer(st1.nextToken(), ",");
        String temp = st2.nextToken();
        if (debugMode) {
            System.out.println("[CricketDaemon.processMessage]: temp: " + temp);
        }
        int distance = Integer.parseInt(temp);//st2.nextToken());
        if (debugMode) {
            System.out.println("[CricketDaemon.processMessage]: distance: " + distance);
        }
        if((distance < MIN_DB) || (distance > MAX_DB)) {
            if (debugMode) {
                System.out.println("[CricketDaemon.processMessage]: wrong distance; discarding message");
            }
        }
        else {
            boolean complete = true;
            if (debugMode) {
                System.out.println("[CricketDaemon.processMessage]: saving data");
            }
            int index = getIndexOf(spaceID);
            if(index < 0) {
                if (debugMode) {
                    System.out.println("[CricketDaemon.processMessage]: unknown beacon detected; terminating");
                }
                System.exit(-5);
            }            
            int readyToCalcPos = beaconInfo.add(this.x[index], this.y[index], this.z[index], spaceID, 0, distance);

            if(readyToCalcPos == 1) {
                PositionData positionData = positionSolver.positionEstimateVKnown (null, beaconInfo.x, beaconInfo.y, beaconInfo.z, beaconInfo.space, beaconInfo.id, beaconInfo.dist);
                if (debugMode) {
                    System.out.println("------------------------------------------------------------------------------------------------------");
                }
                try {
                    // Create the string with the position to send to the Position Updater
                    // In the string we have: "( "+x+" "+y+" "+z+" )"
                    String data = new String (positionData.getCoord().toString());
                    //System.out.println ("[CricketDaemon.processMessage]: data String: " + data);
                    // Create the byte array
                    byte[] buf = data.getBytes();
                    // Prepare the packet to send the position
                    DatagramPacket packet = new DatagramPacket (buf, buf.length, InetAddress.getLocalHost(), sockport);
                    // Send the position to a local DatagramSocket
                    dsock.send (packet);
		    //System.out.println ("CricketDaemon.processMessage: sent a packet to " + InetAddress.getLocalHost() + ":" + sockport);
                    
                    System.out.println ("[CricketDaemon.processMessage]: Position: " + positionData.toString());
                }
                catch(NullPointerException npe) {
                    if (debugMode) {
                        System.out.println("[CricketDaemon.processMessage]: imaginary Z; discarding output");
                    }
                }
                catch (IOException ioe) {
                    ioe.printStackTrace();
                }
                if (debugMode) {
                    System.out.println("------------------------------------------------------------------------------------------------------");
                    System.out.println("[CricketDaemon.processMessage]: erasing old positions");
                }
                beaconInfo = new BeaconInfo();
            }
        }
    }

    public int getIndexOf (String spaceID)
    {
        for(int i = 0;i < this.space.length;i++)
        {
            if(this.space[i].compareTo(spaceID) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    public static void main(String args[])
    {
        new CricketDaemon();
    }

    private class BeaconInfo
    {
        public double x[];
        public double y[];
        public double z[];
        public String space[];
        public int id[];
        public double dist[];

        public BeaconInfo()
        {
            x = new double[3];
            y = new double[3];
            z = new double[3];
            space = new String[3];
            id = new int[3];
            dist = new double[3];
        }
        public int add(double x, double y, double z, String space, int id, double dist)
        {
            for(int i = 0; i < 3;i++)
            {
                if(debugMode)
                    System.out.println("[BeaconInfo.addBeacon]: inside the for-loop; i: " + i);
                if (this.space[i] == null)
                {
                    if(debugMode)
                        System.out.println("[BeaconInfo.addBeacon]: inserting new value in pos: " + i);
                    this.x[i] = x;
                    this.y[i] = y;
                    this.z[i] = z;
                    this.space[i] = space;
                    this.id[i] = id;
                    this.dist[i] = dist;
                    if(debugMode)
                    {
                        System.out.println("[BeaconInfo.addBeacon]: beaconInfo[" + i + "].spaceID: " + this.space[i]);
                        System.out.println("[BeaconInfo.addBeacon]: beaconInfo[" + i + "].distance: " + this.dist[i]);        
                    }
                    if(i == 2)
                    {
                        if(debugMode)                        
                            System.out.println("[BeaconInfo.addBeacon]: the array is full");
                        return 1;
                    }
                    return 0;
                }                
                else if(this.space[i].compareTo(space) == 0)
                {
                    if(debugMode)                    
                    {                    
                        System.out.println("[BeaconInfo.addBeacon]: beaconInfo[" + i + "].spaceID: " + this.space[i]);
                        System.out.println("[BeaconInfo.addBeacon]: beaconInfo[" + i + "].distance: " + this.dist[i]);
                    }                    
                    this.x[i] = x;
                    this.y[i] = y;
                    this.z[i] = z;
                    this.id[i] = id;
                    this.dist[i] = dist;
                    if(debugMode)                    
                    {
                        System.out.println("[BeaconInfo.addBeacon]: updating...");
                        System.out.println("[BeaconInfo.addBeacon]: beaconInfo[" + i + "].distance: " + this.dist[i]);
                    }
                    return 0;
                }
            }
            if(debugMode)
                System.out.println("[BeaconInfo.addBeacon]: outside the for-loop");
            return -1;
        }
    }
}

