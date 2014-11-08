
package us.ihmc.mockets.test;

import us.ihmc.mockets.*;
import java.io.*;
import java.net.*;

public class StreamMocketTest
{
    public static void main(String[] args)
        throws Exception
    {
        String usage = "usage: StreamMocketTest <server|client>";
        if (args.length < 1) {
            System.out.println(usage);
            System.exit(1);
        }

        String address = "127.0.0.1";
        int port = 4000;
        byte[] buff = new byte[2048];
        int DATA_LEN = 1024*1024;

        for (int i = 0; i < buff.length; i++) {
            buff[i] = (byte)('a' + (i % 20));
        }

        if (args[0].equals("client")) {
            StreamMocket mocket = new StreamMocket();
            mocket.connect(InetAddress.getByName(address), port);

            System.out.println("mocket connected.");

            DataOutputStream dos = new DataOutputStream(mocket.getOutputStream());

            System.out.println("will write " + DATA_LEN + " bytes");
            dos.writeInt(DATA_LEN);
            int written = 0;

            while (written < DATA_LEN) {
                dos.write(buff);
                dos.flush();

                written += buff.length;
            }

            DataInputStream dis = new DataInputStream (mocket.getInputStream());
            dis.readInt();

            System.out.println("done writing data!");
            mocket.close();
        }
        else if (args[0].equals("server")) {
            //StreamServerMocket serverMocket = new StreamServerMocket(port);
            InetSocketAddress isa = new InetSocketAddress(InetAddress.getByName(address), port);
            StreamServerMocket serverMocket = new StreamServerMocket(isa);

            while (true) {
                try {
                    System.out.println("waiting for a connection...");
                    StreamMocket mocket = serverMocket.accept();
                    System.out.println("connection accepted.");

                    DataInputStream dis = new DataInputStream (mocket.getInputStream());
                    DataOutputStream dos = new DataOutputStream (mocket.getOutputStream());

                    int dataSize = dis.readInt();

                    int read = 0;
                    int totalRead = 0;
                    while (totalRead < dataSize) {
                        System.out.println("read " + totalRead + " out of " + dataSize + " total bytes");
                        read = dis.read (buff);
                        if (read < 0) {
                            System.out.println("error reading data. Exit!");
                            mocket.close();
                            System.exit (1);
                        }

                        totalRead += read;
                    }

                    dos.writeInt(0);
                    dos.flush();

                    System.out.println("done reading the data.");
                    mocket.close();
                }
                catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        }
        else {
            System.out.println(usage);
            System.exit(2);
        }
    }
}
