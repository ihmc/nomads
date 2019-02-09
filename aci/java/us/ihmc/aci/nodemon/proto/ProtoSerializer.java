package us.ihmc.aci.nodemon.proto;

import com.google.protobuf.InvalidProtocolBufferException;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;

import java.util.Objects;

/**
 * ProtoSerializer.java
 * <p/>
 * Class <code>ProtoSerializer</code> handles serialization and deserialization of Containers.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ProtoSerializer
{
    /**
     * Serializes this <code>Container</code> into a byte array.
     *
     * @param c the ProtoBuf <code>Container</code> to serialize.
     * @return a byte array of the serialized object
     */
    public static byte[] serialize (Container c)
    {
        Objects.requireNonNull(c, "Container to be serialized can't be null");
        byte[] data = c.toByteArray();
        log.trace("Serialized Container. Size: " + data.length);
        return data;
    }

    /**
     * Deserializes this byte array to a Container.
     *
     * @param buffer the byte array to deserialize.
     * @return the deserialized <code>Container</code>.
     * @throws InvalidProtocolBufferException if unable to deserialize the object correctly
     */
    public static Container deserialize (byte[] buffer) throws InvalidProtocolBufferException
    {
        Objects.requireNonNull(buffer, "Data buffer can't be null");
        if (buffer.length < 1) {
            throw new IllegalArgumentException("Data buffer length can't be < 1");
        }
        log.trace("Deserializing Proto data.. Size: " + buffer.length);
        return Container.parseFrom(buffer);
    }

    private static final Logger log = Logger.getLogger(ProtoSerializer.class);
}
