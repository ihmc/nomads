package us.ihmc.aci.kernel;

import us.ihmc.aci.agserve.InvokeResponseEncoder;

import com.ibm.JikesRVM.ThreadCapturedException;
import com.ibm.JikesRVM.VM_CompiledMethod;
import com.ibm.JikesRVM.VM_Reflection;
import com.ibm.JikesRVM.classloader.VM_Method;
import com.ibm.JikesRVM.classloader.VM_TypeReference;

/**
 * Subclass of ServiceInvokerThread implementing special invocation of restored methods
 *
 * @author rquitadamo
 */
class RestoredServiceInvokerThread extends ServiceInvokerThread {

    /**
     * The information needed to restore the service invocation
     *
     * @param sii The instance object
     * @param jsr The JavaServiceRequest restored locally (needed to send the answer back to the client)
     * @param binaryRequest Is this a binary invocation
     * @param method The compiled method to invoke through JikesRVM reflection
     */
    public RestoredServiceInvokerThread(ServiceInstanceInfo sii, JavaServiceRequest jsr,
                                        boolean binaryRequest, VM_CompiledMethod method) {
        super(sii, jsr);
        isBinaryRequest = binaryRequest;       
        methodToInvoke = method;
    }

    /**
     * Lifecycle method, overriding the one from ServiceInvokerThread
     */
    public void run()
    {
         String methodName = null;
         boolean success = true;
         boolean relocated = false;

         try
         {
             /* *** Notify the service instance that an invocation is about to be performed ***** */
             serviceInstance.invokeOnServiceInstance();
             /* ********************************************************************************* */

             if (methodToInvoke != null)
             {
                 methodName = methodToInvoke.getMethod().getName().toString();

                 log ("re-establishing call stack for method " + methodName + " in service " + serviceInstance.serviceInfo.serviceClassName + " and continuing...");

                 // 1. Prepare the empty arguments to reflectively invoke the restored method
                 Object[] args = getServiceInvocationParameters(methodToInvoke);

                 // 2. Invoke the specialized method through JikesRVM's reflection
                 Object response = VM_Reflection.invoke(methodToInvoke, serviceInstance.instance, args);

                 // Asynchronous requests do not need responses
                 if (request.asynchronous) {
                     return;
                 }
                 
                 // Synchronous requests need response instead
                 InvokeResponseEncoder encoder = new InvokeResponseEncoder();
                 encoder.encode (methodName, response, isBinaryRequest, request.sos);
             }
             else {
                 log ("No method was re-established by JikesRVM for method " + methodName + " in service " + serviceInstance.serviceInfo.serviceClassName);
                 success = false;
             }
         }
         catch(ThreadCapturedException e)
         {
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
        	 if(request.asynchronous) {
	             if (relocated) {
	                 EmbeddedRVMServiceManager.requestRelocated(request, request.isTryAgain);
                     }
	             else {
	                 EmbeddedRVMServiceManager.requestCompleted(request, success);
                     }
        	 }
             // Unregister this worker
             serviceInstance.invokers.remove(this);
         }
    }

    /**
     * Method to create an empty array of parameters to pass to JikesRVM reflection
     *
     * @param cm The restored method to invoke
     *
     * @return The created array of method arguments
     */
    private Object[] getServiceInvocationParameters(VM_CompiledMethod cm)
    {
        VM_Method method = cm.getMethod();
        VM_TypeReference[] params =  method.getParameterTypes();
        Object[] objArgs = new Object[params.length];

        for(int i=0; i<params.length; i++)
        {
            objArgs[i]=null; // TEMPORARILY SET TO NULL. IT SHOULD BE OK
        }

        return objArgs;
    }

    // //////////////////////////////////////////////////////////////////////////
    // INSTANCE FIELDS                                                         //
    // //////////////////////////////////////////////////////////////////////////

    /**
     * The method restored by the VM_FrameInstaller to be invoke through JikesRVM's Reflection
     */
    transient VM_CompiledMethod methodToInvoke;
}
