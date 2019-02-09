/**
 * HTTPAgserveClient
 *
 * @author Marco Arguedas      <marguedas@ihmc.us>
 * @author Matteo Rebeschini   <mrebeschini@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.agserve;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.URL;
import java.util.Hashtable;

import us.ihmc.aci.util.kernel.Locator;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.Dime;
import us.ihmc.util.HTTPHelper;

/**
 *
 */
public class HTTPAgServeClient extends AgServeClient
{
    public HTTPAgServeClient()
    {
        super();
    }

    public HTTPAgServeClient (boolean useMockets)
    {
        super(useMockets);
    }

    /**
     *
     */
    protected String activateService (CommHelper commHelper, String nodeUUID, String serviceName, boolean onLocalNodeOnly)
        throws Exception
    {
        if (serviceName == null) {
            throw new NullPointerException("serviceClassname");
        }

        HTTPHelper httpHelper = new HTTPHelper (commHelper);

        Hashtable htAux = new Hashtable();

        try {
            // connect to the ACI Kernel
            // and issue an 'activate' request.

            if (onLocalNodeOnly) {
                htAux.put ("Action", "activate_locally");
            }
            else {
                htAux.put ("Action", "activate");
            }
            htAux.put ("Content-Type", "dime");
            htAux.put ("Content-Length", "0");
            htAux.put ("RequestorUUID", _clientUUID);
            htAux.put ("Expect", "100-continue");
            if (nodeUUID != null) {
                htAux.put ("NodeUUID", nodeUUID);
            }

            httpHelper.sendPOSTRequest (serviceName, htAux);

            httpHelper.expectResponse (100);

            // send the DIME here (not necessary right now for 'activate'. Just send a blank line)
            //httpHelper.sendBlankLine();

            // expect a "HTTP/1.1 200 OK"
            httpHelper.expectResponse (200);

            // expect "Content-Type: xml"
            //        "Content-Length: <dimeLength>
            Hashtable header = httpHelper.readHeaderFields();

            int contentLength = Integer.parseInt( (String) header.get("Content-Length") );

            //now read and parse the reply

            //this should be the SOAP message (eventually it will be).
            // for now, just expect something like:
            // <serviceInstanceUUID>1234-1234-1234-1234</serviceInstanceUUID>

            byte[] responseBytes = commHelper.receiveBlob (contentLength);

            String strResponse = new String (responseBytes);
            String uri = strResponse.replaceAll("<[^>]+>", "");
            Locator loc = new Locator (uri);
            String instanceUUID = loc.getInstanceUUID();
            nodeUUID = loc.getNodeUUID();
            _svcLocations.put (instanceUUID, nodeUUID);

            log ("received the Instance UUID :: " + instanceUUID);

            return instanceUUID;
        }
        catch (Exception ex) {
            log ("Error activating service " + serviceName + ":");
            ex.printStackTrace();
            throw ex;
        }
    }

    /**
     *
     */
    protected boolean deactivateService (CommHelper commHelper, String nodeUUID, String serviceInstanceUUID)
        throws Exception
    {
        if (serviceInstanceUUID == null) {
            throw new NullPointerException ("serviceInstanceUUID");
        }

        HTTPHelper httpHelper = new HTTPHelper (commHelper);

        try {
            Hashtable htParams = new Hashtable();
            htParams.put ("Action", "deactivate");
            if (nodeUUID != null) {
                htParams.put ("NodeUUID", nodeUUID);
            }
            htParams.put ("InstanceUUID", serviceInstanceUUID);
            htParams.put ("Content-Type", "dime");
            htParams.put ("Content-Length", "0");
            htParams.put ("RequestorUUID", _clientUUID);
            htParams.put ("Expect", "200-OK");

            httpHelper.sendPOSTRequest (serviceInstanceUUID, htParams);
            httpHelper.expectResponse (200);

            _svcLocations.remove (serviceInstanceUUID);
        }
        catch (Exception ex) {
            ex.printStackTrace();
            throw ex;
        }

        return true;
    }

    /**
     *
     */
    protected void deployService (CommHelper commHelper, URL acrFileURL)
        throws Exception
    {
        if (acrFileURL == null) {
            throw new NullPointerException ("acrFileURL");
        }

        HTTPHelper httpHelper = new HTTPHelper (commHelper);
        String acrFileName = acrFileURL.getFile().replace ('\\', '/');
        acrFileName = acrFileName.substring (acrFileURL.getFile().lastIndexOf("/") + 1);

        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            BufferedInputStream bis = new BufferedInputStream (acrFileURL.openStream());

            byte[] buff = new byte[2048];
            int read;

            while ( (read = bis.read(buff)) > 0 ) {
                baos.write (buff, 0, read);
            }

            Dime dimeMsg = new Dime();
            dimeMsg.addRecord ("ACR_File", baos.toByteArray(), "ACR_File");

            Hashtable htAux = new Hashtable();
            htAux.put ("Action", "deploy");
            htAux.put ("Content-Type", "dime");
            htAux.put ("Content-Length", Integer.toString(dimeMsg.getDimeLength()));
            htAux.put ("RequestorUUID", _clientUUID);
            htAux.put ("Expect", "100-continue");

            httpHelper.sendPOSTRequest (acrFileName, htAux);
            httpHelper.expectResponse (100);

            commHelper.sendBlob (dimeMsg.getDime());
            httpHelper.expectResponse (200);
        }
        catch (Exception ex) {
            ex.printStackTrace();
            throw ex;
        }
    }

    /**
     *
     */
    protected InetSocketAddress getNodeLocation (CommHelper commHelper, String nodeUUID)
        throws Exception
    {
        HTTPHelper httpHelper = new HTTPHelper (commHelper);

        Hashtable htAux = new Hashtable();
        htAux.put ("Action", "control");
        htAux.put ("NodeUUID", nodeUUID);
        htAux.put ("RequestorUUID", _clientUUID);
        htAux.put ("Content-Type", "xml");
        htAux.put ("Content-Length", "0");

        httpHelper.sendPOSTRequest ("getNodeLocation", htAux);
        httpHelper.expectResponse (200);

        htAux = httpHelper.readHeaderFields();

        int contentLength = Integer.parseInt ((String) htAux.get("Content-Length"));
        byte[] responseBytes = commHelper.receiveBlob (contentLength);
        String strResponse = new String (responseBytes);
        String aux = strResponse.replaceAll("<[^>]+>", "");
        log ("received Instance location info: " + aux);
        String[] auxArr = aux.split(":");

        InetSocketAddress isa = new InetSocketAddress (InetAddress.getByName(auxArr[0]),
                                                       Integer.parseInt(auxArr[1]));

        return isa;
    }

    /**
     *
     */
    protected String lookupService (CommHelper commHelper, String instanceUUID, String oldNodeUUID)
        throws Exception
    {
        HTTPHelper httpHelper = new HTTPHelper (commHelper);

        Hashtable htAux = new Hashtable();
        htAux.put ("Action", "control");
        htAux.put ("InstanceUUID", instanceUUID);
        if (oldNodeUUID != null) {
            htAux.put ("OldNodeUUID", oldNodeUUID);
        }
        htAux.put ("Content-Type", "xml");
        htAux.put ("Content-Length", "0");
        htAux.put ("RequestorUUID", _clientUUID);

        httpHelper.sendPOSTRequest ("lookupService", htAux);
        httpHelper.expectResponse (200);

        htAux = httpHelper.readHeaderFields();

        int contentLength = Integer.parseInt ((String) htAux.get("Content-Length"));
        byte[] responseBytes = commHelper.receiveBlob (contentLength);
        String strResponse = new String (responseBytes);
        String nodeUUID = strResponse.replaceAll("<[^>]+>", "");
        if (nodeUUID != null) {
            log ("lookup service: received service location: " + nodeUUID);
            return nodeUUID;
        }
        else {
            throw new Exception ("service location unknown");
        }
    }

    /**
     *
     */
    protected CommHelper connectToService (CommHelper commHelper, String serviceInstanceUUID)
        throws Exception
    {
        HTTPHelper httpHelper = new HTTPHelper (commHelper);

        try {
            Hashtable htAux = new Hashtable();
            htAux.put ("Action", "connect_locally"); //TODO: since we connect only locally this should be renamed "connect"
            htAux.put ("InstanceUUID", serviceInstanceUUID);
            htAux.put ("Content-Type", "dime");
            htAux.put ("Content-Length", "0");
            htAux.put ("RequestorUUID", _clientUUID);
            htAux.put ("Expect", "200-OK");

            httpHelper.sendPOSTRequest (serviceInstanceUUID, htAux);

            // expect a "HTTP/1.1 200 OK"
            httpHelper.expectResponse (200);

            return commHelper;
        }
        catch (Exception ex) {
            throw ex;
        }
    }

    /**
     *
     */
    protected Dime invokeService (CommHelper commHelper, String nodeUUID, String serviceInstanceUUID, String methodName, Dime dimeRequest, boolean asynchronous)
        throws Exception
    {
        log ("Invoking service " + serviceInstanceUUID + " method " + methodName + " on node " + nodeUUID );

        HTTPHelper httpHelper = new HTTPHelper (commHelper);

        try {
            Hashtable htAux = new Hashtable();
            htAux.put ("Action", "invoke_locally"); //TODO: since we always invoke "locally" this should be just "invoke"
            htAux.put ("InstanceUUID", serviceInstanceUUID);
            htAux.put ("MethodName", methodName);
            htAux.put ("Content-Type", "dime");
            htAux.put ("Content-Length", Integer.toString(dimeRequest.getDimeLength()));
            htAux.put ("RequestorUUID", _clientUUID);
            htAux.put ("Expect", "100-continue");
            if (nodeUUID != null) {
                htAux.put ("NodeUUID", nodeUUID);
            }
            if (asynchronous) {
                htAux.put ("Asynchronous", "true");
            }
            httpHelper.sendPOSTRequest (serviceInstanceUUID, htAux);
            httpHelper.expectResponse (100);

            // now send the dime.
            byte[] dimeBuff = dimeRequest.getDime();
            commHelper.sendBlob (dimeBuff);

            if (asynchronous) {
                return null;
            }
            else {
                // expect a "HTTP/1.1 200 OK"
                httpHelper.expectResponse (200);

                // now read the response...
                htAux = httpHelper.readHeaderFields();

                int dimeLength = Integer.parseInt ((String)htAux.get("Content-Length"));

                //read the Dime with the results...
                log("reading the dime (of size " + dimeLength + ") with the results...");

                Dime resultDime = new Dime (commHelper.getInputStream());
                if (resultDime.getRecordCount() < 1) {
                    throw new Exception("the DIME message received seems to be empty or corrupted");
                }

                return resultDime;
            }
        }
        catch (Exception ex) {
           log ("Exception occurred invoking service: " + ex.getMessage());
            if (nodeUUID != null) {
                _nodeUUIDToKernelAddressTable.remove (nodeUUID);
            }
            throw ex;
        }
    }

    /**
     *
     */
    protected float getResourceInfo (CommHelper commHelper, int queryType, String serviceName, String methodSignature)
        throws Exception
    {
        HTTPHelper httpHelper = new HTTPHelper (commHelper);

        Hashtable htAux = new Hashtable();
        htAux.put ("Action", "control");
        htAux.put ("ServiceName", serviceName);
        htAux.put ("ResInfoQueryType", queryType + "");
        htAux.put ("Content-Type", "xml");
        htAux.put ("Content-Length", "0");
        htAux.put ("RequestorUUID", _clientUUID);

        httpHelper.sendPOSTRequest ("getResourceInfo", htAux);
        httpHelper.expectResponse (200);

        htAux = httpHelper.readHeaderFields();

        int contentLength = Integer.parseInt ((String) htAux.get("Content-Length"));
        byte[] responseBytes = commHelper.receiveBlob (contentLength);
        String strResponse = new String (responseBytes);
        float resourceInfoRes = Float.parseFloat (strResponse.replaceAll("<[^>]+>", ""));
        if (resourceInfoRes != -1) {
            log ("getResourceInfo - received resource info: " + resourceInfoRes);
            return resourceInfoRes;
        }
        else {
            throw new Exception ("invalid query type");
        }
    }
}
