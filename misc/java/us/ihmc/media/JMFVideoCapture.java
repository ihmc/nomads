package us.ihmc.media;

import javax.media.Buffer;
import javax.media.MediaLocator;
import javax.media.Manager;

import javax.media.protocol.PushBufferDataSource;
import javax.media.protocol.PushBufferStream;
import javax.media.protocol.BufferTransferHandler;


/**
 * Models a class for supplying a stream of Buffers as a video.
 */
public class JMFVideoCapture implements BufferTransferHandler
{
    /**
     * Require some receiver for the video.
     */
    public JMFVideoCapture (BufferReceiver bufRecv)
    {
        _bufRecv = bufRecv; 
    }
    
    /**
     * JMFVideoCapture configured to use first video capture device.  
     */
    public void startCapture()
    {
        /*
         * JMF does not go through Windows Driver Model (WDM) or USB drivers. It
         * goes through VFW API, windows provides a wrapper for WDM devices for 
         * VFW API.  JMF on Linux uses V4L driver interface to access capture 
         * devices. 
         */
        String osName = System.getProperty("os.name");
        
        if (osName.equals("Linux")) {
            startCaptureFromLoc ("v4l://0");
        }
        else if (osName.startsWith("Windows")) {
            startCaptureFromLoc ("vfw://0");
        }
        else {
            System.out.println("OS: " + osName + " unsupported by us.ihmc.media.JMFImageCapture");
        }
    }

    /**
     * JMFVideoCapture configured to use file for input.
     * <p>
     * @param filePath
     */
    public void startCapture (String filePath)
    {
        startCaptureFromLoc ("file://" + filePath);    
    }
    
    /**
     * Start capturing video.
     * <p>
     * @param mediaLocation
     */
    private void startCaptureFromLoc (String mediaLocation)
    {
        try {
            // Create a new PushBufferDataSource to get buffers from the camera.
            MediaLocator locator = new MediaLocator (mediaLocation);
            _pbds = (PushBufferDataSource) Manager.createDataSource (locator);

            // Connect to the data source and then start the data source.
            _pbds.connect();
            _pbds.start();

            // Get the first stream (the video stream) from the data source and 
            // then set this class as the transfer handler.
            PushBufferStream[] streams = _pbds.getStreams();
            PushBufferStream firstStream = streams[0];
            
            // PushBufferStream notifies data handler (BufferTransferHandler
            // object registered with stream) when data is available to be pushed. 
            firstStream.setTransferHandler (this);
        } 
        catch (Exception e) {
            e.printStackTrace();    
        }
    }

    /**
     * Stop capturing video.
     */
    public void stopCapture()
    {
        try {
            _pbds.stop();
            _pbds.disconnect();
        } 
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    /**
     * Required by interface BufferTransferHandler method.  It is called every 
     * time a new Buffer is available. Please do not call this method directly.
     * <p>
     * @see     Class javax.media.protocol.BufferTransferHandler
     * @method      transferData (javax.media.protocol.PushBufferStream)
     */
    public void transferData (PushBufferStream stream) 
    {        
        try {
            // After this method is called we know a new Buffer is available, 
            // so we read that buffer and call the sendBuf method
            Buffer readBuf = new Buffer();
            stream.read (readBuf);
            _bufRecv.receiveBuffer (readBuf);    
        } 
        catch (Exception e) {
            e.printStackTrace();
        }
    }
        
    /**
     * Sets the BufferReciever from class to receive Buffer (images). There can 
     * be only one receiver.
     * <p>
     * @param bufRecv
     */
    public void setBufferReceiver (BufferReceiver bufRecv)
    {
        _bufRecv = bufRecv;    
    }
    
    
    private BufferReceiver       _bufRecv;
    private PushBufferDataSource _pbds;
}
