package mil.darpa.coabs.gmas.util;

/**
 * CustomObjectInputStream extends (@link java.io.ObjectInputStream) and
 * overrides the resolveClass method in order to avoid ClassNotFound exceptions
 * when, within a thread, some of the classes are loaded by the system class
 * loader and then some are loaded by a different class loader, for instance a
 * URL ClassLoader.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version 1.0
 */
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectStreamClass;

public class CustomObjectInputStream extends java.io.ObjectInputStream {

    /**
     * If no classloader is provided, use the context classloader.
     * 
     * @param is InputStream currently being handled
     * @throws IOException
     */
    public CustomObjectInputStream (InputStream is) 
            throws IOException
    {
        this (is, Thread.currentThread().getContextClassLoader());
    }

    /**
     * Utilize the provided classloader to resolve any classes found in the 
     * stream.
     * 
     * @param is InputStream currently being handled
     * @param cl ClassLoader to utilize in resolving any classes in the stream
     * @throws IOException
     */
    public CustomObjectInputStream (InputStream is, ClassLoader cl)
            throws IOException
    {
        super (is);
        classloader = cl;
    }

    /**
     * Resolve the class in the object stream by using the classloader passed
     * in via the constructor.
     * 
     * @param objectStreamClass class in the current object stream that needs to
     *                          be resolved
     * @return class object loaded that matches the object in the stream
     */
    protected Class resolveClass (ObjectStreamClass objStrmClass)
            throws IOException, ClassNotFoundException
    {
        return classloader.loadClass (objStrmClass.getName());
    }
    
    protected ClassLoader classloader;
}