package mil.darpa.coabs.gmas.util;

/**
 * Base64Transcoders provides methods to convert from Base64 encoded ASCII 
 * strings to Byte Arrays or to Objects, and conversely, it provides methods
 * to convert Objects or Byte Arrays to Base64 encoded ASCII strings. This 
 * allows for manual serialization of objects and the ability to transport them
 * via text messaging.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com>
 */
import java.io.IOException;
import java.io.Serializable;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.lang.String;

public class Base64Transcoders {

    public Base64Transcoders()
    {
    }

    /**
     * Convert the Base64 encoded string to an Object.
     * 
     * @param b64String Base64 encoded ASCII string to be converted
     * @return Serializable object
     * @throws ClassNotFoundException
     * @throws IOException
     * @throws Base64FormatException
     */
    public Serializable convertB64StringToObject (String b64String)
            throws ClassNotFoundException, IOException, Base64FormatException
    {
        // Convert from byte array to serializable object
        ByteArrayInputStream bIn = new ByteArrayInputStream (convertB64StringToByteArray (b64String));
        ObjectInputStream In = new ObjectInputStream (bIn);
        Object o = In.readObject();
        return (Serializable) o;
    }

    /**
     * Convert the Base64 encoded string to an Object. Use the provided 
     * classLoader with the specified subclass of the ObjectInputStream so 
     * that its resolveClass method can be overridden. This will enable us to 
     * avoid ClassNotFound exceptions due to scope conflicts from using 
     * multiple class loaders during runtime.
     * 
     * @param b64String Base64 encoded ASCII string to be converted
     * @param loader ClassLoader to use in loading any classes being deserialized
     * @return Serializable object
     * @throws ClassNotFoundException
     * @throws IOException
     * @throws Base64FormatException
     */
    public Serializable convertB64StringToObject (String b64String,
                                                  ClassLoader loader)
            throws ClassNotFoundException, IOException, Base64FormatException
    {
        // Convert from byte array to serializable object
        ByteArrayInputStream bIn = new ByteArrayInputStream (convertB64StringToByteArray (b64String));
        CustomObjectInputStream In = new CustomObjectInputStream (bIn, loader);
        Object o = In.readObject();
        return (Serializable) o;
    }

    /**
     * Convert the Base64 encoded string to a byte array. This is the first half
     * of the process of converting the Base64String to an Object.
     * 
     * @param b64String Base64 encoded ASCII string to be converted
     * @return byte array
     * @throws ClassNotFoundException
     * @throws IOException
     * @throws Base64FormatException
     */
    public static byte[] convertB64StringToByteArray (String b64String)
            throws IOException, Base64FormatException
    {
        // Convert from Base64 string to byte array
        ByteArrayInputStream b64In = new ByteArrayInputStream (b64String.getBytes ("ISO-8859-1"));
        ByteArrayOutputStream bOut = new ByteArrayOutputStream();
        Base64Decoder b64conv = new Base64Decoder (b64In, bOut);
        b64conv.process();
        return bOut.toByteArray();
    }

    /**
     * Converts a serializable object to a Base64-encoded String.
     * 
     * @param o Serializable object to be converted
     * @return Base64 encoded ASCII string that is the serialized object.
     * @throws IOException
     */
    public String convertObjectToB64String (Serializable o)
            throws IOException
    {
        // Convert the object to a byte array
        ByteArrayOutputStream bOut = new ByteArrayOutputStream();
        ObjectOutputStream oOut = new ObjectOutputStream (bOut);
        oOut.writeObject (o);
        oOut.flush();
        oOut.close();

        return convertByteArrayToB64String (bOut.toByteArray());
    }

    /**
     * Converts a binary encoded byte array to a Base64-encoded String. This is
     * the second half of the process of converting an Object to a Base-64
     * encoded string.
     * 
     * @param byteArray byte array to be converted to an ASCII representation
     * @return String Base 64 encoded string
     * @throws IOException
     */
    public static String convertByteArrayToB64String (byte[] byteArray)
            throws IOException
    {
        // Convert the binary encoded byte array to a Base64 encoded byte array
        ByteArrayInputStream bIn = new ByteArrayInputStream (byteArray);
        ByteArrayOutputStream b64Out = new ByteArrayOutputStream();
        Base64Encoder b64conv = new Base64Encoder (bIn, b64Out);
        b64conv.process();
        return new String (b64Out.toByteArray(), "ISO-8859-1");
    }
}
