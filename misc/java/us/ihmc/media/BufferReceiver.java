package us.ihmc.media;

import javax.media.Buffer;

/**
 * Classes that wish to recieve Buffer (images) by registering with the 
 * JMFVideoCapture class should implement this interface.
 */
public interface BufferReceiver
{
    public void receiveBuffer (Buffer buf);    
}