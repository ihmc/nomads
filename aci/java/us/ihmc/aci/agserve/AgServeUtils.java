/**
 * AgServeUtils
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.agserve;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamClass;

import us.ihmc.algos.Statistics;
import us.ihmc.util.Dime;
import us.ihmc.util.DimeRecord;
import java.io.OutputStream;

/**
 *
 */
public class AgServeUtils
{
    public AgServeUtils()
    {
    }

    public AgServeUtils (ClassLoader classLoader)
    {
        _customClassLoader = classLoader;
    }

    /**
     *
     */
    public void setCustomClassLoader (ClassLoader classLoader)
    {
        _customClassLoader = classLoader;
    }

    /**
     *
     */
    public Dime encodeBinaryRequestParams (Object[] objects)
        throws Exception
    {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        ObjectOutputStream oos = new ObjectOutputStream (baos);

        int len = (objects == null) ? 0 : objects.length;

        oos.writeInt (len);
        if (objects != null) {
            for (int i = 0; i < objects.length; i++) {
                oos.writeObject (objects[i]);
            }
        }

        oos.flush();
        oos.close();

        byte[] buf = baos.toByteArray();

        Dime dimeMsg = new Dime();
        dimeMsg.addRecord ("binary_invoke_request", buf, "");

        return dimeMsg;
    }

    /**
     *
     */
    public Object[] decodeBinaryRequestParams (Dime dime)
        throws Exception
    {
        long timestamp;

        if (dime == null) {
            throw new NullPointerException ("dime");
        }
        if (dime.getRecordCount() == 0) {
            return new Object[0];
        }

        DimeRecord dimeRec = (DimeRecord) dime.getRecords().nextElement();

        if ( !"binary_invoke_request".equals(dimeRec.getType()) ) {
            throw new Exception ("DimeRecord type is " + dimeRec.getType() + ". It should be \"binary_invoke_request\"");
        }

        ByteArrayInputStream bais = new ByteArrayInputStream (dimeRec.getPayload());
        ObjectInputStream ois = getObjectInputStream (bais);//new ObjectInputStream (bais);

        int numObjects = ois.readInt();
        Object[] objects = new Object[numObjects];

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis();
        }

        for (int i = 0; i < numObjects; i++) {
            objects[i] = ois.readObject();
        }

        if (BENCHMARK && !_firstInvocation) {
            timestamp = System.currentTimeMillis() - timestamp;
            _deserializationStats.update (timestamp);
        }
        else {
            _firstInvocation = false;
        }

        return objects;
    }

    /**
     *
     */
    public Dime encodeObjectInDime (Object obj)
        throws Exception
    {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        ObjectOutputStream oos = new ObjectOutputStream (baos);
        oos.writeObject (obj);
        oos.flush();
        oos.close();

        Dime dimeMsg = new Dime();
        dimeMsg.addRecord("java.lang.Object", baos.toByteArray(), "");

        return dimeMsg;
    }

    public void encodeObjectInDime (Object obj, OutputStream os)
        throws Exception
    {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        ObjectOutputStream oos = new ObjectOutputStream (baos);
        oos.writeObject (obj);
        oos.flush();
        oos.close();

        Dime dimeMsg = new Dime();
        dimeMsg.addRecord("java.lang.Object", baos.toByteArray(), "");

        dimeMsg.getDime (os);
    }

    /**
     *
     */
    public Object decodeObjectFromDime (Dime dime)
        throws Exception
    {
        if (dime == null) {
            throw new NullPointerException ("dime");
        }
        if (dime.getRecordCount() == 0) {
            return null;
        }

        DimeRecord dimeRec = (DimeRecord) dime.getRecords().nextElement();
        if ( !"java.lang.Object".equals(dimeRec.getType()) ) {
            throw new Exception ("DimeRecord type is not \"java.lang.Object\"");
        }

        ByteArrayInputStream bais = new ByteArrayInputStream (dimeRec.getPayload());
        ObjectInputStream ois = getObjectInputStream (bais); //new ObjectInputStream (bais);
        Object obj = ois.readObject();

        bais.close();
        ois.close();

        return obj;
    }

    /**
     *
     */
    private ObjectInputStream getObjectInputStream (InputStream is)
        throws Exception
    {
        if (_customClassLoader != null) {
            return new CustomObjectInputStream (is, _customClassLoader);
        }
        else {
            return new ObjectInputStream (is);
        }
    }


    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    public class CustomObjectInputStream extends ObjectInputStream
    {
        public CustomObjectInputStream (InputStream is, ClassLoader classLoader)
            throws Exception
        {
            super (is);
            _classLoader = classLoader;
        }

        protected Class resolveClass (ObjectStreamClass desc)
            throws IOException, ClassNotFoundException
        {
          //            Class cl = _classLoader.loadClass (desc.getName());
          //  return cl;
              String name = desc.getName();
              try {
                  return Class.forName(name, true, _classLoader);
              }
              catch(ClassNotFoundException x)
              {
                  if (name.equals("void"))
                    return Void.TYPE;
                  else if (name.equals("boolean"))
                    return Boolean.TYPE;
                  else if (name.equals("byte"))
                    return Byte.TYPE;
                  else if (name.equals("char"))
                    return Character.TYPE;
                  else if (name.equals("short"))
                    return Short.TYPE;
                  else if (name.equals("int"))
                    return Integer.TYPE;
                  else if (name.equals("long"))
                    return Long.TYPE;
                  else if (name.equals("float"))
                    return Float.TYPE;
                  else if (name.equals("double"))
                    return Double.TYPE;
                  else
                    throw x;
              }
        }

        private ClassLoader _classLoader;
    }

    // /////////////////////////////////////////////////////////////////////////
    public static final boolean BENCHMARK = false;
    public static Statistics _deserializationStats;
    public boolean _firstInvocation = true;

    public ClassLoader _customClassLoader = null;

    // /////////////////////////////////////////////////////////////////////////
    // STATIC INITIALIZER //////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    static {
        if (BENCHMARK) {
            _deserializationStats = new Statistics();
        }
    }
}
