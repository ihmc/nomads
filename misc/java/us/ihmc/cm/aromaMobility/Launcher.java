package us.ihmc.cm.aromaMobility;

import java.io.Serializable;
import java.util.*;

import us.ihmc.cm.*;
import us.ihmc.nomads.Agent;
import us.ihmc.nomads.StrongMobilityService;
import us.ihmc.util.URI;

public class Launcher extends Agent
{
    static Agent _launcher = null;
    
    public void start (String[] args)
    {
        String platprov = null;
        String mobprov  = null;
        Launcher._launcher = this;
        if (args.length > 0) {
            String rovMindClassName = args[0];

            //showProps (System.getProperties ());
            
            if (args.length > 1) {
                mobprov = args[1];
            }
            if (args.length > 2) {
                platprov = args[2];
            }
            /*
            //Properties sysProps = System.getProperties ();
            for (int i=1; i<args.length; i++) {
                if (args[i].startsWith ("-mp=")) {
                    sysProps.put ("us.ihmc.cm.mobilityProviderClass", args[i].substring (4));
                }
                else if (args[i].startsWith ("-pp=")) {
                    sysProps.put ("us.ihmc.cm.platformProviderClass", args[i].substring (4));
                }
            }
            try {
                System.setProperties (sysProps);
            }
            catch (Exception xcp) {
                xcp.printStackTrace();
            }
            */

            msg ("Roving Mind class name is "+rovMindClassName);
            try {
                Class rovMindClass = Class.forName (rovMindClassName);

                // Get RovingMind class from the arguments
                //RovingMind rovMind = new ExampleRovingMind ();
                //Properties p = System.getProperties ();
                //msg (p.toString());
                
                RovingMind rovMind = (RovingMind) rovMindClass.newInstance ();
                rovMind.initProviders (mobprov, platprov);
                rovMind.init ();
                rovMind.startMobilityProvider ();
                rovMind.run ();
            }
            catch (Exception xcp) {
                xcp.printStackTrace();
                msg ("Could not instantiate Roving Mind class: "+rovMindClassName);
            }
        }
    }

    
    public static void showProps (Properties props)
    {
        Enumeration e = props.keys ();
        while (e.hasMoreElements ()) {
            String key = (String) e.nextElement();
            System.out.println (key+"\t"+((String) props.get (key)));
        }
    }
    
    public static void msg (String msg)
    {
        System.out.println ("Launcher: "+msg);
    }
}
