package us.ihmc.aci.kernel;

import java.lang.reflect.Method;
import us.ihmc.aci.agserve.InvokeRequestParser;
import us.ihmc.aci.agserve.InvokeResponseEncoder;
import com.ibm.JikesRVM.ThreadCapturedException;
import com.ibm.JikesRVM.CaptureStateRequest;
import com.ibm.JikesRVM.MobileThread;

/**
 * Class implementing the worker threads handling invocation requests
 *
 * @author rquitadamo
 */
class ServiceInvokerThread extends WorkerThread
{
    /**
     * The main constructor of the worker thread class
     *
     * @param sii The instance to act upon
     * @param jsr The service request that contains the invocation
     */
    ServiceInvokerThread(ServiceInstanceInfo sii, JavaServiceRequest jsr)
    {
        super(sii, jsr, "[ServiceInvokerThread]");
    }

    /**
     * Main loop of the invoker thread
     */
    public void run()
    {
        String methodName = null;
        boolean success = true;
        boolean relocated = false;
        byte[] encodedResponse = null;

        try{

            /* *** Notify the service instance that an invocation is about to be performed ***** */
            serviceInstance.invokeOnServiceInstance();
            /* ********************************************************************************* */

            // 1. Get the class loader from the service info object of this instance
            ClassLoader cl = serviceInstance.serviceInfo.classLoader;

            // 2. Parse the invocation request, reading parameters from the client socket
            InvokeRequestParser parser = new InvokeRequestParser (request.sis, cl);

            // 3. Read the name of the method to invoke and deserialize its parameters
            methodName = parser.isBinaryRequest() ? request.methodName : parser.getMethodName();
            Object[] paramValues = parser.getParameterValues();
            isBinaryRequest = parser.isBinaryRequest();

            // 4. Find the method to invoke using reflection
            Method methodToInvoke = ServiceManager.findMatchingMethod (serviceInstance.serviceInfo.serviceClass,
                                        methodName, parser.getParameterTypes());

            // 5. Did we find a corresponding method to invoke?
            if (methodToInvoke != null) {

                log ("found a matching method! Now Invoking...");

                if (request.asynchronous) {
                    EmbeddedRVMServiceManager.requestCompleted (request, success);
                }

                // Method invocation ...
                Object response = methodToInvoke.invoke (serviceInstance.instance, paramValues);

                // Asynchronous requests do not need responses
                if (request.asynchronous) {
                    return;
                }

                // Synchronous requests need response instead
                InvokeResponseEncoder encoder = new InvokeResponseEncoder();
                encoder.encode (methodName, response, parser.isBinaryRequest(), request.sos);
            }
            else {
                log ("Did not find a matching method (" + methodName + ") in " + serviceInstance.serviceInfo.serviceClassName);
                success = false;
            }
        }
        catch (ThreadCapturedException e) {
            relocated = true;

            // If the thread comes here is because it has been captured and it has to terminate is execution on this machine
            log ("Invocation Handler for method " + methodName + " in service " + serviceInstance.serviceInfo.serviceClassName + " captured. Now dying...");
        }
        catch (Exception e) {
            log ("Problem invoking the " + serviceInstance.serviceInfo.serviceClassName + "." + methodName + " method:");
            success = false;
            e.printStackTrace();
        }
        finally{
            if (!request.asynchronous) {
                if (relocated) {
                    EmbeddedRVMServiceManager.requestRelocated (request, request.isTryAgain);
                }
                else {
                    EmbeddedRVMServiceManager.requestCompleted (request, success);
                }
            }

            // Unregister this worker
            serviceInstance.invokers.remove(this);
        }
    }

    /**
     * This method passes a capture state request to the invoker thread so that it can
     * collect its state and write it into the passed CaptureStateRequest.
     *
     * @param request The CaptureStateRequest object built to represent this migration request
     *
     * @throws IllegalStateTransition if the invoker cannot be captured because in an invalid state
     */
    void notifyCaptureRequest (CaptureStateRequest captureRequest)
        throws IllegalStateTransition
    {
        // 1. If we are here, the transition is allowed.
        // Then, it's time to notify the thread, passing it the CaptureStateRequest object and return.
        if ( ((MobileThread)this).captureThread(captureRequest, request.isTryAgain) == false ) {
            throw new IllegalStateTransition("The invoker thread maybe dead or not yet started");
        }

        // 2. Notification ok!
    }

    boolean isTryAgainInvocation()
    {
        return request.isTryAgain;
    }

    boolean isBinaryRequest()
    {
        return isBinaryRequest;
    }

    // //////////////////////////////////////////////////////////////////////////
    // INSTANCE FIELDS                                                         //
    // //////////////////////////////////////////////////////////////////////////

    /**
     * Is the request handled a binary request?
     */
    transient boolean isBinaryRequest;

}
