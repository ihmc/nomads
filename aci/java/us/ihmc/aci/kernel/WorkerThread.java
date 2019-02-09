package us.ihmc.aci.kernel;

import com.ibm.JikesRVM.CaptureStateRequest;

/**
 * Generic worker thread, superclass of invokers, connectors and restore threads
 *
 * @author rquitadamo
 *
 */
abstract class WorkerThread extends Thread {

    WorkerThread(ServiceInstanceInfo sii, JavaServiceRequest jsr, String name) 
    {
        super(name);
        serviceInstance = sii;
        request = jsr;

        // Register this worker into the ServiceInstanceInfo object
        serviceInstance.invokers.add(this);
    }

    abstract boolean isTryAgainInvocation();

    /**
     * This method passes a capture state request to the worker thread so that it can
     * collect its state and write it into the passed CaptureStateRequest.
     *
     * @param request The CaptureStateRequest object built to represent this migration request
     *
     * @throws IllegalStateTransition if the thread cannot be captured because in an invalid state
     */
    abstract void notifyCaptureRequest(CaptureStateRequest request) throws IllegalStateTransition;


    /**
     * Logging function for the worker thread
     *
     * @param msg
     */
    static void log(String msg) {
        if (DEBUG) {
            System.out.println(Thread.currentThread().getName() + ": " + msg);
        }
    }


    // //////////////////////////////////////////////////////////////////////////
    // INSTANCE FIELDS                                                         //
    // //////////////////////////////////////////////////////////////////////////

    /**
     * The instance of the service on whom the method is to be invoked
     */
    transient protected ServiceInstanceInfo serviceInstance;

    /**
     * The invocation request assigned to this worker
     */
    transient protected JavaServiceRequest request;

    // //////////////////////////////////////////////////////////////////////////
    // STATIC FIELDS                                                           //
    // //////////////////////////////////////////////////////////////////////////
    private static final boolean DEBUG = true;
}
