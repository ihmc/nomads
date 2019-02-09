package us.ihmc.aci.kernel;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import com.ibm.JikesRVM.MobileJikesRVM;
import com.ibm.JikesRVM.MobileThread;
import com.ibm.JikesRVM.UnsupportedJikesRVMEnvironment;
import com.ibm.JikesRVM.VM_CompiledMethod;
import us.ihmc.aci.grpMgrOld.GroupManagerProxy;
import us.ihmc.aci.service.DirectConnectionService;
import us.ihmc.aci.service.Service;

/**
 * Service Manager implementation for the Embedded JikesRVM Container
 *
 * @author rquitadamo
 *
 */
public class EmbeddedRVMServiceManager extends ServiceManager
{
    /**
     * Default constructor for the Service Manager
     */
    public EmbeddedRVMServiceManager()
    {
        // 1. Create the global data structures for this service manager
        _servicesHT = new Hashtable();
        _instancesHT = new Hashtable();

        // 2. Create the group manager proxy to respond to group manager events
        _gmp = new GroupManagerProxyJSMImpl();

        // 3. Start the event handler thread that retrieves events from the ACI kernel
        // and handles them (invoking the proper methods on the GroupManagerProxy)
        new RVMEventHandlerThread(_gmp).start();
    }

    /**
     * Main lifecycle loop for the service manager
     * Its main goal is to retrieve service requests from the C++ kernel and
     * spawn worker threads to handle them.
     *
     * N.B. No exception handling is done here. Exception handling is (when possible)
     * done per-handler, i.e. every worker/handler thread catches the possible exceptions
     * raised during the service request processing and sends back an error to the service
     * invoker (i.e. client).
     */
    public void run()
    {
        while (true) {

            // JavaServiceRequest should be transient objects. They become garbage right after
            // they are done serving their purpose.s
            JavaServiceRequest jsr = new JavaServiceRequest();

            while (getNextRequest(jsr) == -1) {
                try {
                    Thread.sleep(4);
                }
                catch (InterruptedException e) {
                }
            }

            // The native method getNextRequest has returned a new JavaServiceRequest
            // Now we have to process it and return as soon as possible to this main
            // loop to read other requests (i.e. to avoid a possible bottleneck)
            switch(jsr.requestType) {
                case ServiceRequest.REQUEST_TYPE_ACTIVATE:
                    processActivateRequest (jsr);
                    break;
                case ServiceRequest.REQUEST_TYPE_TERMINATE:
                    processTerminateRequest (jsr);
                    break;
                case ServiceRequest.REQUEST_TYPE_INVOKE:
                    processInvokeRequest (jsr);
                    break;
                case ServiceRequest.REQUEST_TYPE_CONNECT:
                    processConnectRequest (jsr);
                    break;
                case ServiceRequest.REQUEST_TYPE_CAPTURE_STATE:
                    processCaptureStateRequest (jsr);
                    break;
                case ServiceRequest.REQUEST_TYPE_RESTORE_STATE:
                    processRestoreStateRequest (jsr);
                    break;
                default:
                    log ("Unknown request type " + jsr.requestType + " received for service");
                    requestCompleted (jsr, false);
            }
        }
    }

    /**
     * Method that sends a capture state notification to all invoker threads
     * and then waits for the completion
     *
     * @param jsr The capture state service request
     */
    private void processCaptureStateRequest(JavaServiceRequest jsr)
    {
        ServiceInstanceInfo sii = null;
        //RVMServiceRequestReader.printServiceRequest(jsr);

        try{
             // 1. Find the service instance using the UUID.
             sii = (ServiceInstanceInfo) _instancesHT.get(jsr.UUID);
             if (sii == null) {
                 log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
                 requestCompleted (jsr, false);
                 return;
             }

             // 2. Before actually passing the capture request to this invoker threads,
             //    check if the transition to "migrating" is allowed for this service instance
             sii.migratingOnServiceInstance();

             // 3. Create the shared CaptureWorkerThreadState object, and then pass it to each
             // worker thread. Note that the state object is initialized
             // with the maximum number of threads to capture. This helps waking up the main thread
             // if migration is completed before the timeout expires.
             Vector invokers = sii.invokers;
             int invokersNumber = invokers.size();
             int tryAgainInvokers = sii.countTryAgainInvokers();
             CaptureWorkerThreadState state = new CaptureWorkerThreadState(jsr, sii.instance, invokersNumber - tryAgainInvokers);

             // 4. The capture notification has to be notified to every invoker and
             // connector thread running in this service instance at this time.
             // NOTE: During the state capture the Service Manager becomes "deaf" to other service
             // requests from the kernel. This is because we give maximum priority to service capture
             // so that it can be as fast as possible. Every other invocation received later, will
             // be accepted (if migration aborted) or refused as illegal transitions (if migration succeded).
             for (int i = 0; i < invokersNumber; i++) {
                 WorkerThread t = (WorkerThread) invokers.get(i);
                 t.notifyCaptureRequest(state);
             }

             if (sii.instance instanceof Thread) {
                 ((MobileThread) sii.instance).captureThread(state, false);
             }

             // 5. Wait until the threads have been captured (or until the timeout expires)
             byte[] stateBuffer = state.waitForCompletion();

             // TODO: CLEAN UP POSSIBLE NON CAPTURED SERVICES!!!!

             // 6. Send the state to the kernel
             log("Capture request successfully completed. Sending state back to the ACI Kernel.");

             if (stateBuffer != null) {
                 jsr.sos.write (stateBuffer, 0, stateBuffer.length);
             }
             int result = requestCompleted (jsr, true);
             state.setCompletionResult (result == 0);

             if (result == 0) {
                 log("Service instance " + sii.instanceUUID + " definitely migrated from this Service Manager.");
                 sii.migratedOnServiceInstance();

                 // Remove the instance from both the global _instancesHT and from its ServiceInfo object
                 sii.serviceInfo.removeServiceInstance(sii);
                 _instancesHT.remove(sii);
             }
             else {
                 log("Service instance " + sii.instanceUUID + " failed to migrate due to some kernel error." +
                         "Returning to activated state");
                 sii.activateOnServiceInstance();
             }

             // 7. Our job is done.
             return;
         }
         catch (Exception e) {
             try {
                 if (sii != null) {
                     sii.activateOnServiceInstance();
                 }
             }
             catch (IllegalStateTransition e1) {
             }

             log("Capture request failed because of the following exception: " + e);
             if (DEBUG) {
                 e.printStackTrace();
             }

             requestCompleted (jsr, false);
         }
    }

    /**
     * @param jsr
     */
    private void processRestoreStateRequest(JavaServiceRequest jsr)
    {
        // RVMServiceRequestReader.printServiceRequest(jsr);
        boolean success = true;
        ServiceInfo si = null;
        ServiceInstanceInfo serviceInstance = null;

        // In a restore request there is no instance yet and we have to search for a possible
        // ServiceInfo object associated to our service class name.
        try {
            // 1. Find the service class name in the servicesHT.
            si = (ServiceInfo) _servicesHT.get(jsr.classNameForService);
            if (si == null) {
                si = new ServiceInfo (jsr.classNameForService, jsr.classPathForService);

                // Register the service into the global hash table
                _servicesHT.put(jsr.classNameForService, si);
            }

            // 2. Create the RestoreState helper object
            RestoreWorkerThreadState state = new RestoreWorkerThreadState (jsr);

            // 3. Instantiate the service class and store the new instance into the _instancesHT
            serviceInstance = si.restoreServiceInstance (jsr, state);
            _instancesHT.put (jsr.UUID, serviceInstance);

            // 4. Notify the service instance that it is the restoring state
            serviceInstance.restoreOnServiceInstance();

            // 5. The next step of restoration is about service invocations. Every invocation should be
            // restored and associated to a special invoker thread.
            int invocations = state.invocations;
            VM_CompiledMethod[] methodsToBeInvoked = new VM_CompiledMethod[invocations];
            for (int i = 0; i < invocations; i++) {
                methodsToBeInvoked[i] = state.restoreInvocation(i);
            }

            // 6. Notify the service instance that restoring is finished and the restore service instance
            // is now finally activated
            serviceInstance.activateOnServiceInstance();

            // 7. If the service is a thread, start it
            if (serviceInstance.instance instanceof Thread) {
                ((Thread)serviceInstance.instance).start();
            }

            // 8. Now, we can safely start the invoker threads and let them restore the invocations
            for (int i = 0; i < invocations; i++) {
                // TODO: CHANGE IT WHEN WE CAN GET A REAL SERVICE REQUEST FROM THE KERNEL
                JavaServiceRequest invocationRequest = new JavaServiceRequest();
                RestoredServiceInvokerThread thread;
                thread = new RestoredServiceInvokerThread(serviceInstance,
                                                          invocationRequest,
                                                          state.binaryRequests[i],
                                                          methodsToBeInvoked[i]);
                thread.start();
            }

            // Success
            return;
        }
        catch (Exception e) {
            // Undo service instantiation
            if ((si != null) && (serviceInstance != null)) {
                try {
                    serviceInstance.deactivateOnServiceInstance();
                    si.removeServiceInstance(serviceInstance);
                }
                catch (Exception e1) {
                }
                _instancesHT.remove(serviceInstance);
            }

            log("Restoration request failed because of the following exception: " + e);
            if(DEBUG) {
                e.printStackTrace();
            }
            success = false;
        }
        finally {
            // Send result to the client
            requestCompleted (jsr, success);
        }
    }

    /**
     * This method creates a new Connector Thread to handle a direct connection service
     * invocation.
     *
     * @param jsr The Java Service Request containing the input stream and output stream to be used
     */
    private void processConnectRequest (JavaServiceRequest jsr)
    {
        try {
            // 1. Find the service instance using the UUID.
            ServiceInstanceInfo sii = (ServiceInstanceInfo) _instancesHT.get(jsr.UUID);
            if (sii == null) {
                log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
                requestCompleted (jsr, false);
                return;
            }

            // 2. Check that the service is a direct connection service
            if (!(sii.instance instanceof DirectConnectionService)) {
                log ("*** ERROR: the service " + jsr.classNameForService +
                     " does not implement the Service Interface. This is required to process the 'Connect' request");
                requestCompleted (jsr, false);
                return;
            }

            // 3. Pass the found instance to a new worker thread
            new ServiceConnectorThread(sii, jsr).start();

            // 4. Our job is done. Now the rest of the service connection is delegated to the worker thread
            return;
        }
        catch (Exception e) {
            log("ConnectService request failed because of the following exception: " + e);
            if(DEBUG) {
                e.printStackTrace();
            }
            requestCompleted (jsr, false);
        }
    }

    /**
     * Method that spawns a new thread to handle a service invocation request
     *
     * @param jsr The invocation request read from the kernel
     */
    private void processInvokeRequest(JavaServiceRequest jsr)
    {
        try{
            // 1. Find the service instance using the UUID.
            ServiceInstanceInfo sii = (ServiceInstanceInfo) _instancesHT.get(jsr.UUID);
            if(sii == null)
            {
                log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
                requestCompleted (jsr, false);
                return;
            }

            // 2. Pass the found instance to a new worker thread
            new ServiceInvokerThread(sii, jsr).start();

            // 3. Our job is done. Now the rest of the invocation is delegated to the worker thread
            return;
        }
        catch (Exception e) {
            log("Invocation request failed because of the following exception: " + e);
            if(DEBUG) {
                e.printStackTrace();
            }
            requestCompleted (jsr, false);
        }
    }

    /**
     * Method that deactivates an instance of a service
     *
     * @param jsr The Java Service Request containing the UUID of the instance to deactivate
     */
    private void processTerminateRequest(JavaServiceRequest jsr)
    {
        boolean success = true;

        try {
            // 1. Find the service instance in the _instancesHT.
            ServiceInstanceInfo sii = (ServiceInstanceInfo) _instancesHT.get(jsr.UUID);
            if (sii == null) {
                 log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
                 success = false;
                 return;
            }

            /* *************************************** NOTE ******************************************************
            *  Deactivation (like activation) is a service request that doesn't need a new thread to be handled. *
            *  It simply removes the service instance object from all the tables, so that further                *
            *  invocations on that instanceUUID will fail.                                                       *
            ******************************************************************************************************/

            // 2. Notify the deactivation request to the ServiceInstanceInfo
            sii.deactivateOnServiceInstance();

            // 3. Remove the instance from both the global _instancesHT and from its ServiceInfo object
            sii.serviceInfo.removeServiceInstance(sii);
            _instancesHT.remove(sii);

            // 4. Success.
            return;
        }
        catch(Exception e) {
            log("Deactivation request failed because of the following exception: " + e);
            if (DEBUG) {
                e.printStackTrace();
            }
            success = false;
        }
        finally {
            // Send result to the client
            requestCompleted (jsr, success);
        }
    }

    /**
     * Method to handle an activation request for a service.
     * The activation phase has three phases:
     * 1) Class loading and service methods retrieval
     * 2) Instantiation
     * 3) Initialization
     * The first one is executed just the first time a service is executed.
     * The third one is executed only if the service implements the Service interface.
     */
    private void processActivateRequest(JavaServiceRequest jsr)
    {
        boolean success = true;
        ServiceInfo si = null;
        ServiceInstanceInfo serviceInstance = null;

        try {
            // 1. Find the service class name in the servicesHT.
            si = (ServiceInfo) _servicesHT.get(jsr.classNameForService);
            if (si == null) {
                si = new ServiceInfo(jsr.classNameForService, jsr.classPathForService);

                // Register the service into the global hash table
                _servicesHT.put(jsr.classNameForService, si);
            }

            /* *************************************** NOTE *******************************************************
             *  Activation (like deactivation) is a service request that doesn't need a new thread to be handled. *
             *  It simply adds a newly created service instance object into some hash tables, so that further     *
             *  invocations on that instanceUUID will use it.                                                     *
             ******************************************************************************************************/

            // 2. Instantiate the service class and store the new instance into the _instancesHT
            // The service instance is generated into the initial "DEPLOYED" state
            serviceInstance = si.newServiceInstance(jsr.UUID);
            _instancesHT.put(jsr.UUID, serviceInstance);

            // 3. Initialize the service instance, if it implements the Service interface
            Object obj = serviceInstance.instance;
            if (obj instanceof Service) {
                Service svc = (Service) obj;
                svc.init (this);
            }

            // 4. Notify the service instance of activation's success
            serviceInstance.activateOnServiceInstance();

            // 5. Success
            return;
        }
        catch (Exception e) {
            // Undo service instantiation
            if ((si != null) && (serviceInstance != null)) {
                try {
                    serviceInstance.deactivateOnServiceInstance();
                    si.removeServiceInstance(serviceInstance);
                }
                catch (Exception e1) {

                }
                _instancesHT.remove(serviceInstance);
            }

            log("Activation request failed because of the following exception: " + e);
            if (DEBUG) {
                e.printStackTrace();
            }

            success = false;
        }
        finally {
            // Send result to the client
            requestCompleted (jsr, success);
        }
    }

    /**
     * Main entry point of the ServiceManager class
     *
     * @param args Unused. No arguments are passed to the service manager
     */
    public static void main(String[] args)
    {
        // First of all, configure the Mobile JikesRVM environment as ACIK
        try {
            MobileJikesRVM.configureJikesRVM(MobileJikesRVM.ACIK);
        }
        catch (UnsupportedJikesRVMEnvironment e) {
            e.printStackTrace();
        }

        EmbeddedRVMServiceManager jsm = EmbeddedRVMServiceManager.getInstance();
        jsm.run();
    }

    /**
     * Get the only instance of this class
     *
     * @return the constructed instance of the service manager
     */
    public synchronized static EmbeddedRVMServiceManager getInstance()
    {
        return _jsmInstance;
    }

    /**
     * Logging function for the ServiceManager
     *
     * @param msg
     */
    private static void log(String msg)
    {
        if (DEBUG) {
            System.out.println("[EmbeddedRVMServiceManager] " + msg);
        }
    }

    // //////////////////////////////////////////////////////////////////////////
    // IMPLEMENTATION OF THE ServiceManager INTERFACE                          //
    // JavaDoc to be provided in ServiceManager.java                           //
    // //////////////////////////////////////////////////////////////////////////

    public String getServiceInstanceUUID (Object serviceInstance)
    {
        Enumeration en = _instancesHT.keys();
        while (en.hasMoreElements()) {
            String uuid = (String) en.nextElement();
            if (_instancesHT.get(uuid) == serviceInstance) {
                return uuid;
            }
        }

        return null;
    }


    public void registerServiceAttribute (Service service, String attrKey, String attrVal)
    {
        String serviceUUID = this.getServiceInstanceUUID(service);
        if (serviceUUID == null) {
            log("registerServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                    + service.getClass().toString());
            return;
        }

        JavaServiceManager.registerServiceAttribute(serviceUUID, attrKey, attrVal);
    }

    public void deregisterServiceAttribute (Service service, String attrKey, String attrVal)
    {
        String serviceUUID = this.getServiceInstanceUUID(service);
        if (serviceUUID == null) {
            log("deregisterServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                    + service.getClass().toString());
            return;
        }

        JavaServiceManager.deregisterServiceAttribute(serviceUUID, attrKey, attrVal);
    }

    public boolean hasServiceAttribute (Service service, String attrKey, String attrVal)
    {
        String serviceUUID = this.getServiceInstanceUUID(service);
        if (serviceUUID == null) {
            log("hasServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                    + service.getClass().toString());
            return false;
        }
        return JavaServiceManager.hasServiceAttribute(serviceUUID, attrKey, attrVal);
    }

    public String startPeerSearch (Service service, byte[] param, long TTL)
    {
        String serviceUUID = this.getServiceInstanceUUID(service);
        return JavaServiceManager.startPeerSearch(serviceUUID, param, TTL);
    }

    public String startPersistentPeerSearch (Service service, byte[] param)
    {
        String serviceUUID = this.getServiceInstanceUUID(service);
        return JavaServiceManager.startPeerSearch(serviceUUID, param, -1);
    }

    public void stopPersistentPeerSearch (Service service, String searchUUID)
    {
        String serviceUUID = this.getServiceInstanceUUID(service);
        JavaServiceManager.stopPeerSearch(serviceUUID, searchUUID);
    }

    public String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout)
    {
        log("findAServiceInstance: Unimplemented method on EmbeddedRVMServiceManager");
        return null;
    }

    public String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout, boolean force)
    {
        log("findAServiceInstance: Unimplemented method on EmbeddedRVMServiceManager");
        return null;
    }

    public Vector findServiceInstances (String serviceName, String attrKey, String attrValue, long timeout, int maxInstanceNum)
    {
        log("findServiceInstances: Unimplemented method on EmbeddedRVMServiceManager");
        return null;
    }

    public GroupManagerProxy getGroupManagerProxy()
    {
          return _gmp;
    }

    public LocalCoordinator getLocalCoordinatorRef()
    {
        log("getLocalCoordinatorRef: Unimplemented method on EmbeddedRVMServiceManager");
        return null;
    }

    public void initLocalCoordinator (String localCoordClassName)
        throws Exception
    {
        log("initLocalCoordinator: Unimplemented method on EmbeddedRVMServiceManager");
    }

    // //////////////////////////////////////////////////////////////////////////
    // INSTANCE FIELDS                                                         //
    // //////////////////////////////////////////////////////////////////////////
    /**
     * The hashtable of all the services currently activated on this service manager
     * It is accessed using the "className" as a key and it returns (if found) an instance
     * of the ServiceInfo class. See ServiceInfo.java for further details.
     */
    private Hashtable _servicesHT;

    /**
     * The global table of all instances in this service manager.
     * It is accessed using the UUID of the service instance and returns one particular Java
     * instance of the desired service (see ServiceInstanceInfo.java).
     */
    private Hashtable _instancesHT;

    /**
     * The proxy to the native Group Manager (shared with the JavaServiceManager)
     */
    private GroupManagerProxy _gmp;

    // /////////////////////////////////////////////////////////////////////////
    // NATIVE METHODS                                                         //
    // /////////////////////////////////////////////////////////////////////////
    private static native int getNextRequest (JavaServiceRequest jsr);
    static native int requestCompleted (JavaServiceRequest jsr, boolean success);
    static native void requestRelocated (JavaServiceRequest jsr, boolean isTryAgain);
    static native int getNextEvent (ServiceEvent se);

    // //////////////////////////////////////////////////////////////////////////
    // STATIC FIELDS                                                           //
    // //////////////////////////////////////////////////////////////////////////
    private static EmbeddedRVMServiceManager _jsmInstance = new EmbeddedRVMServiceManager();
    private static final boolean DEBUG = true;

    // //////////////////////////////////////////////////////////////////////////
    // STATIC INITIALIZER                                                      //
    // //////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary("acinative");
    }
}
