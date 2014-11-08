/*
 * Position.java
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

package us.ihmc.util.geo;

import org.apache.commons.lang3.builder.EqualsBuilder;
import org.apache.commons.lang3.builder.HashCodeBuilder;

import java.math.BigDecimal;

/**
 * Position.java
 * <p/>
 * Class <code>Position</code> represents a geographic-spatial point with coordinates (latitude and longitude).
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public final class Position
{
    /**
     * Constructor for <code>Position</code>
     *
     * @param latitude  <code>BigDecimal</code> representation of the latitude value
     * @param longitude <code>BigDecimal</code> representation of the longitude value
     */
    public Position (BigDecimal latitude, BigDecimal longitude)
    {
        _latitude = latitude;
        _longitude = longitude;
    }

    /**
     * Constructor for <code>Position</code>
     *
     * @param latitude  <code>double</code> representation of the latitude value
     * @param longitude <code>double</code> representation of the longitude value
     */
    public Position (double latitude, double longitude)
    {
        _latitude = new BigDecimal(latitude);
        _longitude = new BigDecimal(longitude);
    }

    /**
     * Gets the latitude of this Position's object
     *
     * @return a BigDecimal reference representing latitude
     */
    public BigDecimal getLatitude ()
    {
        return _latitude;
    }

    /**
     * Gets the longitude of this Position's object
     *
     * @return a BigDecimal reference representing longitude
     */
    public BigDecimal getLongitude ()
    {
        return _longitude;
    }

    @Override
    public int hashCode()
    {
        return new HashCodeBuilder(17, 31). //two randomly chosen prime numbers
                append(_latitude).
                append(_longitude).
                toHashCode();
    }

    @Override
    public boolean equals (Object o)
    {
        if (!(o instanceof Position))
            return false;

        Position p = (Position) o;
        return new EqualsBuilder().
                append(_latitude, p._latitude).
                append(_longitude, p._longitude).
                isEquals();
    }

    /**
     * Overrides <code>Object</code>'s <code>toString()</code> method to provide a correct representation of this
     * <code>Position</code>.
     *
     * @return the <code>String</code> representation of this <code>Position</code>
     */
    @Override
    public String toString ()
    {
        StringBuilder sb = new StringBuilder();
        sb.append(_latitude.doubleValue());
        sb.append(separator);
        sb.append(_longitude.doubleValue());
        return sb.toString().trim();
    }

    private final BigDecimal _latitude;
    private final BigDecimal _longitude;

    public final static String separator = ",";
    public final static double SAMPLE_LATITUDE = 21.44688;
    public final static double SAMPLE_LONGITUDE = -157.760086;
}
