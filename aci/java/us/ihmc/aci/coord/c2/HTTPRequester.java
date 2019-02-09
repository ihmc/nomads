package us.ihmc.aci.coord.c2;

import java.io.BufferedWriter;
import java.io.OutputStreamWriter;

import java.net.InetAddress;
import java.net.Socket;

import java.util.StringTokenizer;

//import us.ihmc.ffaci.feedParams.FeedParams;

import us.ihmc.io.LineReaderInputStream;

/**
 * HTTPRequester.java
 *
 * @author nsuri
 */
public class HTTPRequester
{
    public static String tellNodeToActivateService (InetAddress nodeAddr, int kernelPort, String serviceName)
    {
        try {
            Socket s = new Socket (nodeAddr, kernelPort);

            LineReaderInputStream lris = new LineReaderInputStream (s.getInputStream());
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter (s.getOutputStream()));
            bw.write ("POST " + serviceName + " HTTP/1.1\r\n");
            bw.write ("Action: activate_locally\r\n");
            bw.write ("Content-Type: dime\r\n");
            bw.write ("Content-Length: 0\r\n");
            bw.write ("Expect: 100-continue\r\n");
            bw.write ("\r\n");
            bw.flush();

            String lineAux = lris.readLine();

            StringTokenizer st = new StringTokenizer(lineAux, "\n\r:,; ");
            st.nextToken();
            String respCode = st.nextToken();

            if (!"100".equals (respCode)) {
                throw new Exception ("Error occurred: " + respCode);
            }

            // send the DIME here (not necessary right now for 'activate')
            bw.write ("\r\n");
            bw.flush();

            // expect a "HTTP/1.1 200 OK"
            lineAux = lris.readLine();
            st = new StringTokenizer (lineAux, "\n\r:,; ");
            st.nextToken();
            respCode = st.nextToken();

            if (!"200".equals(respCode)) {
                throw new Exception ("Error occurred: " + respCode);
            }

            // expect "Content-Type: xml+soap"
            //        "Content-Length: <dimeLength>
            String contentType = null;
            int contentLength = 0;

            while ((lineAux = lris.readLine()) != null) {
                if (lineAux.equals ("\r\n") || lineAux.equals ("")) {
                    break;
                }
                st = new StringTokenizer (lineAux, "\n\r;:, ");

                String key = st.nextToken();
                String value = st.nextToken();

                if (key.equals("Content-Type")) {
                    contentType = value;
                }
                else if (key.equals("Content-Length")) {
                    contentLength = Integer.parseInt(value);
                }
            }

            //now read and parse the dime
            /*
            Dime dimeMsg = new Dime(lris);
            if (dimeMsg.getRecordCount() < 1) {
                throw new Exception("the DIME message received seems to be empty or corrupted");
            }

            DimeRecord dimeRec = (DimeRecord) (dimeMsg.getRecords().nextElement());

            //this should be the SOAP message (eventually it will be).
            // for now, just expect something like:
            // <serviceInstanceUUID>1234-1234-1234-1234</serviceInstanceUUID>
            */
            lineAux = new String (lris.readLine());
            String uuid = lineAux.replaceAll("<[^>]+>", "");
            return uuid;
        }
        catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

//    public static String tellNodeToStartFeed (InetAddress nodeAddr, int kernelPort, String producerName, String subscriberNodeUUID, String subscriberName, FeedParams feedParams)
//    {
//        return null;
//    }

    public static String tellNodeToStopFeed (InetAddress nodeAddr, int kernelPort, String feedId)
    {
        return null;
    }
}
