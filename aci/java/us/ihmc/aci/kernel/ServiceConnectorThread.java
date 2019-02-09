package us.ihmc.aci.kernel;

import us.ihmc.aci.service.DirectConnectionService;
import com.ibm.JikesRVM.ThreadCapturedException;
import com.ibm.JikesRVM.CaptureStateRequest;

/**
 * This class implements the DirectConnection communication mode between a client
 * and a service in the ServiceManager. It uses a ServiceInputStream and a ServiceOutputStream
 * to exchange data in both directions
 *
 * @author rquitadamo
 */
class ServiceConnectorThread extends WorkerThread {

    /**
     * Main constructor of the Connector Thread
     *
     * @param sii The instance of the service
     * @param jsr The service request to handle
     */
    ServiceConnectorThread(ServiceInstanceInfo sii, JavaServiceRequest jsr) {
        super(sii, jsr, "[ServiceConnectorThread]");
    }

    /**
     * Main lifecyle loop of this Service Connector thread
     */
    public void run()
    {
        try{
            /* *** Notify the service instance that connection is about to be performed by this thread ***** */
            serviceInstance.connectOnServiceInstance();
            /* ********************************************************************************************* */

            log ("Service [" + request.UUID + "] connected");
            DirectConnectionService directConnService = (DirectConnectionService) serviceInstance.instance;
            EmbeddedRVMServiceManager.requestCompleted (request, true);

            // Invoke the interface method handleConnection()
            directConnService.handleConnection (request.sis, request.sos);

            log ("Service [" + request.UUID + "] returned from handleConnection().");
        }
        catch (ThreadCapturedException e)
        {
            // If the thread comes here is because it has been captured and it has to terminate is execution on this machine
            log ("Connection Handler for service " + serviceInstance.serviceInfo.serviceClassName + " captured. Now dying...");
        }
        catch (Exception e)
        {
            log ("Problem connecting service " + serviceInstance.serviceInfo.serviceClassName);
            e.printStackTrace();
        }
        finally{
            // Unregister this worker
            serviceInstance.invokers.remove(this);
        }
    }

    /**
     * This method passes a capture state request to the worker thread so that it can
     * collect its state and write it into the passed CaptureStateRequest.
     *
     * @param request The CaptureStateRequest object built to represent this migration request
     *
     * @throws IllegalStateTransition if the thread cannot be captured because in an invalid state
     */
    void notifyCaptureRequest(CaptureStateRequest captureRequest) throws IllegalStateTransition
    {
        throw new UnsupportedOperationException("Capture not yet implemented for Connector threads");
    }

    /**
     * A connection request is never a try again invocation
     */
    boolean isTryAgainInvocation() {
        return false;
    }

}
