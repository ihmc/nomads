/*
 * CheckUtil.java
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
 * This class provides a number of precondition checking utilities.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public final class CheckUtil
{
    /**
     * Constructor for <code>CheckUtil</code>
     */
    private CheckUtil ()
    {
        throw new AssertionError();
    }

    /**
     * Ensures the truth of an expression involving one or more parameters to the
     * calling method.
     *
     * @param expression   a boolean expression
     * @param errorMessage the exception message to use if the check fails
     * @throws IllegalArgumentException if expression is false
     */
    public static void checkArgument (boolean expression, String errorMessage)
    {
        if (!expression) {
            throw new IllegalArgumentException(errorMessage);
        }
    }

    /**
     * Ensures that an object reference passed as a parameter to the calling
     * method is not null.
     *
     * @param reference    an object reference
     * @param errorMessage the exception message to use if the check fails
     * @return the non-null reference that was validated
     * @throws NullPointerException if the reference is null
     */
    public static <T> T checkNotNull (T reference, String errorMessage)
    {
        if (reference == null) {
            throw new NullPointerException(errorMessage);
        }
        return reference;
    }

    /**
     * Ensures that an object reference passed as a parameter to the calling
     * method is not null.
     * A standard error message will be printed in output if the checked reference is null.
     *
     * @param reference an object reference
     * @return the non-null reference that was validated
     * @throws NullPointerException if the reference is null
     */
    public static <T> T checkNotNull (T reference)
    {
        return checkNotNull(reference, "Checked reference is null");
    }
}
