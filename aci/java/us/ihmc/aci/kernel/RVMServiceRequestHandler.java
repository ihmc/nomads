package us.ihmc.aci.kernel;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;


import us.ihmc.aci.agserve.AgServeUtils;
import us.ihmc.aci.agserve.InvokeRequestParser;
import us.ihmc.aci.agserve.InvokeResponseEncoder;
import us.ihmc.aci.kernel.JavaServiceManager.JarFileFilter;
import us.ihmc.aci.kernel.RVMServiceManager.ServiceManagerStatistics;
import us.ihmc.aci.service.DirectConnectionService;
import us.ihmc.aci.service.Service;

import com.ibm.JikesRVM.*;

class RVMServiceRequestHandler extends Thread
{

    private static final boolean DEBUG = true;
    static final boolean BENCHMARK = false;
    private static final int MAX_STATE_BUF = 65536;

    private RVMServiceRequestReader _reader;
    private ServiceManagerStatistics _statsObj;
    private RVMServiceManager _serviceManager;
    private boolean _stop;

    RVMServiceRequestHandler(RVMServiceRequestReader r, RVMServiceManager sm)
    {
        _reader = r;
        _statsObj = sm._statsObject;
        _serviceManager = sm;
        _stop = false;
    }

    void stopHandler()
    {
        _stop = true;
    }

    public void run()
    {
        JavaServiceRequest sr = null;
        while (!_stop) {
            try {
                sr = (JavaServiceRequest) _reader.getNextRequest();
                if (sr != null) {
                    processRequest(sr);
                }
            }
            catch (IOException e) {
                e.printStackTrace();
            }
            finally {
                // ****** Cleaning up the current ServiceRequest ************
                try {
                    if ((sr != null) && (sr.sis != null) && (sr.sos != null)) {
                        sr.sis.close();
//                      sr.sos.close();
                        sr = null; // sr now becomes garbage!
                    }
                }
                catch (IOException e) {
                }
                // ******************************************************************
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    private void processRequest (JavaServiceRequest jsr) throws IOException
    {
        switch(jsr.requestType)
        {
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
            requestCompleted (jsr, null, false);
        }
    }


    private void requestCompleted (JavaServiceRequest jsr, byte[] result, boolean success) throws IOException
    {
        byte[] buffer = null;

        if ((jsr.requestType == ServiceRequest.REQUEST_TYPE_CAPTURE_STATE) || (jsr.requestType == ServiceRequest.REQUEST_TYPE_RESTORE_STATE))
        {
             jsr.stateBuffer = result;
        }
        else
        {
            if (success) {
                 if (jsr.requestType == ServiceRequest.REQUEST_TYPE_INVOKE) {
                     if(!jsr.asynchronous)
                     {
                         buffer = "HTTP/1.1 200 OK\r\n\r\n".getBytes();
                         jsr.sos.arrayWrite(buffer, 0, buffer.length);

                         buffer = "Content-Type: dime\r\n".getBytes();
                         jsr.sos.arrayWrite(buffer, 0, buffer.length);

                         buffer = ("Content-Length: "+result.length+"\r\n\r\n").getBytes();
                         jsr.sos.arrayWrite(buffer, 0, buffer.length);

                         jsr.sos.arrayWrite(result, 0, result.length);
                         jsr.sos.flush();
                     }
                     else { // asynchronous requests are not supposed to be answered
                         // do nothing
                     }
                 }
                 else if(jsr.requestType == ServiceRequest.REQUEST_TYPE_CONNECT)
                 {
                     buffer = ("HTTP/1.1 200 OK\r\n\r\n").getBytes();
                     jsr.sos.arrayWrite(buffer, 0, buffer.length);
                     jsr.sos.flush();
                 }
             }
             else {
                 buffer = ("HTTP/1.1 501 Internal Server Error\r\n").getBytes();
                 jsr.sos.arrayWrite(buffer, 0, buffer.length);
                 jsr.sos.flush();
             }
        }

         // Notify the aci kernel!!!
         _reader.notifyRequestCompleted(jsr, success);
    }

    /**
     *
     */
    private void processActivateRequest (JavaServiceRequest jsr) throws IOException
    {
        try {

            ClassLoader cl = getURLClassloader(jsr);// ClassLoader.getSystemClassLoader();

            Class c = cl.loadClass(jsr.classNameForService);
            Object serviceInstance = c.newInstance();

            _serviceManager._servicesHT.put (jsr.UUID, serviceInstance);

            if (serviceInstance instanceof Service) {
                Service svc = (Service) serviceInstance;
                svc.init (_serviceManager);
            }

            log ("Calling requestCompleted()");
            requestCompleted (jsr, null, true);
        }
        catch (Exception ex) {
            ex.printStackTrace();
            requestCompleted (jsr, null, false);
        }
    } //processActivateRequest()


    private ClassLoader getURLClassloader(JavaServiceRequest jsr) throws IOException, ClassNotFoundException
    {
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

        return cl;
    }

    /**
     *
     */
    private void processTerminateRequest (JavaServiceRequest jsr) throws IOException
    {
        log ("Termination Request received for service " + jsr.UUID);

        Object serviceInstance = _serviceManager._servicesHT.remove (jsr.UUID);

        if (serviceInstance == null) {
            log ("*** (RequestTerminate) WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
            requestCompleted (jsr, null, false);
            return;
        }

        Method m = ServiceManager.findDeactivateMethod (serviceInstance.getClass());
        if (m != null) {
            log ("deactivate method in class " + serviceInstance.getClass().toString() + " found... invoking.");
            try {
                m.invoke (serviceInstance, new Object[0]);
            }
            catch (Exception ex) {
                log("problem invoking " + serviceInstance.getClass().toString() + ".deactivate():");
                ex.printStackTrace();
                requestCompleted (jsr, null, false);
                return;
            }
        }
        else {
            log ("'deactivate' method not found in class " + serviceInstance.getClass().toString());
        }

        requestCompleted (jsr, null, true);
    } //processTerminateRequest()


    private void processRestoreStateRequest(JavaServiceRequest jsr) throws IOException {
        log ("State Restore Request received for service " + jsr.UUID);
        long timestamp;

        try{

            ByteArrayInputStream bais = new ByteArrayInputStream(jsr.stateBuffer);
            ClassLoader cl = getURLClassloader(jsr);// ClassLoader.getSystemClassLoader();
            VM_MobileObjectInputStream ois = new VM_MobileObjectInputStream(bais, cl);

            if (BENCHMARK) {
                timestamp = System.currentTimeMillis();
            }

            // Reading the service instance and thread execution state
            Object serviceInstance = ois.readObject();

            // Registering the service locally
            _serviceManager._servicesHT.put(jsr.UUID, serviceInstance);

            if (BENCHMARK) {
                timestamp = System.currentTimeMillis() - timestamp;
                _statsObj.updateStats(ServiceManagerStatistics.RESTORE, timestamp);
            }


            // Start the thread
            ((Thread)serviceInstance).start();

            log ("Calling requestCompleted()");
            requestCompleted(jsr, null, true);
        }
        catch(ClassNotFoundException ex)
        {
            requestCompleted(jsr, null, false);
        }

    }

//    private Object[] getServiceInvocationParameters(VM_CompiledMethod cm, VM_MobileFrame frames)
//    {
//      VM_Method method = cm.getMethod();
//      VM_TypeReference[] params =  method.getParameterTypes();
//      Object[] objArgs = new Object[params.length];
//
//      for(int i=0; i<params.length; i++)
//      {
//          objArgs[i]=null; // TEMPORARILY SET TO NULL. IT SHOULD BE OK
//      }
//
//      return objArgs;
//    }
//
//    private Object invokeSpecializedMethod(VM_CompiledMethod cm, Object thisArg, Object[] otherArgs)
//    {
//      VM_Method method = cm.getMethod();
//      VM_CompiledMethod oldCM = method.getCurrentCompiledMethod();
//
//      method.replaceCompiledMethod(cm); // DANGEROUS... CHANGE IT AS SOON AS POSSIBLE
//
//      Object ret = VM_Reflection.invoke(method, thisArg, otherArgs);
//
//      method.replaceCompiledMethod(oldCM);
//
//      return ret;
//    }
//

    private void processCaptureStateRequest(JavaServiceRequest jsr) throws IOException {

        log ("State Capture Request received for service " + jsr.UUID);
        long timestamp;

        try{
            ByteArrayOutputStream baos = new ByteArrayOutputStream(MAX_STATE_BUF);
            ObjectOutputStream oos = new ObjectOutputStream(baos);

            Object serviceInstance = _serviceManager._servicesHT.get (jsr.UUID);

            if (serviceInstance == null) {
                log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
                requestCompleted (jsr, null, false);
                return;
            }

            if (BENCHMARK) {
                timestamp = System.currentTimeMillis();
            }

            // Write the service instance --> This serializes also the thread state associated with it
            oos.writeObject(serviceInstance);
            oos.flush();
            oos.close();

            if(serviceInstance instanceof MobileService)
                ((MobileService) serviceInstance).onServiceMigrated();

            if (BENCHMARK) {
                timestamp = System.currentTimeMillis() - timestamp;
                    _statsObj.updateStats(ServiceManagerStatistics.CAPTURE, timestamp);
            }

            requestCompleted(jsr, baos.toByteArray(), true);

        }
        catch(IOException e)
        {
            e.printStackTrace();
            requestCompleted(jsr, null, false);
        }

    }

    /**
     *
     */
    private void processInvokeRequest (JavaServiceRequest jsr) throws IOException
    {
        log ("Invocation Request received for service " + jsr.UUID + " methodName = " + jsr.methodName + " asynchronous = "+ jsr.asynchronous);
        long timestamp;

        Object serviceInstance = _serviceManager._servicesHT.get (jsr.UUID);
        if (serviceInstance == null) {
            log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
            requestCompleted (jsr, null, false);
            return;
        }

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis();
        }

        ClassLoader cl = serviceInstance.getClass().getClassLoader();
        InvokeRequestParser parser = new InvokeRequestParser (jsr.sis, cl);

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis() - timestamp;
            if (!_statsObj.getFirstInvocation()) {
                _statsObj.updateStats(ServiceManagerStatistics.PARSING, timestamp);
            }
        }

        String methodName = parser.isBinaryRequest() ? jsr.methodName : parser.getMethodName();
        Object[] paramValues = parser.getParameterValues();

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis();
        }

        Method methodToInvoke = ServiceManager.findMatchingMethod (serviceInstance.getClass(),
                                                    methodName,
                                                    parser.getParameterTypes());

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis() - timestamp;
            if (!_statsObj.getFirstInvocation()) {
                _statsObj.updateStats(ServiceManagerStatistics.FIND_METHOD, timestamp);
            }
        }

        if (methodToInvoke != null) {
            log ("found a matching method! Now Invoking...");
            try {
                if (BENCHMARK) {
                    timestamp = System.currentTimeMillis();
                }

                if (jsr.asynchronous) {
                    requestCompleted (jsr, null, true);
                }

                Object response = methodToInvoke.invoke (serviceInstance, paramValues);

                if (BENCHMARK) {
                    timestamp = System.currentTimeMillis() - timestamp;
                    if (!_statsObj.getFirstInvocation()) {
                        _statsObj.updateStats(ServiceManagerStatistics.INVOKE, timestamp);
                    }
                    timestamp = System.currentTimeMillis();
                }

                if (jsr.asynchronous) {
                    return;
                }

                InvokeResponseEncoder encoder = new InvokeResponseEncoder();
                byte[] encodedResponse = encoder.encode (methodName, response, parser.isBinaryRequest());

                if (BENCHMARK) {
                    timestamp = System.currentTimeMillis() - timestamp;
                    if (!_statsObj.getFirstInvocation()) {
                        _statsObj.updateStats(ServiceManagerStatistics.ENCODE, timestamp);
                    }
                }

                try {
                    requestCompleted (jsr, encodedResponse, true);
                }
                catch (Exception ex) {
                    log ("Problems writing out the response: ");
                    requestCompleted (jsr, null, false);
                    ex.printStackTrace();
                }

                if (BENCHMARK) {
                    if (!_statsObj.getFirstInvocation()) {
                        System.out.println("RVMServiceManager:: avg. time to parse the request:: "
                                           + _statsObj.getAverage(ServiceManagerStatistics.PARSING) + " (stdev: " + _statsObj.getStDev(ServiceManagerStatistics.PARSING) + ")");

                        System.out.println("RVMServiceManager:: AgServerUtils:: avg. time to deserialize the parameters:: "
                                           + AgServeUtils._deserializationStats.getAverage() + " (stdev: " + AgServeUtils._deserializationStats.getStDev() + ")");

                        System.out.println("RVMServiceManager:: InvokeReqParser:: avg. time to read/parse DIME:: "
                                           + InvokeRequestParser._dimeParsingStats.getAverage() + " (stdev: " + InvokeRequestParser._dimeParsingStats.getStDev() + ")");

                        System.out.println("RVMServiceManager:: avg. time to encode the reply:: "
                                           + _statsObj.getAverage(ServiceManagerStatistics.ENCODE) + " (stdev: " + _statsObj.getStDev(ServiceManagerStatistics.ENCODE) + ")");

                        System.out.println("RVMServiceManager:: avg. time on invocation:: "
                                           + _statsObj.getAverage(ServiceManagerStatistics.INVOKE) + " (stdev: " + _statsObj.getStDev(ServiceManagerStatistics.INVOKE) + ")");

                        System.out.println("RVMServiceManager:: avg. time to find the method:: "
                                           + _statsObj.getAverage(ServiceManagerStatistics.FIND_METHOD) + " (stdev: " + _statsObj.getStDev(ServiceManagerStatistics.FIND_METHOD) + ")");
                    }
                    else {
                        _statsObj.setFirstInvocation(false);
                    }
                }

                return;
            }
            catch (Exception ex) {
                log ("Problem invoking the " + serviceInstance.getClass() + "." + methodName + " method:");
                requestCompleted (jsr, null, false);
                ex.printStackTrace();
            }
        }
        else {
            log ("did not find a matching method (" + methodName + ") in " + serviceInstance.getClass().toString());
            requestCompleted (jsr, null, false);
        }
    } //processInvokeRequest()

    /**
     *
     */
    private void processConnectRequest (JavaServiceRequest jsr) throws IOException
    {
        log ("ConnectRequest received for service " + jsr.UUID);

        Object serviceInstance = _serviceManager._servicesHT.get (jsr.UUID);


        if (serviceInstance == null) {
            log ("*** WARNING:: the service instance for UUID " + jsr.UUID + " is null !!\n" );
            requestCompleted (jsr, null, false);
            return;
        }

        if (!(serviceInstance instanceof DirectConnectionService)) {
            log ("*** ERROR: the service " + jsr.classNameForService +
                 " does not implement the Service Interface. This is required to process the 'Connect' request");
            requestCompleted (jsr, null, false);
        }

        DirectConnectionService directConnService = (DirectConnectionService) serviceInstance;
        requestCompleted (jsr, null, true);

        try {
            directConnService.handleConnection (jsr.sis, jsr.sos);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }

        log ("ConnectRequest:: service [" + jsr.UUID + "] returned from handleConnection().");
    } //processConnectRequest()

    private static void log (String msg)
    {
        if (DEBUG) {
            System.out.println("[RVMServiceRequestHandler-"+Thread.currentThread().getName()+"] " + msg);
        }
    }

}
