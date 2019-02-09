package mil.darpa.coabs.gmas;

import java.lang.reflect.Method;

import mil.darpa.coabs.gmas.mobility.GmasMobilityException;

/**
 * Class that represents the local Grid-compliant mobile agent system. Currently
 * responsible for returning a platform-specific implementation of the mobility
 * or the messaging service. Allows us to maintain a common code repository in the
 * mil.darpa.coabs.gmas package and yet provide different implementations
 * for different systems.
 * 
 * @author Greg Hill
 * @author Niranjan Suri
 * @author Tom Cowin <tom.cowin@gmail.com> Addition of messaging as well as
 *         support for native agent adapters for both mobility and messaging.
 * @see BasicGridService
 */
public class GridMobileAgentSystem {

    /**
     * Get the instance of the primary GMAS Service, listening for incoming GMAML 
     * messages. This method passes in an object that is a handle to the existing
     * agent system environment. This is the method that needs to be invoked if 
     * there is any doubt that this service is pre-existing. Some agent systems
     * may choose to start the GmamlMessageHandlingService automatically, and may
     * not need to pass in a handle to the agent system evironment. In Aglets, 
     * this handle is required to start any agents. The implementor should define
     * a java.property of 'gmas.service' during startup that gives the name of 
     * the local implementation of the GmamlMessageHandlingService.
     * 
     * @param agentSystemHandle
     * @return local instance of GmamlMessageHandlingService
     * @see mil.darpa.coabs.gmas.GmamlMessageHandlingService
     */
    public static GmamlMessageHandlingService getGmamlMessageHandlingService (Object agentSystemHandle)
    {
        if (_gmasService == null) {
            String messageHandlingService = System.getProperty ("gmas.service");
            try {
                if (messageHandlingService == null) {
                    throw new Exception ("Fatal GMAS Error: no message handling service defined.");
                }
                else {
                    Method method = Class.forName (messageHandlingService).getMethod ("getGmamlMessageHandlingService",
                                                                                      new Class[] {Object.class});
                    if (method == null) {
                        throw new Exception ("Unable to get method getGmamlMessageHandlingService(Object) in class: "
                                + messageHandlingService);
                    }
                    _gmasService = (GmamlMessageHandlingService) method.invoke (null,
                                                                        new Object[] {agentSystemHandle});
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return _gmasService;
    }

    /**
     * Get the instance of the primary GMAS Service, listening for incoming
     * GMAML messages. This method only returns an existing service instance.
     * The programmer should invoke (@link
     * #getGmamlMessageHandlingService(Object)) if s/he needs to be sure that
     * service is actually running. The implementor should define a
     * java.property of 'gmas.service' during startup that gives the name of the
     * local implementation of the GmamlMessageHandlingService.
     * 
     * @return local instance of GmamlMessageHandlingService
     * @see mil.darpa.coabs.gmas.GmamlMessageHandlingService
     */
    public static GmamlMessageHandlingService getGmamlMessageHandlingService() 
            throws GmasMobilityException
    {
        if (_gmasService == null) {
            throw new GmasMobilityException ("Attempt to getGmamlMessageHandlingService when service does not exist");

        }
        return _gmasService;
    }
    
    /**
     * Get the locally defined instance of the mobility service. The implementor
     * should define a java.property of 'gmas.mobilityservice' during startup
     * that gives the name of the local implementation of the MobilityService.
     * 
     * @return local instance of the MobilityService
     * @see mil.darpa.coabs.gmas.MobilityService
     * @see mil.darpa.coabs.gmas.mobility.MobilityServiceImpl
     */
    public static MobilityService getGridMobilityService()
    {
        if (_gmasMobilityService == null) {
            String mobilityService = System.getProperty ("gmas.mobilityservice");
            try {
                if (mobilityService == null) {
                    throw new Exception ("Fatal GMAS Error: no mobility service defined.");
                }
                else {
                    _gmasMobilityService = (MobilityService) Class.forName (mobilityService).newInstance();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return _gmasMobilityService;
    }
    
    /**
     * Get the locally defined instance of the messaging service. The implementor
     * should define a java.property of 'gmas.messagingservice' during startup
     * that gives the name of the local implementation of the MobilityService.
     * 
     * @return local instance of Messaging Service
     * @see mil.darpa.coabs.gmas.MessagingService
     * @see mil.darpa.coabs.gmas.messaging.MessagingServiceImpl
     */
    public static MessagingService getGridMessagingService()
    {
        if (_gmasMessagingService == null) {
            String messagingService = System.getProperty ("gmas.messagingservice");
            try {
                if (messagingService == null) {
                    throw new Exception ("Fatal GMAS Error: no mobility service defined.");
                }
                else {
                    _gmasMessagingService = (MessagingService) Class.forName (messagingService).newInstance();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return _gmasMessagingService;
    }
    
    private static GmamlMessageHandlingService _gmasService;
    private static MessagingService _gmasMessagingService;
    private static MobilityService _gmasMobilityService;
}
