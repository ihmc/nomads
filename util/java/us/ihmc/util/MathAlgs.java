/*
 * MathAlgs.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

/**
 * MathAlgs defines some common math algorithms
 */
public class MathAlgs
{
    /**
     * Computes the Greatest Common Divisor (GCD) in a set of numbers
     * 
     * @param values    array of integer values over which the GCD should be computed
     * 
     * @return the GCD value
     */
    public static int gcd (int[] values)
    {
        // Start with the minimum of the values
        int startValue = min (values);

        for (int candidateValue = startValue; candidateValue > 1; candidateValue--) {
            boolean found = true;
            for (int i = 0; i < values.length; i++) {
                if ((values[i] % candidateValue) != 0) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return candidateValue;
            }
        }
        return 1;
    }
    
    /**
     * Computes the minimum of an array of values. The array must have at least one element.
     * 
     * @param values    array of integer values
     * 
     * @return the minimum value
     */
    public static int min (int[] values)
    {
        if (values.length < 1) {
            throw new ArithmeticException ("array must have at least 1 element");
        }
        int minValue = values[0];
        for (int i = 1; i < values.length; i++) {
            if (minValue > values[i]) {
                minValue = values[i];
            }
        }
        return minValue;
    }
}
