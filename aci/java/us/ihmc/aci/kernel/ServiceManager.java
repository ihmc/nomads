/**
 * ServiceManager
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.kernel;

import java.lang.reflect.Method;
import java.util.Vector;

import us.ihmc.aci.grpMgrOld.GroupManagerProxy;
import us.ihmc.aci.service.Service;

/**
 *
 */
public abstract class ServiceManager
{
    public abstract GroupManagerProxy getGroupManagerProxy();

    public abstract String getServiceInstanceUUID (Object serviceInstance);
    public abstract void registerServiceAttribute (Service service, String attrKey, String attrVal);
    public abstract void deregisterServiceAttribute (Service service, String attrKey, String attrVal);
    public abstract boolean hasServiceAttribute (Service service, String attrKey, String attrVal);
    public abstract String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout);
    public abstract String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout, boolean force);
    public abstract Vector findServiceInstances (String serviceName, String attrKey, String attrValue, long timeout, int maxInstanceNum);

    public abstract String startPeerSearch (Service service, byte[] param, long TTL);
    public abstract String startPersistentPeerSearch (Service service, byte[] param);
    public abstract void stopPersistentPeerSearch (Service service, String searchUUID);

    public abstract LocalCoordinator getLocalCoordinatorRef();
    public abstract void initLocalCoordinator (String localCoordClassName)
        throws Exception;

    /**
     *
     */
    public static Method findDeactivateMethod (Class serviceClass)
    {
        Method[] serviceMethods = serviceClass.getMethods();
        for (int i = 0; i < serviceMethods.length; i++) {
            Method m = serviceMethods[i];

            if (m.getName().equals("deactivate") && (m.getParameterTypes().length == 0)) {
                return m;
            }
        }

        return null;
    }

    /**
     *
     */
    public static Method findMatchingMethod (Class serviceClass, String methodName, Class[] methodParamTypes)
    {
        Method classMethods[] = serviceClass.getMethods();
        for (int i = 0; i < classMethods.length; i++) {
            Method m = classMethods[i];

            if (methodMatches(methodName, methodParamTypes, m)) {
                return m;
            }
        }

        return null;
    }

    /**
     *
     */
    public static boolean methodMatches (String name, Class[] paramTypes, Method method)
    {
        if (!method.getName().equals(name)) {
            return false;
        }

        Class[] methodParamTypes = method.getParameterTypes();

        if (methodParamTypes.length != paramTypes.length) {
            return false;
        }

        for (int i = 0; i < methodParamTypes.length; i++) {
            if (paramTypes[i] == null) {
                continue;
            }

            if (!methodParamTypes[i].isAssignableFrom(paramTypes[i])) {
                return false;
            }
        }

        return true;
    }

    // /////////////////////////////////////////////////////////////////////////
}
