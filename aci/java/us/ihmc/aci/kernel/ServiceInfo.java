package us.ihmc.aci.kernel;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Hashtable;
import us.ihmc.aci.kernel.JavaServiceManager.JarFileFilter;

/**
 * This class contains information common to all instances of a certain service.
 * These data are obtained when the service is activated for the first time
 * and then are used for several purposes, e.g.:
 * - speeding up other service requests
 * - collecting statistics on a certain service type
 * - grouping all instances of a certain service
 *
 * @author rquitadamo
 */
class ServiceInfo {

    /**
     * The only constructor to a ServiceInfo object
     *
     * @param className The name of the service class as passed by the activation request
     * @param classPath The classpath used to build the URL classloader
     * @throws MalformedURLException
     * @throws ClassNotFoundException
     */
    ServiceInfo(String className, String classPath) throws MalformedURLException, ClassNotFoundException {

        // 1. Build the list of all URLs to pass to the URLClassloader
        File libDir = new File (classPath + "/lib");
        File[] libFileList = libDir.listFiles (new JarFileFilter());

        File externalsDir = new File (classPath + "/externals");
        File[] externalsFileList = externalsDir.listFiles (new JarFileFilter());

        int size = 1 + ((libFileList != null) ? libFileList.length : 0);
        size += ((externalsFileList != null) ? externalsFileList.length : 0);

        URL[] urlList = new URL[size];

        int i = 0;
        urlList[i++] = (new File (classPath)).toURI().toURL();

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

        // 2. Create the URL Class Loader with the prepared urls.
        classLoader = new URLClassLoader (urlList);

        // 3. Load the class in the VM
        serviceClassName = className;
        serviceClass = classLoader.loadClass(className);
        instances = new Hashtable();

        if (DEBUG) {
            URL[] urls = classLoader.getURLs();
            System.out.println("[ServiceInfo]:");
            for (int j = 0; j < urls.length; j++) {
                System.out.println ("Classpath URL[" + j + "] " + urls[j].toString());
            }
        }
    }

    /**
     * This method builds a new instance of this service, creates a new ServiceInstanceInfo object and
     * stores it in the instances table of this ServiceInfo object.
     *
     * @return The ServiceInstanceInfo object created
     *
     * @throws InstantiationException
     * @throws IllegalAccessException
     */
    ServiceInstanceInfo newServiceInstance(String serviceInstanceUUID) throws InstantiationException, IllegalAccessException
    {
        // 1. Try to instantiate the service class and create a new object
        Object obj = serviceClass.newInstance();

        // 2. Wrap the instance around a ServiceInstanceInfo object
        ServiceInstanceInfo sii = new ServiceInstanceInfo(serviceInstanceUUID, obj, this);

        // 3. Add the service instance object to the local hashtable "instances"
        instances.put(serviceInstanceUUID, sii);

        // 4. return the instance to the caller
        return sii;
    }

    ServiceInstanceInfo restoreServiceInstance(JavaServiceRequest jsr, RestoreWorkerThreadState state) throws InstantiationException, IllegalAccessException, IOException, ClassNotFoundException
    {
        // 1. Read the service object from the stream, using the classloader for this ServiceInfo object
        Object obj = state.readServiceInstance(classLoader);

        // 2. Wrap the instance around a ServiceInstanceInfo object
        ServiceInstanceInfo sii = new ServiceInstanceInfo(jsr.UUID, obj, this);

        // 3. Add the service instance object to the local hashtable "instances"
        instances.put(jsr.UUID, sii);

        // 4. return the instance to the caller
        return sii;
    }

    void removeServiceInstance(ServiceInstanceInfo sii) throws Exception
    {
        // 1. Find the deactivate method in the service class and invoke it
        Method m = ServiceManager.findDeactivateMethod (serviceClass);
        if (m != null) {
            log ("Deactivate method in class " + serviceClass.toString() + " found... invoking.");
            m.invoke (sii.instance, new Object[0]);
        }
        else {
            log ("'deactivate' method not found in class " + serviceClass.toString());
        }

        // 2. Remove the instance from the instances table
        instances.remove(sii);
    }

    /**
     * Logging function for the ServiceInfo class
     *
     * @param msg
     */
    private static void log(String msg) {
        if (DEBUG) {
            System.out.println("[ServiceInfo] " + msg);
        }
    }

    // //////////////////////////////////////////////////////////////////////////
    // INSTANCE FIELDS                                                         //
    // //////////////////////////////////////////////////////////////////////////
    /**
     * The fully-qualified name of the service class
     */
    String serviceClassName;

    /**
     * The class loader created from the classpath URLs
     */
    URLClassLoader classLoader;

    /**
     * The VM class object for the service (used to enable Reflection)
     */
    Class serviceClass;

    /**
     * The map of all instances for this service. It is accessed using the UUID
     * as key and it returns a ServiceInstanceInfo object (if one was found)
     */
    Hashtable instances;

    // //////////////////////////////////////////////////////////////////////////
    // STATIC FIELDS                                                           //
    // //////////////////////////////////////////////////////////////////////////
    private static final boolean DEBUG = false;
}
