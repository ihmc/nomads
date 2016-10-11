/*
 * IDGenerator.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.util;

import java.util.Random;

public class IDGenerator
{
    public static String generateID()
    {
        return String.valueOf (System.currentTimeMillis()) + "-" +
               String.valueOf (Math.abs(_rand.nextInt())) + "-" +
               String.valueOf (_counter++);
    }
    
    /**
     * _rand is a random number generator that is used in constructing
     * new ID's for agents when they register with the grid via the
     * bridge.
     */
    private static Random               _rand = new Random();
    
    /**
     * _counter is used to ensure that the ID's are unique for agents
     * trying to register.  By adding the counter to the ID, this guards
     * against the random number generator spitting out the same number 
     * more than once in the sequence.
     */
    private static int                  _counter = 0;
}