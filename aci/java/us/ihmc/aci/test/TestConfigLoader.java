package us.ihmc.aci.test;

import us.ihmc.util.ConfigLoader;

/**
 * TestConfigLoader
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Aug 4, 2004 at 8:37:41 AM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class TestConfigLoader
{
    public static void main(String[] args)
    {
        ConfigLoader loader = ConfigLoader.initDefaultConfigLoader ((String) System.getProperties().get("NOMADS_HOME"),
                                                                    "/conf/aci.properties");
        String saux = loader.getProperty("arl.provider.POS.poolInterval");
        System.out.println("Result: " + saux);
    }

}
