/**
 * MutableURLClassLoader.java $author Tom Apr 11, 2005 $version
 */
package mil.darpa.coabs.gmas.util;

import java.net.URL;
import java.net.URLClassLoader;

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;

/**
 * We subclass URLClassLoader so we can determine if a given URL is already in
 * the ClassLoaders URLClassPath, and add them to it if need be, as new codebase
 * URLs come in from new GMAS clients.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com>
 */
public class MutableURLClassLoader extends URLClassLoader {

    /**
     * 
     */
    public MutableURLClassLoader ()
    {
        super (new URL[] {});
    }

    /**
     * @param parent public MutableURLClassLoader (ClassLoader parent) { super
     *            (new URL[] {}, parent); }
     */

    /**
     * Construct a new URLClassLoader with an array of URLs and a pre- existing
     * parent class loader.
     */
    public MutableURLClassLoader (URL[] urls)
    {
        super (urls);

    }

    /**
     * Construct a new URLClassLoader with an array of URLs and a pre- existing
     * parent class loader.
     */
    public MutableURLClassLoader (URL[] urls, ClassLoader classLoader)
    {
        super (urls, classLoader);

    }

    /**
     * Add the URL provided to the URLClassPath searched by this class loader.
     */
    public void addURL (URL u)
    {
        System.out.println ("MutableURLClassLoader:\n\nINFO: adding url: " + u.toExternalForm ());
        super.addURL (u);
    }

    /**
     * Determine if this class loader has the URL u in its search path. If it
     * does, this returns true.
     * 
     * @param u check Classloader's URLClassPath to see if it contains this
     * @return boolean indicating whether or not URL u is contained in the
     *         ClassLoader's classpath
     */
    public boolean contains (URL u)
    {
        URL[] URLlist = super.getURLs ();
        for (URL i : URLlist) {
            if (i.equals (u)) {
                return true;
            }
        }
        return false;
    }
}
