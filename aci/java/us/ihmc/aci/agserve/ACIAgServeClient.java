package us.ihmc.aci.agserve;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.net.InetSocketAddress;
import java.net.URL;

import us.ihmc.aci.util.kernel.Locator;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.Dime;

/**
 * 
 * @author mrebeschini
 */
public class ACIAgServeClient extends AgServeClient
{
    protected boolean authenticate (CommHelper commHelper)
    {
        try {
            commHelper.sendLine ("ACI/1.0");
            if (_kernelNodeUUID.equals("")) {
                _kernelNodeUUID = commHelper.receiveRemainingLine ("WELCOME to ");
            }
            else {
                commHelper.receiveRemainingLine ("WELCOME to ");
            }
        }
        catch (Exception e) {
			e.printStackTrace();
            return false;
        }
        return true;
    }

	protected String activateService (CommHelper commHelper, String nodeUUID, String serviceName, boolean onLocalNodeOnly) 
		throws Exception 
	{
		commHelper.sendLine ("SPEAK activate/1.0");
		
		commHelper.sendLine ("START_SERVICEINFO");
		commHelper.sendLine ("ServiceName " + serviceName);
        if (nodeUUID != null) {
		    commHelper.sendLine ("NodeUUID " + nodeUUID);
		}
        commHelper.sendLine ("RequestorUUID " + _clientUUID);
        if (onLocalNodeOnly) {
            commHelper.sendLine ("OnLocalNodeOnly");
        }
		commHelper.sendLine ("END_SERVICEINFO");
		
		commHelper.receiveMatch ("SERVICEINFO_VALID");
		
		commHelper.receiveMatch ("START_ACTIVATION_RESULT");
		String serviceURI = commHelper.receiveLine();
		commHelper.receiveMatch ("END_ACTIVATION_RESULT");

        Locator locator = new Locator (serviceURI);
        _svcLocations.put (locator.getInstanceUUID(), locator.getNodeUUID());
		return locator.getInstanceUUID();
	}

	protected CommHelper connectToService (CommHelper commHelper, String serviceInstanceUUID) 
		throws Exception 
	{
		commHelper.sendLine ("SPEAK connect/1.0");
		
		commHelper.sendLine ("START_SERVICEINFO");
		commHelper.sendLine ("ServiceInstanceUUID " + serviceInstanceUUID);
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
		commHelper.sendLine ("END_SERVICEINFO");
		
		commHelper.receiveMatch ("SERVICEINFO_VALID");
        commHelper.receiveMatch ("CONNECTTOSERVICE_OK");
		
		return commHelper;
	}

	protected boolean deactivateService (CommHelper commHelper, String nodeUUID, String serviceInstanceUUID) 
		throws Exception 
	{
		commHelper.sendLine ("SPEAK deactivate/1.0");
		
		commHelper.sendLine ("START_SERVICEINFO");
		commHelper.sendLine ("NodeUUID " + nodeUUID);
		commHelper.sendLine ("ServiceInstanceUUID " + serviceInstanceUUID);
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
		commHelper.sendLine ("END_SERVICEINFO");
		
		commHelper.receiveMatch ("SERVICEINFO_VALID");
		
		try {
			commHelper.receiveMatch ("DEACTIVATESERVICE_OK");
			return true;
		}
		catch (Exception e) {
			return false;
		}
	}

	protected void deployService (CommHelper commHelper, URL acrFileURL)
		throws Exception 
	{
        String acrFileName = acrFileURL.getFile().replace ('\\', '/');
        acrFileName = acrFileName.substring (acrFileURL.getFile().lastIndexOf("/") + 1);
        
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        BufferedInputStream bis = new BufferedInputStream (acrFileURL.openStream());

        byte[] buff = new byte[2048];
        int read;

        while ( (read = bis.read(buff)) > 0 ) {
            baos.write (buff, 0, read);
        }

        Dime dimeMsg = new Dime();
        dimeMsg.addRecord ("ACR_File", baos.toByteArray(), "ACR_File");

        commHelper.sendLine ("SPEAK deploy/1.0");
		commHelper.sendLine ("START_SERVICEINFO");
        commHelper.sendLine ("ServiceName " + acrFileName); //TODO: read the service name from the xml inside the acr
		commHelper.sendLine ("DimeLength " + Integer.toString(dimeMsg.getDimeLength()));
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
        commHelper.sendLine ("OnLocalNodeOnly"); // TODO: this should be passed as an argment
		commHelper.sendLine ("END_SERVICEINFO");
		
		commHelper.receiveMatch ("SERVICEINFO_VALID");
		
		commHelper.sendLine ("START_DEPLOYSERVICE");
		commHelper.sendBlob (dimeMsg.getDime());
		commHelper.sendLine ("END_DEPLOYSERVICE");
		commHelper.receiveMatch ("DEPLOYSERVICE_OK");
	}

	protected InetSocketAddress getNodeLocation (CommHelper commHelper, String nodeUUID) 
		throws Exception 
	{
        if (nodeUUID == null) {
            throw new NullPointerException ("nodeUUID");
        }

		commHelper.sendLine ("SPEAK control/1.0");
		
		commHelper.sendLine ("START_CONTROL_INFO");
		commHelper.sendLine ("ActionName getNodeLocation");
		commHelper.sendLine ("NodeUUID " + nodeUUID);
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
		commHelper.sendLine ("END_CONTROL_INFO");
		
        commHelper.receiveMatch ("CONTROLINFO_VALID");
		commHelper.receiveMatch ("START_CONTROL_RESULT");
		String buff = commHelper.receiveRemainingLine ("NodeLocation "); // ip:port
		commHelper.receiveMatch ("END_CONTROL_RESULT");
		String[] nodeSockAdd = buff.split(":");
		
		return new InetSocketAddress (nodeSockAdd[0], Integer.parseInt(nodeSockAdd[1]));
	}

	protected Dime invokeService (CommHelper commHelper, String nodeUUID, String instanceUUID, String methodName, Dime dimeRequest, boolean asynchronous)
		throws Exception 
	{
		commHelper.sendLine ("SPEAK invoke/1.0");
		
		commHelper.sendLine ("START_SERVICEINFO");
		if (nodeUUID != null) {
			commHelper.sendLine ("NodeUUID " + nodeUUID);
		}
		commHelper.sendLine ("ServiceInstanceUUID " + instanceUUID);
		commHelper.sendLine ("MethodName " + methodName);
        if (asynchronous) {
		    commHelper.sendLine ("Asynchronous");
		}
        commHelper.sendLine ("DimeLength " + dimeRequest.getDimeLength());
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
		commHelper.sendLine ("END_SERVICEINFO");
		
		commHelper.receiveMatch ("SERVICEINFO_VALID");
		commHelper.sendBlob (dimeRequest.getDime());
		
		if (asynchronous) {
            return null;
        }
        else {
        	commHelper.receiveMatch ("START_INVOKE_RESULT");
        	int dimeLength = Integer.parseInt (commHelper.receiveRemainingLine ("DimeLength "));
        	log ("reading the dime (of size " + dimeLength + ") with the results...");

            Dime resultDime = new Dime (commHelper.getInputStream());
            if (resultDime.getRecordCount() < 1) {
                throw new Exception("the DIME message received seems to be empty or corrupted");
            }

            commHelper.receiveMatch ("END_INVOKE_RESULT");
            return resultDime;
        }
	}

	protected String lookupService (CommHelper commHelper, String instanceUUID, String oldNodeUUID) 
		throws Exception 
	{
		commHelper.sendLine ("SPEAK control/1.0");

		commHelper.sendLine ("START_CONTROL_INFO");
		commHelper.sendLine ("ActionName getNodeLocation");
		commHelper.sendLine ("InstanceUUID " + instanceUUID);
		commHelper.sendLine ("OldNodeUUID " + oldNodeUUID);
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
		commHelper.sendLine ("END_CONTROL_INFO");
        commHelper.receiveMatch ("CONTROLINFO_VALID");

		commHelper.receiveMatch ("START_CONTROL_RESULT");
		String nodeUUID = commHelper.receiveRemainingLine ("NodeUUID "); // nodeUUID
		commHelper.receiveMatch ("END_CONTROL_RESULT");
		return nodeUUID;
	}

    protected float  getResourceInfo (CommHelper commHelper, int queryType, String serviceName, String methodSignature)
        throws Exception
    {
        commHelper.sendLine ("SPEAK control/1.0");

        commHelper.sendLine ("START_CONTROL_INFO");
        commHelper.sendLine ("ActionName getResourceInfo");
        commHelper.sendLine ("ServiceName " + serviceName);
        commHelper.sendLine ("MethodSignature " + methodSignature);
        commHelper.sendLine ("ResInfoQueryType " + queryType);
        commHelper.sendLine ("RequestorUUID " + _clientUUID);
        commHelper.sendLine ("END_CONTROL_INFO");

        commHelper.receiveMatch ("CONTROLINFO_VALID");
        commHelper.receiveMatch ("START_CONTROL_RESULT");
        String buff = commHelper.receiveRemainingLine ("ResourceInfoRes ");
        commHelper.receiveMatch ("END_CONTROL_RESULT");
        return Float.parseFloat (buff);
    }
}

