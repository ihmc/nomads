/**
 * SerializableImage
 *
 * @author Marco Carvalho < mcarvalho@ai.uwf.edu >
 * @version $Revision$
 * $Date$
 */

package us.ihmc.media.util;

import java.awt.Image;
import java.io.Serializable;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ByteArrayOutputStream;

import com.sun.jimi.core.util.JimiImageSerializer;

public class SerializableImage implements Serializable
{

    public SerializableImage(Image img)
    {
        _img = img;
    }

    private void writeObject(ObjectOutputStream stream) throws java.io.IOException 
    {
        JimiImageSerializer jimiImageSerializer = new JimiImageSerializer(_img);
        stream.writeObject(jimiImageSerializer);
        stream.flush();
    }

    private void readObject(ObjectInputStream stream) throws java.io.IOException 
    {
        try {
            JimiImageSerializer ser = (JimiImageSerializer) stream.readObject();
            _img = ser.getImage();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    public static int getSerializableImageSize(Image img)
    {
        int imgSize = -1;
        try {
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            ObjectOutputStream obj = new ObjectOutputStream(bos);
            JimiImageSerializer jimiImageSerializer = new JimiImageSerializer(img);
            obj.writeObject(jimiImageSerializer);
            byte[] barray = bos.toByteArray();
            imgSize = barray.length;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return(imgSize);
    }

    public Image getImage()
    {
        return(_img);
    }
  
    private transient Image _img;
}
