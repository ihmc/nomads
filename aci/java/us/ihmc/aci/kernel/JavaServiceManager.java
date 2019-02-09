/**
 * JavaServiceManager
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.kernel;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import us.ihmc.aci.agserve.AgServeUtils;
import us.ihmc.aci.agserve.InvokeRequestParser;
import us.ihmc.aci.agserve.InvokeResponseEncoder;
import us.ihmc.aci.grpMgrOld.GroupManagerProxy;
import us.ihmc.aci.service.DirectConnectionService;
import us.ihmc.aci.service.Service;
import us.ihmc.algos.Statistics;

import us.ihmc.util.ThreadPool;

/**
 *
 */
public class JavaServiceManager extends ServiceManager
    implements ThreadPool.ThreadPoolMonitor
{
    private JavaServiceManager()
    {
        _requestThreadPool = new ThreadPool (100);
        _servicesHT = new Hashtable();
        _gmp = new GroupManagerProxyJSMImpl();

        (new JavaEventHandlerThread()).start();

        if (BENCHMARK) {
            _parsingStats = new Statistics();
            _invokeStats = new Statistics();
            _encodeStats = new Statistics();
            _findMethodStats = new Statistics();
        }
    }

    /**
     * Part of the ThreadPool.ThreadPoolMonitor interface.
     */
    public void runFinished (Runnable r, Exception ex)
    {
    }

    /**
     *
     */
    public GroupManagerProxy getGroupManagerProxy()
    {
        return _gmp;
    } //getGroupManagerProxy()

    /**
     *
     */
    public void run()
    {
        while (true) {
            JavaServiceRequest jsr = new JavaServiceRequest (this);
            getNextRequest (jsr);
            log ("run() :: got a JavaServiceRequest. enqueueing.");

            _requestThreadPool.enqueue (jsr, this);
        }
    } //run()

    /**
     *
     */
    public void processRequest (JavaServiceRequest jsr)
    {
        try {
            if (jsr.requestType == ServiceRequest.REQUEST_TYPE_ACTIVATE) {
                this.processActivateRequest (jsr);
            }
            else if (jsr.requestType == ServiceRequest.REQUEST_TYPE_TERMINATE) {
                this.processTerminateRequest (jsr);
            }
            else if (jsr.requestType == ServiceRequest.REQUEST_TYPE_INVOKE) {
                this.processInvokeRequest (jsr);
            }
            else if (jsr.requestType == ServiceRequest.REQUEST_TYPE_CONNECT) {
                this.processConnectRequest (jsr);
            }
            else {
                log ("Unknown request type " + jsr.requestType + " received for service");
                requestCompleted (jsr, false);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    } //processRequest()

    /**
     *
     */
    public void processEvent (ServiceEvent se)
    {
        switch (se.eventType) {
            case ServiceEvent.NEW_PEER :
                _gmp.newPeer (se.nodeUUID);
                break;

            case ServiceEvent.DEAD_PEER :
                _gmp.deadPeer (se.nodeUUID);
                break;

            case ServiceEvent.GROUP_LIST_CHANGE :
                _gmp.groupListChange (se.nodeUUID);
                break;

            case ServiceEvent.NEW_GROUP_MEMBER :
                _gmp.newGroupMember (se.groupName, se.memberUUID, se.data);
                break;

            case ServiceEvent.GROUP_MEMBER_LEFT :
                _gmp.groupMemberLeft (se.groupName, se.memberUUID);
                break;

            case ServiceEvent.CONFLICS_WITH_PRIVATE_PEER_GROUP :
                _gmp.conflictWithPrivatePeerGroup (se.groupName, se.nodeUUID);
                break;

            case ServiceEvent.PEER_GROUP_DATA_CHANGED :
                _gmp.peerGroupDataChanged (se.groupName, se.nodeUUID, se.data);
                break;

            case ServiceEvent.PEER_SEARCH_REQUEST_RECEIVED :
                _gmp.peerSearchRequestReceived (se.groupName, se.nodeUUID, se.searchUUID, se.param);
                break;

            case ServiceEvent.PEER_SEARCH_RESULT_RECEIVED :
                _gmp.peerSearchResultReceived (se.groupName, se.nodeUUID, se.searchUUID, se.param);
                break;

            case ServiceEvent.PEER_MESSAGE_RECEIVED :
                _gmp.peerMessageReceived (se.groupName, se.nodeUUID, se.data);

            case ServiceEvent.PERSISTENT_PEER_SEARCH_TERMINATED :
                _gmp.persistentPeerSearchTerminated (se.groupName, se.nodeUUID, se.searchUUID);
                break;

            default:
                log ("processEvent:: WARNING: Unknown type for event.");
        }
    } //processEvent

    /**
     *
     */
    public String getServiceInstanceUUID (Object serviceInstance)
    {
        Enumeration en = _servicesHT.keys();
        while (en.hasMoreElements()) {
            String uuid = (String) en.nextElement();
            if (_servicesHT.get(uuid) == serviceInstance) {
                return uuid;
            }
        }

        return null;
    }

    /**
     *
     */
    public void registerServiceAttribute (Service service, String attrKey, String attrVal)
    {
        String serviceUUID = this.getServiceInstanceUUID (service);
        if (serviceUUID == null) {
            log ("registerServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                 + service.getClass().toString());
            return;
        }

        registerServiceAttribute (serviceUUID, attrKey, attrVal);
    }

    /**
     *
     */
    public void deregisterServiceAttribute (Service service, String attrKey, String attrVal)
    {
        String serviceUUID = this.getServiceInstanceUUID (service);
        if (serviceUUID == null) {
            log ("deregisterServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                 + service.getClass().toString());
            return;
        }

        deregisterServiceAttribute (serviceUUID, attrKey, attrVal);
    }

    /**
     *
     */
    public boolean hasServiceAttribute (Service service, String attrKey, String attrVal)
    {
        String serviceUUID = this.getServiceInstanceUUID (service);
        if (serviceUUID == null) {
            log ("hasServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                 + service.getClass().toString());
            return false;
        }
        return hasServiceAttribute (serviceUUID, attrKey, attrVal);
    }

    /**
     *
     */
    public String startPeerSearch (Service service, byte[] param, long TTL)
    {
        String serviceUUID = this.getServiceInstanceUUID (service);
        return startPeerSearch (serviceUUID, param, TTL);
    }

    /**
     *
     */
    public String startPersistentPeerSearch (Service service, byte[] param)
    {
        String serviceUUID = this.getServiceInstanceUUID (service);
        return startPeerSearch (serviceUUID, param, -1);
    }

    /**
     *
     */
    public void stopPersistentPeerSearch (Service service, String searchUUID)
    {
        String serviceUUID = this.getServiceInstanceUUID (service);
        stopPeerSearch (serviceUUID, searchUUID);
    }


    /**
     *
     */
    public synchronized static JavaServiceManager getInstance()
    {
        return _jsmInstance;
    }

    /**
     *
     */
    public LocalCoordinator getLocalCoordinatorRef()
    {
        return _localCoordRef;
    }

    /**
     *
     */
    public void initLocalCoordinator (String localCoordClassName)
        throws Exception
    {
        log ("initLocalCoordinator:: localCoordClassName = " + localCoordClassName);

        try {
            Class cl = Class.forName (localCoordClassName);
            Object obj = cl.newInstance();
            _localCoordRef = (LocalCoordinator) obj;
            _localCoordRef.init (this, new KernelProxy());
        }
        catch (Exception ex) {
            log ("error on initLocalCoordinator():");
            ex.printStackTrace();
            throw ex;
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    private void processActivateRequest (JavaServiceRequest jsr)
    {
        try {
            log ("Activate Request received for service");
            log ("Classpath = " + jsr.classPathForService);

            File libDir = new File (jsr.classPathForService + "/lib");
            File[] libFileList = libDir.listFiles (new JarFileFilter());

            File externalsDir = new File (jsr.classPathForService + "/externals");
            File[] externalsFileList = externalsDir.listFiles (new JarFileFilter());

            int size = 1 + ((libFileList != null) ? libFileList.length : 0);
            size += ((externalsFileList != null) ? externalsFileList.length : 0);
            URL[] urlList = new URL[size];
            int i = 0;
            urlList[i++] = (new File (jsr.classPathForService)).toURI().toURL();

            if (libFileList != null) {
                for (int j = 0; j < libFileList.length; j++){
                    urlList[i++] = libFileList[j].toURI().toURL();
                }
            }
            if (externalsFileList != null) {
                for (int j = 0; j < externalsFileList.length; j++){
                    urlList[i++] = externalsFileList[j].toURI().toURL();
                }
            }

            URLClassLoader cl = new URLClassLoader (urlList);
            if (DEBUG) {
                URL[] urls = cl.getURLs();
                for (int j = 0; j < urls.length; j++) {
                    System.out.println ("Classpath URL[" + j + "] " + urls[j].toString());
                }
            }

            Class c = cl.loadClass (jsr.classNameForService);
            Object serviceInstance = c.newInstance();
            _servicesHT.put (jsr.UUID, serviceInstance);

            if (serviceInstance instanceof Service) {
                Service svc = (Service) serviceInstance;
                svc.init (this);
            }

            log ("Calling requestCompleted()");
            requestCompleted (jsr, true);
        }
        catch (Exception ex) {
            ex.printStackTrace();
            requestCompleted (jsr, false);
        }
    } //processActivateRequest()

    /**
     *
     */
    private void processTerminateRequest (JavaServiceRequest jsr)
    {
        log ("Termination Request received for service " + jsr.UUID);

        Object serviceInstance = _servicesHT.remove (jsr.UUID);
        if (serviceInstance == null) {
            log ("*** (RequestTerminate) WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
            requestCompleted (jsr, false);
            return;
        }

        Method m = findDeactivateMethod (serviceInstance.getClass());
        if (m != null) {
            log ("deactivate method in class " + serviceInstance.getClass().toString() + " found... invoking.");
            try {
                m.invoke (serviceInstance, new Object[0]);
            }
            catch (Exception ex) {
                log("problem invoking " + serviceInstance.getClass().toString() + ".deactivate():");
                ex.printStackTrace();
                requestCompleted (jsr, false);
                return;
            }
        }
        else {
            log ("'deactivate' method not found in class " + serviceInstance.getClass().toString());
        }

        requestCompleted (jsr, true);
    } //processTerminateRequest()

    /**
     *
     */
    private void processInvokeRequest (JavaServiceRequest jsr)
    {
        log ("Invocation Request received for service " + jsr.UUID + " methodName = " + jsr.methodName + " asynchronous = " + jsr.asynchronous);
        long timestamp;

        Object serviceInstance = _servicesHT.get (jsr.UUID);
        if (serviceInstance == null) {
            log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
            requestCompleted (jsr, false);
            return;
        }

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis();
        } //if (BENCHMARK)

        ClassLoader cl = serviceInstance.getClass().getClassLoader();
        InvokeRequestParser parser;

        try{
            parser = new InvokeRequestParser (jsr.sis, cl);
        }
        catch (IOException ex) {
            log("Problems reading the Dime from the InputStream");
            requestCompleted(jsr, false);
            return;
        }

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis() - timestamp;
            if (!_firstInvocation) {
                _parsingStats.update (timestamp);
            }
        } //if (BENCHMARK)

        String methodName = parser.isBinaryRequest() ? jsr.methodName : parser.getMethodName();
        Object[] paramValues = parser.getParameterValues();

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis();
        } //if (BENCHMARK)

        Method methodToInvoke = findMatchingMethod (serviceInstance.getClass(),
                                                    methodName,
                                                    parser.getParameterTypes());

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis() - timestamp;
            if (!_firstInvocation) {
                _findMethodStats.update (timestamp);
            }
        } //if (BENCHMARK)

        if (methodToInvoke != null) {
            log ("found a matching method! Now Invoking...");
            try {
                if (BENCHMARK) {
                    timestamp = System.currentTimeMillis();
                } //if (BENCHMARK)

                if (jsr.asynchronous) {
                    requestCompleted (jsr, true);
                }
                
                //CPUAccountManager capture (J-RAF2)
                Thread currentThread = Thread.currentThread();
                Object threadCPUAccount = null;
                Object currentManager = null;
                Method method = null;
                try {
                	Field fieldAux = currentThread.getClass().getField("org_jraf2_cpuAccount");
                	threadCPUAccount = fieldAux.get(currentThread);
                	jraf2Enabled = true;
                	System.out.println("=== JRaf2 Enabled ===");
                } 
                catch(NoSuchFieldException ex) {
                	System.out.println("=== JRaf2 not found ===");
                	jraf2Enabled = false;
                }  
                //Profile Data reset (J-RAF2)
                if (jraf2Enabled == true) {
                	try {                 			
                		method = threadCPUAccount.getClass().getMethod("getManager", (Class[])null);
                		currentManager = method.invoke(threadCPUAccount, (Object[])null);
                		method = currentManager.getClass().getMethod("resetMonitoringData", (Class[])null);
                		method.invoke(currentManager, (Object[])null);                		                		                		                              		
                	} 
                	catch(NoSuchMethodException ex) {
                		ex.printStackTrace();
                	}                	
                }                 
                
                Object response = methodToInvoke.invoke (serviceInstance, paramValues);

                //Profile Data capture (J-RAF2)                         
                if (jraf2Enabled == true) {
                   	try {                   		
                   		method = currentManager.getClass().getMethod("getMonitoringData", (Class[])null);
                   		_data = new long[2];
                   		_data = (long[])method.invoke(currentManager, (Object[])null);              	
                   	} 
                   	catch(NoSuchMethodException ex) 
                   	{
                  		ex.printStackTrace();
                	}               	
                 }
                
                if (jsr.asynchronous) {
                	//Profile Data saving
                	if (jraf2Enabled == true) {
                		System.out.println("=== Profile Data saving... - asynchronous -) ===");
                		//1: Java           
                		updateInvocationProfileData(1, serviceInstance.getClass().getName(), jsr.UUID, methodName, methodToInvoke.toString(),jsr.requestorNodeUUID, _gmp.getNodeUUID(),
                										_data[0], _data[1], jsr.sis.getTotalBytesRead(), jsr.sos.getTotalBytesWritten());		
                	}
                    return;
                }

                if (BENCHMARK) {
                    timestamp = System.currentTimeMillis() - timestamp;
                    if (!_firstInvocation) {
                        _invokeStats.update (timestamp);
                    }
                    timestamp = System.currentTimeMillis();
                } //if (BENCHMARK)

                InvokeResponseEncoder encoder = new InvokeResponseEncoder();
                encoder.encode (methodName, response, parser.isBinaryRequest(), jsr.sos);

                if (BENCHMARK) {
                    timestamp = System.currentTimeMillis() - timestamp;
                    if (!_firstInvocation) {
                        _encodeStats.update(timestamp);
                    }
                } //if (BENCHMARK)

                try {
                    requestCompleted (jsr, true);
                }
                catch (Exception ex) {
                    log ("Problems writing out the response: ");
                    requestCompleted (jsr, false);
                    ex.printStackTrace();
                }

                if (BENCHMARK) {
                    if (!_firstInvocation) {
                        System.out.println("JavaServiceManager:: avg. time to parse the request:: "
                                           + _parsingStats.getAverage() + " (stdev: " + _parsingStats.getStDev() + ")");

                        System.out.println("JavaServiceManager:: AgServerUtils:: avg. time to deserialize the parameters:: "
                                           + AgServeUtils._deserializationStats.getAverage() + " (stdev: " + AgServeUtils._deserializationStats.getStDev() + ")");

                        System.out.println("JavaServiceManager:: InvokeReqParser:: avg. time to read/parse DIME:: "
                                           + InvokeRequestParser._dimeParsingStats.getAverage() + " (stdev: " + InvokeRequestParser._dimeParsingStats.getStDev() + ")");

                        System.out.println("JavaServiceManager:: avg. time to encode the reply:: "
                                           + _encodeStats.getAverage() + " (stdev: " + _encodeStats.getStDev() + ")");

                        System.out.println("JavaServiceManager:: avg. time on invocation:: "
                                           + _invokeStats.getAverage() + " (stdev: " + _invokeStats.getStDev() + ")");

                        System.out.println("JavaServiceManager:: avg. time to find the method:: "
                                           + _findMethodStats.getAverage() + " (stdev: " + _findMethodStats.getStDev() + ")");
                    }
                    else {
                        _firstInvocation = false;
                    }
                } //if (BENCHMARK)

                if (jraf2Enabled == true) {
                	//Profile Data saving...
                	System.out.println("=== Profile Data saving... - synchronous -) ===");
            			updateInvocationProfileData(1, serviceInstance.getClass().getName(), jsr.UUID, methodName, methodToInvoke.toString(), jsr.requestorNodeUUID, _gmp.getNodeUUID(), _data[0], _data[1], jsr.sis.getTotalBytesRead(), jsr.sos.getTotalBytesWritten());
                }
                return;
            }
            catch (Exception ex) {
                log ("Problem invoking the " + serviceInstance.getClass() + "." + methodName + " method:");
                requestCompleted (jsr, false);
                ex.printStackTrace();
            }
        }
        else {
            log ("did not find a matching method (" + methodName + ") in " + serviceInstance.getClass().toString());
            requestCompleted (jsr, false);
        }
    } //processInvokeRequest()

    /**
     *
     */
    private void processConnectRequest (JavaServiceRequest jsr)
    {
        log ("ConnectRequest received for service " + jsr.UUID);

        Object serviceInstance = _servicesHT.get (jsr.UUID);
        if (serviceInstance == null) {
            log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
            requestCompleted (jsr, false);
            return;
        }

        if (!(serviceInstance instanceof DirectConnectionService)) {
            log ("*** ERROR: the service " + jsr.classNameForService +
                 " does not implement the Service Interface. This is required to process the 'Connect' request");
            requestCompleted (jsr, false);
        }

        DirectConnectionService directConnService = (DirectConnectionService) serviceInstance;
        requestCompleted (jsr, true);

        try {
            jsr.sis._deleteReader = true;
            jsr.sos._deleteWriter = true;
            directConnService.handleConnection (jsr.sis, jsr.sos);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }

        log ("ConnectRequest:: service [" + jsr.UUID + "] returned from handleConnection().");
    } //processConnectRequest()

    /**
     *
     */
    private static void log (String msg)
    {
        if (DEBUG) {
            System.out.println("[JavaServiceManager] " + msg);
        }
    }
    
    //Method to create Java signature of a method.
/*    private String createSignature(Method method)
    {
    	Class[] parameters = method.getParameterTypes();
    	String parameterName;
       	Class returned = method.getReturnType();
       	String returnedName;
    	String signature = "(";
    	String stringAux;
    	
    	if(parameters.length == 0)
    		signature = signature.concat("V");
    	
    	for(int i=0; i<parameters.length;i++)
    	{
    		parameterName = parameters[i].getName();
    		
    		//if (parameters[i].isArray())
    		//	signature = signature.concat("[");
 		
    		if (parameters[i].isPrimitive()) 
    		{
    			if (parameterName == "boolean")
    				signature = signature.concat("Z");
    			if (parameterName == "byte")
    				signature = signature.concat("B");
    			if (parameterName == "char")
    				signature = signature.concat("C");
    			if (parameterName == "double")
    				signature = signature.concat("D");
    			if (parameterName == "float")
    				signature = signature.concat("F");
    			if (parameterName == "int")
    				signature = signature.concat("I");
    			if (parameterName == "long")
    				signature = signature.concat("J");
    			if (parameterName == "object")
    				signature = signature.concat("L");
    			if (parameterName == "short")
    				signature = signature.concat("S");
    			if (parameterName == "void")
    				signature = signature.concat("V");
    			if (parameterName == "array")
    				signature = signature.concat("[");	
    		}
    		else 
    		{
				stringAux = parameterName.replace(".", "/");
	    		signature = signature.concat("L" + stringAux.toUpperCase() + ";");
    		}
	
    	}
    	signature = signature.concat(")");
    	
    	returnedName = returned.getName();
		
    	//if (returned.isArray())
		//	signature = signature.concat("[");
		
    	if (returned.isPrimitive())
    	{
    		if (returnedName == "boolean")
    			signature = signature.concat("Z");
    		if (returnedName == "byte")
    			signature = signature.concat("B");
    		if (returnedName == "char")
    			signature = signature.concat("C");
    		if (returnedName == "double")
    			signature = signature.concat("D");
    		if (returnedName == "float")
    			signature = signature.concat("F");
    		if (returnedName == "int")
    			signature = signature.concat("I");
    		if (returnedName == "long")
    			signature = signature.concat("J");
    		if (returnedName == "object")
    			signature = signature.concat("L");
    		if (returnedName == "short")
    			signature = signature.concat("S");
    		if (returnedName == "void")
    			signature = signature.concat("V");
    		if (returnedName == "array")
    			signature = signature.concat("[");		
    	}
		else 
		{
			stringAux = returnedName.replace(".", "/");
    		signature = signature.concat(stringAux.toUpperCase());
		}
		System.out.println("=========================" + signature + "================================");
		System.out.println("=========================" + method.toGenericString() + "================================");
    	
    	return signature;
    }*/

    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    public static class JarFileFilter implements FilenameFilter
    {
        public boolean accept (File dir, String name)
        {
            return name.endsWith(".jar");
        }
    } //class JarFileFilter

    /**
     *
     */
    public class JavaEventHandlerThread extends Thread
    {
        public JavaEventHandlerThread()
        {
            setName ("JavaEventHandlerThread-[" + getName() + "]");
        }

        /**
         *
         */
        public void run()
        {
            ServiceEvent se = new ServiceEvent();

            while (true) {
                try {
                    getNextEvent (se);
                    processEvent (se);
                }
                catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        }
    } //class JavaEventHandlerThread

    // /////////////////////////////////////////////////////////////////////////
    // NATIVE METHODS //////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    private static native void getNextRequest (JavaServiceRequest jsr);
    private static native void getNextEvent (ServiceEvent se);
    //private static native void requestCompleted (JavaServiceRequest jsr, byte[] result, boolean success);
    private static native void requestCompleted (JavaServiceRequest jsr, boolean success);

    public native String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout);
    public native String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout, boolean force);
    public native Vector findServiceInstances (String serviceName, String attrKey, String attrValue, long timeout, int maxInstanceNum);
    static native void registerServiceAttribute (String serviceUUID, String attrKey, String attrVal);
    static native void deregisterServiceAttribute (String serviceUUID, String attrKey, String attrVal);
    static native boolean hasServiceAttribute (String serviceUUID, String attrKey, String attrVal);
    static native String startPeerSearch (String serviceUUID, byte[] param, long TTL);
    static native void stopPeerSearch (String serviceUUID, String searchUUID);
    
    //Native method to save profile data (J-RAF2)
    private static native int updateInvocationProfileData (int flagContainer, String serviceName, String serviceUUID, String methodName, String methodSignature,
    														String requestorNodeUUID, String nodeUUID, long executionTime, long bytecodesUsed, long totalBytesRead,
    														long totalBytesWritten);


    // /////////////////////////////////////////////////////////////////////////
    private static final boolean DEBUG = true;
    public static final int NUM_WORKER_THREADS = 100;

    private ThreadPool _requestThreadPool;

    private Hashtable _servicesHT;
    private GroupManagerProxy _gmp;

    private LocalCoordinator _localCoordRef = null;

    private static JavaServiceManager _jsmInstance = new JavaServiceManager();
    
    // ---- used for picking up profile data ----
    private boolean jraf2Enabled = false;
    private long _data[];	//to pick up resource utilization data (time and bytecodes) from JRAF-2
    // ----

    // ---- used for benchmarking stuff ----
    protected static final boolean BENCHMARK = false;
    private boolean _firstInvocation = true;
    private Statistics _parsingStats, _invokeStats, _encodeStats, _findMethodStats;
    // ----

    // /////////////////////////////////////////////////////////////////////////
    // MAIN METHOD. to be invoked by the C++ JavaVMContainer. //////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     *
     */
    public static void main (String[] args)
    {
        JavaServiceManager jsm = JavaServiceManager.getInstance();
        jsm.run();
    } //main()

    // /////////////////////////////////////////////////////////////////////////
    // STATIC INITIALIZER //////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary ("acinative");
    }
}
