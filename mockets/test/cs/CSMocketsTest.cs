using System;
using System.Collections.Generic;
using System.Text;

using us.ihmc.mockets;

namespace mocketstest
{
    class Client
    {
        System.Threading.Thread mythread;
        string name;

        public Client(string name)
        {
            this.name = name;
        }

        void ClientThread()
        {
            while (true)
            {
                ManagedMessageMocket mm = new ManagedMessageMocket();
                System.Console.WriteLine("{0}: connecting to localhost:4444", name);
                mm.connect("localhost", 4444);

                int bytes = 0;
                System.Console.WriteLine("{0}: waiting for data", name);
                while (bytes != -1)
                {
                    byte[] buf = new byte[256];
                    bytes = mm.receive(buf, 256);
                    if (bytes != -1)
                    {
                        System.Console.Write("{0}: received {1} bytes: ", name, bytes);
                        System.Console.WriteLine(System.Text.Encoding.ASCII.GetString(buf, 0, bytes));
                    }
                    else
                    {
                        System.Console.WriteLine("{0}: disconnected", name);
                    }
                }
                //System.Threading.Thread.Sleep(500);
            }
        }

        public void go()
        {
            mythread = new System.Threading.Thread(ClientThread);
            mythread.Name = this.name;
            mythread.Start();
        }

        public void stop()
        {
            mythread.Abort();
        }

    }

    class Program
    {
        static ManagedMessageServerMocket masterSrv = null;

        public class Handler
        {
            ManagedMessageMocket c;
            System.Threading.Thread myThread;

            public Handler(ManagedMessageMocket c)
            {
                this.c = c;
                myThread = new System.Threading.Thread(HandlerThread);
                myThread.Start();
            }

            void HandlerThread()
            {
                string str1 = "And it's hi, hi, hey! The Army's on it's way";
                string str2 = "Count off the cadence loud and strong";
                string str3 = "For where e'er we go, you will always know";
                string str4 = "That the Army goes rolling along.";

                ManagedMessageSender s = c.getSender(true, true);
                System.Console.WriteLine("Handler: sending data");
                System.Threading.Thread.Sleep(100);
                s.send(System.Text.Encoding.ASCII.GetBytes(str1), (uint)str1.Length);
                s.send(System.Text.Encoding.ASCII.GetBytes(str2), (uint)str2.Length);
                s.send(System.Text.Encoding.ASCII.GetBytes(str3), (uint)str3.Length);
                s.send(System.Text.Encoding.ASCII.GetBytes(str4), (uint)str4.Length);
                System.Console.WriteLine("Handler: closing mocket");
                c.close();
            }
        }



        static void ServerThread()
        {
            masterSrv = new ManagedMessageServerMocket();
            System.Console.WriteLine("Server: listening on port 4444");
            masterSrv.listen(4444);

            while (true)
            {
                System.Console.WriteLine("Server: waiting for connections");
                ManagedMessageMocket newm = masterSrv.accept();
                System.Console.WriteLine("Server: got connect");
                //System.Threading.Thread.Sleep(2000);
                System.Console.WriteLine("Server: starting handler");
                Handler h = new Handler(newm);
            }
        }

        static void Main(string[] args)
        {
            Client c1 = new Client("Client1");
            Client c2 = new Client("Client2");
            Client c3 = new Client("Client3");
            Client c4 = new Client("Client4");
            System.Threading.Thread server = new System.Threading.Thread(new System.Threading.ThreadStart(ServerThread));

            server.Start();
            System.Threading.Thread.Sleep(1000);
            c1.go();
            c2.go();
            c3.go();
            c4.go();

            //while (true)
            {
                System.Threading.Thread.Sleep(3000);
                System.Console.WriteLine("MAIN THREAD TAKING OUT THE TRASH");
                System.GC.Collect();
            }

            System.Console.WriteLine("----- Shutting down! -----");
            c1.stop();
            c2.stop();
            c3.stop();
            c4.stop();
            server.Abort();
            masterSrv.close();

            System.Console.WriteLine("----- Bye bye! -----");
        }
    }
}
