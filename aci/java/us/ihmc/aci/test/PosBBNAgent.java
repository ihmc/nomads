package us.ihmc.aci.test;

import us.ihmc.nomads.Agent;

/**
 * PosBBNAgent
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Aug 6, 2004 at 12:24:18 PM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class PosBBNAgent extends Agent
{

    public void terminate()
    {
        //do nothing.
    }

    public void start(String[] args)
    {
        _posBBN = new PosBBN(args);
        _posBBN.run();
    }

    private PosBBN _posBBN;
}
