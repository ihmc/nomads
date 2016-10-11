package us.ihmc.aci.disServiceProProxy;

import us.ihmc.aci.disServiceProxy.DisseminationServiceProxy;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxyCallbackHandler;
import us.ihmc.comm.CommHelper;

/**
 *
 * @author gbenincasa
 */
public class CallbackHandlerFactory extends us.ihmc.aci.disServiceProxy.CallbackHandlerFactory
{
    CallbackHandlerFactory()
    {
    }

    @Override
    public DisseminationServiceProxyCallbackHandler getHandler (DisseminationServiceProxy proxy, CommHelper commHelper)
    {
        return new DisServiceProProxyCallbackHandler ((DisServiceProProxy) proxy, commHelper);
    }
}
