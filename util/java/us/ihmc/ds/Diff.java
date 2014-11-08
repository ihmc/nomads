/**
 * Diff.java
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

package us.ihmc.ds;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

public class Diff
{
    /**
     * Performs a diff of two vectors, optionally returning the common elements and the differing elements.
     * This method does not assume that the elements are sorted (or can be sorted) and hence is expensive.
     * 
     * @param a              first vector
     * @param b              second vector
     * @param aIntersectB    vector in which common elements will be returned; can be null if this
     *                       information is not desired
     * @param aMinusB        vector in which elements in the first vector that are not in the second
     *                       vector will be returned; can be null if this information is not desired
     * @param bMinusA        vector in which elements in the second vector that are not in the first
     *                       vector will be returned; can be null if this information is not desired
     * 
     * @return               true if the vectors are different, false if they are the same
     */
    public static boolean vectorDiff (Vector a, Vector b, Vector aIntersectB, Vector aMinusB, Vector bMinusA)
    {
        boolean diff = false;
        Enumeration e = a.elements();
        while (e.hasMoreElements()) {
            Object o = e.nextElement();
            if (b.contains (o)) {
                if (aIntersectB != null) {
                    aIntersectB.addElement(o);
                }
            }
            else {
                diff = true;
                if (aMinusB != null) {
                    aMinusB.addElement (o);
                }
            }
        }
        e = b.elements();
        while (e.hasMoreElements()) {
            Object o = e.nextElement();
            if (!a.contains (o)) {
                diff = true;
                if (bMinusA != null) {
                    bMinusA.addElement (o);
                }
            }
        }
        return diff;
    }
    
    /**
     * Performs a diff of two hashtables, optionally returning the common elements and the differing elements.
     *
     * @param a              first hashtable
     * @param b              second hashtable
     * @param aIntersectB    Hashtable in which common elements will be returned; can be null if this information
     *                       is not desired. NOTE: Only the keys in the hashtable are compared, not the values.
     *                       Therefore, the values in the aIntersectB hashtable are those that are in hashtable a.
     * @param aMinusB        Hashtable in which elements in the first hashtable that are not in the second hashtable
     *                       will be returned; can be null if this information is not desired
     * @param bMinusA        Hashtable in which elements in the second hashtable that are not in the first hashtable
     *                       will be returned; can be null if this information is not desired
     */
    public static boolean hashtableDiff (Hashtable a, Hashtable b, Hashtable aIntersectB, Hashtable aMinusB, Hashtable bMinusA)
    {
        boolean diff = false;
        Enumeration e = a.keys();
        while (e.hasMoreElements()) {
            Object o = e.nextElement();
            if (b.containsKey (o)) {
                if (aIntersectB != null) {
                    aIntersectB.put (o, a.get(o));
                }
            }
            else {
                diff = true;
                if (aMinusB != null) {
                    aMinusB.put (o, a.get(o));
                }
            }
        }
        e = b.keys();
        while (e.hasMoreElements()) {
            Object o = e.nextElement();
            if (!a.containsKey (o)) {
                diff = true;
                if (bMinusA != null) {
                    bMinusA.put (o, b.get(o));
                }
            }
        }
        return diff;
    }
}
