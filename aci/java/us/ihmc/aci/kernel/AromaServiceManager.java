package us.ihmc.aci.kernel;

import java.lang.reflect.Method;
import java.util.Vector;

import us.ihmc.aci.agserve.InvokeRequestParser;
import us.ihmc.aci.agserve.InvokeResponseEncoder;
import us.ihmc.aci.grpMgrOld.GroupManagerProxy;
import us.ihmc.aci.service.Service;
import us.ihmc.algos.Statistics;

/**
 *
 */
public class AromaServiceManager extends ServiceManager
{
    public GroupManagerProxy getGroupManagerProxy()
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public String getServiceInstanceUUID (Object serviceInstance)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public void registerServiceAttribute (Service service, String attrKey, String attrVal)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public void deregisterServiceAttribute (Service service, String attrKey, String attrVal)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public boolean hasServiceAttribute (Service service, String attrKey, String attrVal)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public Vector getNetworkTopology()
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public String startPeerSearch (Service service, byte[] param, long TTL)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public String startPersistentPeerSearch (Service service, byte[] param)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public void stopPersistentPeerSearch (Service service, String searchUUID)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout, boolean force)
    {
      throw new UnsupportedOperationException ("not implemented");
    }

    public Vector findServiceInstances (String serviceName, String attrKey, String attrValue, long timeout, int maxInstanceNum)
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    /**
     *
     */
    public LocalCoordinator getLocalCoordinatorRef()
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    /**
     *
     */
    public void initLocalCoordinator (String localCoordClassName)
        throws Exception
    {
        throw new UnsupportedOperationException ("not implemented");
    }

    public static void main (String[] args)
    {
        // Arguments: args[0] contains the class name of the service to be instantiated
        try {
            boolean firstInvocation = true;
            long timestamp;
            Statistics parsingStats, invokeStats, encodeStats, findMethodStats;

            if (BENCHMARK) {
                parsingStats = new Statistics();
                invokeStats = new Statistics();
                encodeStats = new Statistics();
                findMethodStats = new Statistics();
            }

            Class serviceClass = Class.forName (args[0]);
            Object service = serviceClass.newInstance();
            ServiceRequest sr = new ServiceRequest();

            while (true) {
                getNextRequest (sr);

                if (sr.requestType == ServiceRequest.REQUEST_TYPE_TERMINATE) {
                    log ("Deactivation Request received for service " + args[0]);

                    Method m = findDeactivateMethod(serviceClass);
                    if (m != null) {
                        log ("deactivate method in class " + serviceClass + " found... invoking.");
                        try {
                            m.invoke(service, new Object[0]);
                        }
                        catch (Exception ex) {
                            log ("problem invoking " + serviceClass.toString() + ".deactivate():");
                            ex.printStackTrace();
                            requestCompleted (sr, false);
                            break;
                        }
                    }
                    else {
                        log ("'deactivate' method not found in class " + serviceClass.toString());
                    }

                    requestCompleted (sr, true);
                    break;
                }
                else if (sr.requestType == ServiceRequest.REQUEST_TYPE_INVOKE) {
                    log ("Invocation Request received for service " + args[0] + ", method " + sr.methodName);

                    if (BENCHMARK) {
                        timestamp = System.currentTimeMillis();
                    }

                    InvokeRequestParser parser = new InvokeRequestParser(sr.sis);

                    if (BENCHMARK) {
                        timestamp = System.currentTimeMillis() - timestamp;
                        if (!firstInvocation) {
                            parsingStats.update (timestamp);
                        }
                    }

                    String methodName = parser.isBinaryRequest() ? sr.methodName : parser.getMethodName();
                    Object[] paramValues = parser.getParameterValues();

                    if (BENCHMARK) {
                        timestamp = System.currentTimeMillis();
                    }

                    Method methodToInvoke = findMatchingMethod (serviceClass, methodName, parser.getParameterTypes());

                    if (BENCHMARK) {
                        timestamp = System.currentTimeMillis() - timestamp;
                        if (!firstInvocation) {
                            findMethodStats.update (timestamp);
                        }
                    }

                    if (methodToInvoke != null) {
                        try {
                            if (BENCHMARK) {
                                timestamp = System.currentTimeMillis();
                            }

                            Object response = methodToInvoke.invoke (service, paramValues);
                            log ("invocation succeeded. Will encode the response.");

                            if (BENCHMARK) {
                                timestamp = System.currentTimeMillis() - timestamp;
                                if (!firstInvocation) {
                                    invokeStats.update (timestamp);
                                }

                                timestamp = System.currentTimeMillis();
                            }

                            InvokeResponseEncoder encoder = new InvokeResponseEncoder();
                            byte[] encodedResponse = encoder.encode (methodName, response, parser.isBinaryRequest());

                            if (BENCHMARK) {
                                timestamp = System.currentTimeMillis() - timestamp;
                                if (!firstInvocation) {
                                    encodeStats.update (timestamp);
                                }
                            }

                            try {
                                log ("Writing the response out...");
                                sr.sos.write (encodedResponse);
                                sr.sos.flush();
                                requestCompleted (sr, true);
                            }
                            catch (Exception ex) {
                                log ("Problems writing out the response: ");
                                ex.printStackTrace();
                                requestCompleted (sr, false);
                            }
                        }
                        catch (Exception ex) {
                            log ("Problem invoking the " + serviceClass + "." + methodName + " method:");
                            requestCompleted (sr, false);
                            ex.printStackTrace();
                        }
                    }
                    else {
                        log ("did not find a matching method (" + methodName + ") in " + args[0]);
                        requestCompleted (sr, false);
                    }

                    if (BENCHMARK) {
                        if (!firstInvocation) {
                            System.out.println("AromaServiceManager:: avg. time to parse the request:: "
                                               + parsingStats.getAverage() + " (stdev: " + parsingStats.getStDev() + ")");

                            System.out.println("AromaServiceManager:: avg. time to encode the reply:: "
                                               + encodeStats.getAverage() + " (stdev: " + encodeStats.getStDev() + ")");

                            System.out.println("AromaServiceManager:: avg. time on invocation:: "
                                               + invokeStats.getAverage() + " (stdev: " + invokeStats.getStDev() + ")");

                            System.out.println("AromaServiceManager:: avg. time to find the method:: "
                                               + findMethodStats.getAverage() + " (stdev: " + findMethodStats.getStDev() + ")");
                        }
                        else {
                            firstInvocation = false;
                        }
                    }
                }
                else {
                    log ("Unknown request type " + sr.requestType + " received for service " + args[0]);
                }
            } //while (true)
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     *
     */
    private static void log (String msg)
    {
        if (DEBUG) {
            System.out.println("[AromaServiceManager] " + msg);
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    private static native void getNextRequest (ServiceRequest sr);
    private static native void requestCompleted (ServiceRequest sr, boolean success);

    // /////////////////////////////////////////////////////////////////////////
    private final static boolean DEBUG = false;
    protected static final boolean BENCHMARK = false;

}
