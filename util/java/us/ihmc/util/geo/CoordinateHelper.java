/*
 * CoordinateHelper.java
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

package us.ihmc.util.geo;

/**
 *
 * @author Giacomo Benincasa    gbenincasa@ihmc.us
 */
public class CoordinateHelper
{
    /**
     * Convert Degree Minute Second angles to decimal format.
     * The degree-minute-second value is in pint-less format.
     * (For instance 223000 (22 30'00") to decimal 22.5)
     */
    public static double dmsToGeodic (int dms)
    {
        int seconds = dms % 100;
        int leftover = dms / 100;
        int minutes = leftover % 100;
        int degrees = leftover / 100;

        return dmsToGeodic (seconds, minutes, degrees);
    }

    /**
     * Convert Degree Minute Second angles to decimal format.
     * (For instance 22 30'00" to decimal 22.5)
     */
    public static double dmsToGeodic (int seconds, int minutes, int degrees)
    {
        return ((degrees) + (minutes * 1/60 ) + (seconds * 1/60 * 1/60));
    }

    /**
     * 
     * @param ulLat Upper Left Latitude (in decimal degrees)
     * @param uLLon Upper Left Longitude (in decimal degrees)
     * @param lrLat Lower Right Latitude (in decimal degrees)
     * @param lrLon Lower Right Longitude (in decimal degrees)
     * @return true, whether the coordinates include the whole globe (the Earth),
     *         false otherwise
     */
    public static boolean isWholeGlobe (float ulLat, float ulLon, float lrLat, float lrLon)
    {
        if (ulLat == MAX_LAT && ulLon == MIN_LON && lrLat == MIN_LAT && lrLon == MAX_LON) {
            return true;
        }
        return false;
    }

    public static final float MAX_LAT = 90f;
    public static final float MIN_LAT = -90f;
    public static final float MAX_LON =  180f;
    public static final float MIN_LON = -180f;
}
