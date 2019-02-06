/**
 * Statistics.java
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
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 * 
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.algos;

/**
 * 
 */
public class Statistics
{
    /**
     *
     */
    public void update (double value)
    {
        _sumValues += value;
        _sumSqValues += Math.pow (value, 2);
        _totalNumValues++;
    }

    /**
     *
     */
    public void reset()
    {
        _sumValues = 0;
        _sumSqValues = 0;
        _totalNumValues = 0;
    }

    /**
     *
     */
    public int getNumValues()
    {
        return _totalNumValues;
    }

    /**
     *
     */
    public double getAverage()
    {
        return (_sumValues/_totalNumValues);
    }

    /**
     *
     */
    public double getStDev()
    {
        double avg = getAverage();
        double aux = (_totalNumValues * avg * avg)
                     - (2 * avg * _sumValues)
                     + _sumSqValues
                     ;
        aux = (double) aux / (_totalNumValues - 1);
        aux = Math.sqrt (aux);

        return aux;
    }

    // /////////////////////////////////////////////////////////////////////////
    private double _sumValues = 0;
    private double _sumSqValues = 0;
    private int _totalNumValues = 0;
} //class Statistics
