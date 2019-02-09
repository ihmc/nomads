package us.ihmc.aci.test;

import java.net.Socket;
import java.io.BufferedReader;
import java.io.InputStreamReader;

/**
 * Created by IntelliJ IDEA. User: mcarvalho Date: Dec 23, 2004 Time: 11:00:01 AM To change this template use File |
 * Settings | File Templates.
 */
public class TestProxy
{
    public static void main(String[] args)
    {
        try {
            Socket sock = new Socket ("localhost", 3275);
            BufferedReader br = new BufferedReader (new InputStreamReader(sock.getInputStream()));
            String sline = "";
            while (sline != null) {
                sline = br.readLine();
                System.out.println(sline);
            }

            System.out.println ("DONE");
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
}
