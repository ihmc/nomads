import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStreamWriter;

import java.net.InetAddress;

//import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.StreamMocket;

public class Client
{
    public static void main (String[] args)
    {
        try {
            StreamMocket m = new StreamMocket();
            m.connect (InetAddress.getLocalHost(), 9876);
            BufferedReader br = new BufferedReader (new InputStreamReader (m.getInputStream()));
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter (m.getOutputStream()));
            BufferedReader brConsole = new BufferedReader (new InputStreamReader (System.in));
            while (true) {
                String line = brConsole.readLine();
                if (line == null) {
                    m.close();
                    break;
                }
                bw.write (line);
                bw.newLine();
                bw.flush();
                String replyLine = br.readLine();
                System.out.println (replyLine);
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }
}
