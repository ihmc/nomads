using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Net.Sockets;
using System.Net;
using System.IO;

using us.ihmc.mockets;

namespace IntDataTest
{
    class Util
    {
        public static long currentTimeMillis()
        {
            return DateTime.Now.Ticks / (long)10e3;
        }

        public static uint from4BytesToUnsignedInt(byte[] buffer, int offset)
        {
            uint result = 0;
            result = buffer[offset + 0];
            result = (result << 8) | buffer[offset + 1];
            result = (result << 8) | buffer[offset + 2];
            result = (result << 8) | buffer[offset + 3];

            return result;
        }

        public static void fromUnsignedIntTo4Bytes(uint value, byte[] buffer, int offset)
        {
            buffer[offset + 0]   = (byte) ((value >> 24) & 0xFF);
            buffer[offset + 1] = (byte) ((value >> 16) & 0xFF);
            buffer[offset + 2] = (byte) ((value >> 8) & 0xFF);
            buffer[offset + 3] = (byte) (value & 0xFF);
        }

        public static void saveStats(ManagedMessageMocket mocket, String type, long txTime)
        {
            String fname = "stats-" + type + "-mockets-cs.txt";
            FileStream file = new FileStream(fname, FileMode.OpenOrCreate, FileAccess.Write);
            StreamWriter sw = new StreamWriter(file);
            sw.WriteLine("[{0}]\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}",
                Util.currentTimeMillis(),
                txTime,
                mocket.getStatistics().getSentPacketCount(),
                mocket.getStatistics().getSentPacketCount(),
                mocket.getStatistics().getSentByteCount(),
                mocket.getStatistics().getReceivedPacketCount(),
                mocket.getStatistics().getReceivedByteCount(),
                mocket.getStatistics().getRetransmitCount(),
                mocket.getStatistics().getDuplicatedDiscardedPacketCount() + mocket.getStatistics().getNoRoomDiscardedPacketCount()
                );

            sw.Flush();
            sw.Close();
            file.Close();
        }

        public static void saveStats(Socket socket, String type, long txTime)
        {
            String fname = "stats-" + type + "-sockets-cs.txt";
            FileStream file = new FileStream(fname, FileMode.OpenOrCreate, FileAccess.Write);
            StreamWriter sw = new StreamWriter(file);
            sw.WriteLine("[{0}]\t{1}",
                Util.currentTimeMillis(),
                txTime
                );

            sw.Flush();
            sw.Close();
            file.Close();
        }

        //public static void saveStats(Socket socket, String type, long txTime)
        public static void saveStats(TcpClient tcpClient, String type, long txTime)
        {
            String fname = "stats-" + type + "-sockets-cs.txt";
            FileStream file = new FileStream(fname, FileMode.OpenOrCreate, FileAccess.Write);
            StreamWriter sw = new StreamWriter(file);
            sw.WriteLine("[{0}]\t{1}",
                Util.currentTimeMillis(),
                txTime
                );

            sw.Flush();
            sw.Close();
            file.Close();
        }

        public static void reportError(String type, String errMSG)
        {
            String fname = "stats-" + type + "-mockets-cs.txt";
            FileStream file = new FileStream(fname, FileMode.OpenOrCreate, FileAccess.Write);
            StreamWriter sw = new StreamWriter(file);
            sw.WriteLine("[{0}]\t{1}",
                Util.currentTimeMillis(),
                errMSG
                );

            sw.Flush();
            sw.Close();
            file.Close();
        }
    } // class Util
    
    class Stats
    {
        public void update(double value)
        {
            _sumValues += value;
            _sumSqValues += (value * value);
            _totalNumValues++;
        }

        public void reset()
        {
            _sumValues = 0;
            _sumSqValues = 0;
            _totalNumValues = 0;
        }

        public int getNumValues()
        {
            return _totalNumValues;
        }

        public double getAverage()
        {
            return (_sumValues/_totalNumValues);
        }

        public double getStDev()
        {
            double avg = getAverage();
            double aux = (_totalNumValues * avg * avg)
                          - (2 * avg * _sumValues)
                          + _sumSqValues;
            aux = (double) aux / (_totalNumValues - 1);
            aux = Math.Sqrt (aux);
            return aux;
        }


        private double _sumValues   = 0.0;
        private double _sumSqValues = 0.0;
        private int _totalNumValues = 0;
    } //class Stats

    
    class ClientConnHandler
    {
        public ClientConnHandler (ManagedMessageMocket msgMocket)
        {
            _mocket = msgMocket;
            PeerUnreachableWarningCallback callback = new PeerUnreachableWarningCallback(this.peerUnreachableCallback);
            _mocket.registerPeerUnreachableWarningCallback(callback);
            _useMockets = true;
        }
/*
        public ClientConnHandler (Socket socket)
        {
            _socket = socket;
            _useMockets = false;
        }
*/
        public ClientConnHandler(TcpClient tcpClient)
        {
            _tcpClient = tcpClient;
            _useMockets = false;
        }

        public void run()
        {
            Console.WriteLine("ConnectionHandler: client handler thread started");
            long startTime = Util.currentTimeMillis();
            byte[] buffer = new byte[IntDataTest.BUFFER_SIZE];
            byte[] chArray = new byte[] {(byte)'.'};
            uint totalToRead = 0;
            uint totalRead = 0;

            if (_useMockets) {
                _mocket.receive(buffer, 4, 0);

                totalToRead = Util.from4BytesToUnsignedInt(buffer, 0);
                Console.WriteLine("IntDataTest:Server >> will read a total of [" + totalToRead + "] bytes. Remote port  = " + _mocket.getRemotePort());                
                while ((totalRead < totalToRead) && !_closeConn) {
                    //System.out.println(" will try to read.");
                    uint numRead = (uint)_mocket.receive (buffer, (uint)buffer.Length);
                    if (numRead < 0) {
                        Console.WriteLine("error reading????");
                    }
                    totalRead += numRead;
                    //Console.WriteLine("read so far:: {0}", totalRead);
                }
                
                Console.WriteLine("received " + totalRead + " bytes.");
            }
            else {
                NetworkStream stream = _tcpClient.GetStream();
                Console.WriteLine ("before socket receive....");
                //_socket.Receive (buffer);
                stream.Read(buffer, 0, 4);
                Console.WriteLine ("after socket receive...");
                totalToRead = Util.from4BytesToUnsignedInt (buffer, 0);
                Console.WriteLine("IntDataTest:ServerSocket >> will read a total of [" + totalToRead + "] bytes. ");
                while ((totalRead < totalToRead)) {
                    //System.out.println(" will try to read.");
                    //uint numRead = (uint) _socket.Receive (buffer);
                    uint numRead = (uint)stream.Read(buffer, 0, buffer.Length);
                    if (numRead < 0) {
                        Console.WriteLine ("error reading????");
                    }
                    //Console.WriteLine("just read:: {0}", numRead);
                    totalRead += numRead;
                    //Console.WriteLine("socket read so far:: {0}", totalRead);
                }
            }

            long totalTime = Util.currentTimeMillis() - startTime;
            Console.WriteLine("Socket Done reading");

            if (_closeConn) {
                Console.WriteLine("Connection reset.");
                Util.reportError ("server", "Connection Reset. 1");
                return;
            }
            if (_useMockets) {
                ManagedMessageSender sender = _mocket.getSender (true, true);
                sender.send (chArray, (uint)chArray.Length);
                Console.WriteLine ("IntDataTest: Sending ACK!");
                _mocket.close();
                Util.saveStats (_mocket, "MessageServer", totalTime);
            }
            else {
                Console.WriteLine ("IntDataTest :: sending back the ACK [" + totalRead + "]");
                //_socket.Send (chArray);                
                NetworkStream stream = _tcpClient.GetStream();
                stream.Write(chArray, 0, 1);
                stream.Flush();

                Console.WriteLine("IntDataTest: Socket Sending ACK!");
                //_socket.Close();
                _tcpClient.Close();
                Util.saveStats (_socket, "SocketServer", totalTime);
            }
        }

        public void Start()
        {
            Thread t = new Thread(new ThreadStart(run));
            t.Start();
        }

        public Boolean peerUnreachableCallback(uint timeSinceLastContact)
        {
            Console.WriteLine("\nIntDataTest :: PeerUnreachableWarning called :: {0}", timeSinceLastContact);
            if (timeSinceLastContact > 15000) 
            {
                _closeConn = true;
                return true;
            }
            return false;
        }

        private Boolean _useMockets;
        private ManagedMessageMocket _mocket;
        private Socket _socket;
        private TcpClient _tcpClient;
        private bool _closeConn = false;
    }

    class IntDataTest
    {
        public void runServerMocket()
        {
            ManagedMessageServerMocket serverMocket = new ManagedMessageServerMocket();
            serverMocket.listen(_portNum);

            while (true) {
                Console.WriteLine("serverMocket:: waiting for connections...");
                ManagedMessageMocket mocket = serverMocket.accept();
                Console.WriteLine("serverMocket:: connection Accepted.");
                ClientConnHandler connHandler = new ClientConnHandler(mocket);
                connHandler.Start();
            }
        }

        public void runServerSocket()
        {
            IPAddress bindIP = IPAddress.Any;
            TcpListener serverSocket = new TcpListener(bindIP, _portNum);
            serverSocket.Start();
            while (true) {
                //Socket socket = serverSocket.AcceptSocket();
                //ClientConnHandler connHandler = new ClientConnHandler(socket);
                TcpClient client = serverSocket.AcceptTcpClient();
                ClientConnHandler connHandler = new ClientConnHandler(client);

                connHandler.Start();
            }
        }

        public void runServers(ushort portNumber)
        {
            _portNum = portNumber;
            
            Thread serverMocketThread = new Thread(new ThreadStart(runServerMocket));
            serverMocketThread.Name = "ServerMocketThread";

            Thread serverSocketThread = new Thread(new ThreadStart(runServerSocket));
            serverSocketThread.Name = "ServerSocketThread";

            serverMocketThread.Start();
            serverSocketThread.Start();
        }

        public Boolean peerUnreachableCallback(uint timeSinceLastContact)
        {
            Console.WriteLine("\nIntDataTest :: PeerUnreachableWarning(2) called :: {0}", timeSinceLastContact);
            return (timeSinceLastContact > 15000);
        }

        public int doClientTask (string remoteHost, ushort remotePort, bool useMockets, Stats stats)
        {
            int rc;
            byte[] buf = new byte[1024];

            if (useMockets) {
                Console.WriteLine ("doClientTask: Using MessageMockets");
                ManagedMessageMocket mocket = new ManagedMessageMocket();
                //mocket.bind("127.0.0.1", 6789);

                Console.WriteLine ("doClientTask: MessageMockets: Before connect\n");
                rc = mocket.connect(remoteHost, remotePort);
                if (0 != rc) {
                    Console.WriteLine("doClientTask: failed to connect using MessageMockets to remote host {0} on port {1}; rc = {2}",
                             remoteHost, remotePort, rc);
                    Console.WriteLine("doClientTask: Unable to connect\n");
                    return -1;
                }

                PeerUnreachableWarningCallback callback = new PeerUnreachableWarningCallback(this.peerUnreachableCallback);
                mocket.registerPeerUnreachableWarningCallback(callback);

                int dataSize = IntDataTest.DATA_SIZE;
                int bytesSent = 0;
                long startTime = Util.currentTimeMillis();

                ManagedMessageSender sender = mocket.getSender(true, true);
                byte[] auxBuf = new byte[4];
                Util.fromUnsignedIntTo4Bytes((uint)dataSize, auxBuf, 0);

                // sending the data size.
                sender.send(auxBuf, 4);
                Console.WriteLine("will write {0} bytes of data", dataSize);
                while (bytesSent < dataSize) {
                    sender.send (buf, (uint)buf.Length);
                    bytesSent += buf.Length;
                    //Console.WriteLine("written so far :: {0}", bytesSent);
                }

                Console.WriteLine("done writing data.");

                mocket.receive(auxBuf, 1);
                if (auxBuf[0] != '.') {
                    Console.WriteLine("doClientTask: failed to receive . from remote host");
                    return -2;
                }

                long time = Util.currentTimeMillis() - startTime;
                stats.update((double) time);

                // Save results to a file
                Util.saveStats (mocket, "MsgMockets-cs", time);
            }
            else {
                //Socket socket = new Socket (AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                //socket.Connect(remoteHost, remotePort);
                TcpClient client = new TcpClient(remoteHost, remotePort);
                NetworkStream stream = client.GetStream();

                int dataSize = IntDataTest.DATA_SIZE;
                int bytesSent = 0;
                long startTime = Util.currentTimeMillis();

                byte[] auxBuf = new byte[4];
                Util.fromUnsignedIntTo4Bytes((uint)dataSize, auxBuf, 0);

                // sending the data size.
                //socket.Send (auxBuf);
                stream.Write(auxBuf, 0, 4);
                Console.WriteLine ("socket will write {0} bytes of data", dataSize);
                while (bytesSent < dataSize) {
                    //socket.Send (buf);
                    stream.Write(buf, 0, buf.Length);
                    bytesSent += buf.Length;
                    //Console.WriteLine("written so far :: {0}", bytesSent);
                }
                stream.Flush();

                Console.WriteLine("Socket done writing data.");
                Console.WriteLine("->>:Socket before socket.Receive");
                //socket.Receive (auxBuf);
                stream.Read(auxBuf, 0, 1);
                Console.WriteLine("->>:Socket after socket.Receive", auxBuf[0]);
                if (auxBuf[0] != '.') {
                    Console.WriteLine("doClientTask:Socket failed to receive . from remote host");
                    return -2;
                }

                long time = Util.currentTimeMillis() - startTime;
                stats.update ((double)time);

                // Save results to a file
                Console.WriteLine("->>:Socket before saveStats");
                //Util.saveStats (socket, "Sockets-cs", time);
                Util.saveStats(client, "Sockets-cs", time);
                Console.WriteLine("->>:Socket after saveStats");
            }
            return 0;
        } // doClientTask

        // ------------------------------------------------------------------------------
        private ushort _portNum = 4000;
        public const int BUFFER_SIZE = 1024;
        public const int DATA_SIZE = 1024 * 1024;
        // ------------------------------------------------------------------------------

        static void Main(string[] args)
        {
            if (args.Length < 2) {
                Console.WriteLine("usage: <server|client> port [<remoteHost> [iterations]]");
                System.Environment.Exit(1);
            }

            int portNumber = Int32.Parse(args[1]);
            string remoteHost = "localhost";
            int iterations = 100;

            IntDataTest idt = new IntDataTest();

            if (args.Length > 2) {
                remoteHost = args[2];
            }
            if (args.Length > 3) {
                iterations = Int32.Parse(args[3]);
            }

            if (args[0] == "client") {
                Console.WriteLine("Trying to establish a mocket/socket connection to port {0}", portNumber);
                Stats socketStats = new Stats();
                Stats mocketStats = new Stats();
                int rc;

                for (int i = 0; i < iterations; i++)
                {
                    Console.WriteLine("iteration # :: {0}", i);

                    rc = idt.doClientTask(remoteHost, (ushort) portNumber, true, mocketStats);
                    if (rc != 0)
                    {
                        Console.WriteLine("main: doClientTask failed for sockets with rc = {0}", rc);
                        System.Environment.Exit(-2);
                    }
                    
                    rc = idt.doClientTask(remoteHost, (ushort) portNumber, false, socketStats);
                    if (rc != 0)
                    {
                        Console.WriteLine("main: doClientTask failed for mockets with rc = {0}", rc);
                        System.Environment.Exit(-3);
                    }

                    Console.WriteLine("-----------------------------------------");
                    Console.WriteLine("TotalAttempts: {0}\n", i + 1);
                    Console.WriteLine("MessageMocket Stats:: Average:       {0,10:#.##}", mocketStats.getAverage());
                    Console.WriteLine("MessageMocket Stats:: St Deviation:  {0,10:#.##}", mocketStats.getStDev());
                    Console.WriteLine("Socket Stats:: Average:       {0,10:#.##}", socketStats.getAverage());
                    Console.WriteLine("Socket Stats:: St Deviation:  {0,10:#.##}", socketStats.getStDev());
                    //Console.WriteLine("Socket Stats:: St Deviation:  %10.4f", socketStats.getStDev());
                    Console.WriteLine("-----------------------------------------");

                    Console.WriteLine("Sleeping for 3 seconds...\n");
                    Thread.Sleep(3000);
                }
            }
            else if (args[0] == "server") {
                idt.runServers((ushort)portNumber);
            }
        } //Main
    }
}
