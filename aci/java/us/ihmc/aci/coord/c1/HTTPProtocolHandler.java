package us.ihmc.aci.coord.c1;

import java.io.*;
import java.net.*;

import java.util.StringTokenizer;

import us.ihmc.io.LineReaderInputStream;

/**
 * HTTPProtocolHandler.java
 *
 * Created on October 10, 2006, 8:26 PM
 *
 * @author  nsuri
 */
public class HTTPProtocolHandler extends Thread
{
    public HTTPProtocolHandler (Coordinator c)
        throws IOException
    {
        init (80);
        _coordinator = c;
    }

    public HTTPProtocolHandler (Coordinator c, int port)
        throws IOException
    {
        init (port);
        _coordinator = c;
    }

    public void run()
    {
        try {
            while (true) {
                Socket s = _serverSock.accept();
                handleConnection (s);
            }
        }
        catch (Exception e) {
            System.out.println ("HTTPProtocolHandler terminating due to exception " + e);
        }
    }

    private class HTTPHeader
    {
        String requestMethod;
        String resourceName;
        String action;
        String version;
        String clientNodeUUID;
        String contentType;
        int contentLength;
        boolean continueResponseNeeded;
    }

    private void init (int port)
        throws IOException
    {
        _serverSock = new ServerSocket (port);
    }

    private void handleConnection (Socket s)
    {
        try {
            LineReaderInputStream lris = new LineReaderInputStream (s.getInputStream());
            OutputStream os = s.getOutputStream();
            HTTPHeader header = readAndParseHeader (lris);
            checkHeader (header);

            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter (s.getOutputStream()));
            
            if (header.action.equalsIgnoreCase ("activate")) {
                Coordinator.ActivationInfo activationInfo;
                activationInfo = _coordinator.activateService (header.resourceName, header.clientNodeUUID);
                
                if (activationInfo != null) {
                    //activation was successfull
                    String replyMsg = "<activationResponse>\n\t<instanceUUID>";
                    replyMsg += activationInfo.instanceUUID;
                    replyMsg += "</instanceUUID>\n\t<nodeUUID>";
                    replyMsg += activationInfo.nodeUUID;
                    replyMsg += "</nodeUUID>\n</activationResponse>";

                    bw.write ("HTTP/1.1 200 OK\r\n");
                    bw.write ("Content-Type: soap+xml\r\n");
                    bw.write ("Content-Length: " + replyMsg.length()+1 + "\r\n");
                    bw.write ("\r\n");
                    bw.write (replyMsg + "\r\n");
                    bw.flush();
                    bw.close();
                    s.close();
                }
                else {
                    //activation failed
                    bw.write ("HTTP/1.1 500 Internal Server Error\r\n");
                    bw.close();
                    s.close();
                }
            }

            s.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private HTTPHeader readAndParseHeader (LineReaderInputStream lris)
        throws ProtocolException
    {
        try {
            HTTPHeader header = new HTTPHeader();
            
            // Parse the first line, which is the protocol line
            String protoLine = lris.readLine();
            StringTokenizer st = new StringTokenizer (protoLine, "/ \t\n\r\f");
            header.requestMethod = st.nextToken();
            header.resourceName = st.nextToken();
            String protocol = st.nextToken();
            if (!protocol.equalsIgnoreCase ("HTTP")) {
                throw new ProtocolException ("Expected protocol to be HTTP but received " + protocol);
            }
            header.version = st.nextToken();

            // Parse the rest of the header
            while (true) {
                String headerLine = lris.readLine();
                if (headerLine.equalsIgnoreCase ("")) {
                    // Reached the end of the header
                    return header;
                }
                st = new StringTokenizer (headerLine);
                String attr = st.nextToken();
                String value = st.nextToken();
                if (attr.equalsIgnoreCase ("Action:")) {
                    header.action = value;
                }
                else if (attr.equalsIgnoreCase ("Content-Type:")) {
                    header.contentType = value;
                }
                else if (attr.equalsIgnoreCase ("Content-Length:")) {
                    header.contentLength = Integer.parseInt (value);
                }
                else if (attr.equalsIgnoreCase ("Client-Node-UUID:")) {
                    header.clientNodeUUID = value;
                }
                else if ((attr.equalsIgnoreCase ("Expect:")) && (value.equalsIgnoreCase ("100-continue"))) {
                    header.continueResponseNeeded = true;
                }
                else {
                    System.out.println ("Ignoring unknown received line <" + headerLine + ">");
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
            throw new ProtocolException ("Exception <" + e + "> when reading and parsing header");
        }
    }

    private void checkHeader (HTTPHeader header)
        throws ProtocolException
    {
        if (!header.requestMethod.equalsIgnoreCase ("POST")) {
            throw new ProtocolException ("Expected method to be POST but received <" + header.requestMethod + ">");
        }
        if (!header.action.equalsIgnoreCase ("Activate")) {
            throw new ProtocolException ("Expected action to be Activate but received <" + header.action + ">");
        }
        if (!header.contentType.equalsIgnoreCase ("DIME")) {
            throw new ProtocolException ("Expected content type to be DIME but received <" + header.contentType + ">");
        }
    }

    private Coordinator _coordinator;
    private int _port;
    private ServerSocket _serverSock;
}
