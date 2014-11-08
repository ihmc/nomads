/*
 * MocketSelectorProvider.java
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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
 * Implementation of SelectorProvider, 
 * which wraps the JDK's default SelectorProvider obtained by
 * sun.nio.ch.DefaultSelectorProvider.create().
 * This wrapping is necessary to provide special implementations of
 * ServerMocketChannel and MocketChannel (?? and Selector).
 *
 * @author ebenvegnu
 */

package us.ihmc.mockets;

import java.io.IOException;
import java.nio.channels.DatagramChannel;
import java.nio.channels.Pipe;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.AbstractSelector;
import java.net.ProtocolFamily;
import java.lang.reflect.Method;
import java.nio.channels.spi.SelectorProvider;

public class MocketSelectorProvider extends SelectorProvider
{
    /**
     * Initializes a new instance of this class.  </p>
     *
     */
    protected MocketSelectorProvider()
    {
        //PROVIDER = getProvider();
    }

/*    private MocketSelectorProvider getProvider()
    {
        return new
        SelectorProvider sp = null;
        try {
            Class classSP = Class.forName( "sun.nio.ch.DefaultSelectorProvider" );
            Method createMeth = classSP.getMethod("create", new Class[] {});
            sp = (SelectorProvider) createMeth.invoke( null, new Object[] {} );
        }
        catch( Exception e ) {
            throw new RuntimeException("Unable to create default SelectorProvider.", e);
        }
        return sp;
 
    }
 */
 
/** THIS IS NOT WHAT HAPPENS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     * Returns the system-wide default selector provider for this invocation of
     * the Java virtual machine.
     *
     * <p> The first invocation of this method locates the default provider
     * object as follows: </p>
     *
     * <ol>
     *
     *   <li><p> If the system property
     *   <tt>java.nio.channels.spi.SelectorProvider</tt> is defined then it is
     *   taken to be the fully-qualified name of a concrete provider class.
     *   The class is loaded and instantiated; if this process fails then an
     *   unspecified error is thrown.  </p></li>
     *
     *   <li><p> If a provider class has been installed in a jar file that is
     *   visible to the system class loader, and that jar file contains a
     *   provider-configuration file named
     *   <tt>java.nio.channels.spi.SelectorProvider</tt> in the resource
     *   directory <tt>META-INF/services</tt>, then the first class name
     *   specified in that file is taken.  The class is loaded and
     *   instantiated; if this process fails then an unspecified error is
     *   thrown.  </p></li>
     *
     *   <li><p> Finally, if no provider has been specified by any of the above
     *   means then the system-default provider class is instantiated and the
     *   result is returned.  </p></li>
     *
     * </ol>
     *
     * <p> Subsequent invocations of this method return the provider that was
     * returned by the first invocation.  </p>
     *
     * @return  The system-wide default selector provider
     */
    public static MocketSelectorProvider getProvider()
    {
        if (PROVIDER == null) {
            PROVIDER = new MocketSelectorProvider();
        }
        return PROVIDER;
    }

    /**
     * Opens a mocket channel. </p>
     *
     * @return  The new mocket channel
     */
    public MocketChannel openMocketChannel()
            throws IOException
    {
        return new MocketChannel (this);
    }

    public MocketChannel openMocketChannel (Mocket m)
            throws IOException
    {
        return new MocketChannel (this, m);
    }

    /**
     * Opens a server mocket channel. </p>
     *
     * @return  The new server mocket channel
     */
    public ServerMocketChannel openServerMocketChannel()
            throws IOException
    {
        return new ServerMocketChannel (this);
    }

    /*
     *
     */
    public DatagramChannel openDatagramChannel()
            throws IOException
    {
        return PROVIDER.openDatagramChannel();
    }

// WORKS ONLY IN JDK 7
    public DatagramChannel openDatagramChannel(ProtocolFamily family) {
        return PROVIDER.openDatagramChannel(family);
    }

    /*
     *
     */
    public Pipe openPipe() throws IOException
    {
        return PROVIDER.openPipe();
    }

    /*
     *
     */
    public AbstractSelector openSelector()
            throws IOException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    /*
     *
     */
    public ServerSocketChannel openServerSocketChannel()
            throws IOException
    {
        return PROVIDER.openServerSocketChannel();
    }

    /*
     *
     */
    public SocketChannel openSocketChannel()
            throws IOException
    {
        return PROVIDER.openSocketChannel();
    }

    private static MocketSelectorProvider PROVIDER = null;


}
