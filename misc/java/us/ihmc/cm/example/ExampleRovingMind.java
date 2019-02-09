package us.ihmc.cm.example;

import us.ihmc.cm.*;

public class ExampleRovingMind extends RovingMind
{
    
    ExampleBotPart[] abps;
    
    public void init ()
    {
        abps = new ExampleBotPart[3];
        // Create the bot parts
        abps[0] = new ExampleBotPart ("tcp://localhost:3280");
        abps[1] = new ExampleBotPart ("tcp://localhost:3281");
        abps[2] = new ExampleBotPart ("tcp://localhost:3282");
        
        // Add them to this roving mind's set of bot parts
        for (int i=0; i<abps.length; i++) {
            addBotPart (abps[i]);
        }
        
        this.setTimeslice (1000);
        msg ("Botparts created and added");
    }
    
    public void run()
    {
        int sequence = 0;
        while (true) {
            // Just sleep for a while and printf something
            try {
                //msg ("Roving Mind awakening from sleep");
                double r = Math.random ();
                int i = (int) (r*abps.length);
                System.out.println ("\n\n\n==================================================================\n"+
                "ExRovMind ("+sequence+"): Value from bot part "+i+": "+this.getBotPartValue (abps[i].getURI(), "value").toString()+
                                    "\n==================================================================\n\n\n");
                sequence++;
            }
            catch (Exception xcp) {
                xcp.printStackTrace();
            }
        }
    }
    
    private static void msg (String m)
    {
        System.out.println ("ExRovMind: "+m);
    }
}
