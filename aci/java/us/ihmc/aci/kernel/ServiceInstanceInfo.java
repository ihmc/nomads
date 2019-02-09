package us.ihmc.aci.kernel;

import java.util.Vector;

import com.ibm.JikesRVM.ThreadCapturedException;

/**
 * This class is the wrapper around each instance of a certain service.
 * It's useful for several reasons:
 * - it gives the service manager the possibility to set limits on resource utilization (TODO)
 * - it allows to collect statistics about the resource utilization (TODO)
 * - it manages the worker threads issuing invocations o direct connections on a certain instance
 * - it handles the possible states of the service, avoiding unsafe or illegal transitions.
 *
 * @author rquitadamo
 */
class ServiceInstanceInfo implements ServiceInstanceStates
{

    /**
     * Main constructor
     *
     * @param serviceInstanceUUID The UUID assigned to this instance by the ACI kernel
     * @param obj The object instance created by the VM
     */
    ServiceInstanceInfo(String serviceInstanceUUID, Object obj, ServiceInfo service) {

        // 1. Set instance fields
        instance = obj;
        instanceUUID = serviceInstanceUUID;
        serviceInfo = service;

        // 2. Create the vector to keep track of invokers and connectors
        invokers = new Vector(1);

        // 3. Initial state is "deployed". There's no need to notify anyone because no one has
        // a reference to this service instance yet
        state = DEPLOYED;
    }

    synchronized void activateOnServiceInstance() throws IllegalStateTransition
    {
        // 1. Validate state transition for this instance
        if((state != DEPLOYED) && (state != RESTORING) && (state != MIGRATING))
        {
            throw new IllegalStateTransition("Service " + instanceUUID + "cannot be actived because is the state " + stateNames[state]);
        }

        // before returning change the state to "activated"
        state = ACTIVATED;
        notifyAll();
    }

    synchronized void deactivateOnServiceInstance() throws IllegalStateTransition
    {
        // 1. Validate state transition for this instance
        if(state != ACTIVATED)
        {
            throw new IllegalStateTransition("Service " + instanceUUID + "cannot be deactived because is the state " + stateNames[state]);
        }

        // TEMPORARY: Simply do nothing. All the referenced objects will become garbage soon.

        // before returning change the state to "deactivated" and notify all threads waiting!
        state = DEACTIVATED;
        notifyAll();
    }

    synchronized void migratingOnServiceInstance() throws IllegalStateTransition
    {
        // 1. If the instance is not in the "activated" state, it can however return
        // to that soon. Just wait to handle this request.
        while((state == MIGRATING)||(state == RESTORING)||(state == DEPLOYED))
        {
            try {
                wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        // 2. Have we reached a legal state?
        if(state != ACTIVATED)
        {
            throw new IllegalStateTransition("Service " + instanceUUID + "cannot be migrated because is the state " + stateNames[state]);
        }

        // 3. Ok. Change the state and notify all
        state = MIGRATING;
        notifyAll();
    }

    synchronized void migratedOnServiceInstance() throws IllegalStateTransition
    {
        // 1. Are we in a legal state?
        if(state != MIGRATING)
        {
            throw new IllegalStateTransition("Service " + instanceUUID + "cannot be migrated because is the state " + stateNames[state]);
        }

        // 2. Ok. Change the state and notify all
        state = MIGRATED;
        notifyAll();
        
        //3. Unlock possible other threads using the MobileService interface
        if(instance instanceof MobileService)
            ((MobileService) instance).onServiceMigrated();
    }

    synchronized void restoreOnServiceInstance() throws IllegalStateTransition
    {
        // 1. Are we in a legal state?
        if(state != DEPLOYED)
        {
            throw new IllegalStateTransition("Service " + instanceUUID + "cannot be restored because is the state " + stateNames[state]);
        }

        // 2. Ok. Change the state and notify all
        state = RESTORING;
        notifyAll();
    }

    synchronized void connectOnServiceInstance() throws IllegalStateTransition
    {
        // 1. If the instance is not in the "activated" state, it can however return
        // to it soon. Just wait to handle this request.
        while((state == MIGRATING)||(state == RESTORING)||(state == DEPLOYED))
        {
            try {
                wait();
            } catch (InterruptedException e) {

            }
        }

        // 2. Have we reached a legal state?
        if(state == MIGRATED)
        {
            throw new ThreadCapturedException();
        }
        else if(state != ACTIVATED)
        {
            throw new IllegalStateTransition("Service " + instanceUUID + "cannot accept connection requests because is the state " + stateNames[state]);
        }

        // 3. Ok, no changes in the state are needed. The instance remains in the "activated" state
        // So there is no need to notify anyone!
    }

    synchronized void invokeOnServiceInstance() throws IllegalStateTransition
    {
        // 1. If the instance is not in the "activated" state, it can however return
        // to it soon. Just wait to handle this request.
        while((state == MIGRATING)||(state == RESTORING)||(state == DEPLOYED))
        {
            try {
                wait();
            } catch (InterruptedException e) {

            }
        }

        // 2. Have we reached a legal state?
        if(state == MIGRATED)
        {
            throw new ThreadCapturedException();
        }
        else if(state != ACTIVATED)
        {
            throw new IllegalStateTransition("Service " + instanceUUID + "cannot accept invoke requests because is the state " + stateNames[state]);
        }

        // 3. Ok, no changes in the state are needed. The instance remains in the "activated" state
        // So there is no need to notify anyone!
    }

    synchronized int countTryAgainInvokers()
    {
        int i = 0, num = 0;
        for (i = 0; i < invokers.size(); i++)
        {
            if(((WorkerThread)invokers.get(i)).isTryAgainInvocation())
                num ++;
        }

        return num;
    }

    // //////////////////////////////////////////////////////////////////////////
    // INSTANCE FIELDS                                                         //
    // //////////////////////////////////////////////////////////////////////////

    /**
     * The UUID assigned by the ACI kernel to this instance
     */
    String instanceUUID;

    /**
     * The actual instance created by the VM
     */
    Object instance;

    /**
     * The pool of invokers currently performing service invocations on this instance
     */
    Vector invokers;

    /**
     * The reference to the ServiceInfo object from which this instance was created
     */
    ServiceInfo serviceInfo;

    /**
     * The current state of the service instance
     */
    int state;

}
