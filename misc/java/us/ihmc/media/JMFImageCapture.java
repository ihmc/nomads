package us.ihmc.media;

/**
 * JMFImageCapture
 *
 * @author Marco Carvalho <mcarvalho@ai.uwf.edu>
 * @version $Revision$
 * $Date$
 */

import java.awt.Dimension;
import java.awt.Image;

import javax.media.Buffer;
import javax.media.ConfigureCompleteEvent;
import javax.media.ControllerClosedEvent;
import javax.media.ControllerEvent;
import javax.media.ControllerListener;
import javax.media.Format;
import javax.media.Manager;
import javax.media.MediaLocator;
import javax.media.Player;
import javax.media.RealizeCompleteEvent;
import javax.media.control.FormatControl;
import javax.media.control.FrameGrabbingControl;
import javax.media.format.RGBFormat;
import javax.media.format.VideoFormat;
import javax.media.protocol.DataSource;
import javax.media.util.BufferToImage;


public class JMFImageCapture
{
    /**
     * Grabs a frame, returns true if frame was successfully grabbed.
     * <p>
     * @return true if valid frame, otherwise false 
     */
    public boolean grabFrame()
    {
        _currentFrame = doGrabFrame();
        if (_currentFrame == null) {
            return false;
        }
        return true;
    }

    /**
     * Returns grabbed frame as a javax.media.Buffer.  Must call grabFrame()
     * first.
     * <p>
     * @return the grabbed frame
     */
    public Buffer getFrameAsBuffer()
    {
        return _currentFrame;
    }

    /**
     * Returns grabbed frame as a java.awt.Image.  Must call grabFrame() first.
     * <p>
     * @return the grabbed frame as an AWT Image
     */
    public Image getFrameAsImage()
    {
        if (_currentFrame == null) {
            return null;
        }
        
        if (_bti == null) {
            _bti = new BufferToImage ((VideoFormat)_currentFrame.getFormat());
        }
        
        return _bti.createImage (_currentFrame);
    }

    /**
     * Returns the size of the frames being grabbed.
     * <p>
     * @return the frame size
     */
    public static Dimension getFrameSize()
    {
        return ((VideoFormat)_formCtrl.getFormat()).getSize();
    }

    /**
     * Returns Dimension (frame size) array of RGB supported formats.  
     * <p>
     * @return array of frame sizes
     */
    public static Dimension[] getSupportedFrameSizes()
    {
        Format[] formats = _formCtrl.getSupportedFormats();        
        int count = 0;
        for (int i = 0; i < formats.length; i++) {
            System.out.println ("format: " + formats[i]);
            if (formats[i] instanceof RGBFormat) {
                count++;
            }
        }
        
        Dimension[] sizes = new Dimension [count];        
        int index = 0;
        for (int i = 0; i < formats.length; i++) {
            if (formats[i] instanceof RGBFormat) {
                sizes[index++] = ((RGBFormat)formats[i]).getSize();
                System.out.println ("supported size: " + ((RGBFormat)formats[i]).getSize());
            }
        }
        
        return sizes;
    }

    /**
     * Sets the size of the frames to be grabbed.
     * <p>
     * @param d    the size in width and height of the frame
     * 
     * @exception IllegalArgumentException       specified size is not supported
     */
    public void setFrameSize (Dimension d) throws IllegalArgumentException
    {
        Format[] formats = _formCtrl.getSupportedFormats();
        for (int i = 0; i < formats.length; i++) {
            if (formats[i] instanceof RGBFormat) {
                if (((RGBFormat)formats[i]).getSize().equals (d)) {
                    destroy();
                    _desiredFormat = formats[i];
                    try {
                        init();
                    }
                    catch (Exception e) {
                        throw new IllegalArgumentException ("could not set frame size; nested exception = " + e);
                    }
                    _bti = null;       // Cached BufferToImage object will not be valid if format changes
                    
                    return;
                }
            }
        }
        throw new IllegalArgumentException ("frame size " + d + " is not supported");
    }

    /**
     * Returns the javax.media.Player instance being used to grab the frames.
     * <p>
     * @return the Player instance
     */
    public Player getPlayer()
    {
        return (_player);
    }

    /**
     * Destroys this instance of the JMFImageCapture class.
     */
    public void destroy()
    {
        try {
            _player.close();
            while (!_playerListener.closed()) {
                Thread.sleep (100);
            }
        } 
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Returns initialized instance of this class.  Parameter can be used to set 
     * file or known device as source for image capture.
     * <p>
     * @param mediaLocator      location of desired media.
     * 
     * @return instance of this class.
     */
    public static JMFImageCapture getInstance (String mediaLocator)
    {
        try {
            initIfNeeded (mediaLocator);
        }
        catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        
        return new JMFImageCapture();
    }

    /**
     * Returns a new instance of the JMFImageCapture configured to use first 
     * video capture device class.  
     * <p>
     * @return a new instance of the JMFImageCapture class
     */
    public static JMFImageCapture getInstance()
    {
        /*
         * JMF does not go through Windows Driver Model (WDM) or USB drivers. It
         * goes through VFW API, windows provides a wrapper for WDM devices for 
         * VFW API.  JMF on Linux uses V4L driver interface to access capture 
         * devices. 
         */
        try {
            String osName = System.getProperty("os.name");
            if (osName.equals("Linux")) {
                initIfNeeded ("v4l://0");
            }
            else if (osName.startsWith("Windows")) {
                initIfNeeded ("vfw://0");
            }
            else {
                System.out.println("OS: " + osName + " unsupported by us.ihmc.media.JMFImageCapture");
            }
        }
        catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        
        return new JMFImageCapture();
    }

    /**
     * FrameGrabbingControl object gets a Frame and returns it as a Buffer.
     * <p>
     * @return
     */
    protected synchronized static Buffer doGrabFrame()
    {
        return _fgCtrl.grabFrame();
    }

    /**
     * Confirms that jmf control class (FrameGrabbingControl) has not been 
     * previously created, remembers mediaLocator and calls init(). 
     * <p>
     * @param mediaLocator
     * @throws Exception
     */
    protected synchronized static void initIfNeeded (String mediaLocator) throws Exception
    {
        if (_fgCtrl != null) {
            // Already initialized
            return;
        }
        _mediaLocator = mediaLocator;
        init();
    }

    /**
     * Initializes Datasource, Player and obtains FrameGrabbingControl, FormatControl
     * @throws Exception
     */
    protected synchronized static void init() throws Exception
    {
        try {
            MediaLocator locator = new MediaLocator (_mediaLocator);
            _ds = Manager.createDataSource (locator);
            _player = Manager.createPlayer (_ds);
            _playerListener = new StatusListener();
            _player.addControllerListener (_playerListener);
            _player.realize();
            while (!_playerListener.realized()) {
                Thread.sleep (100);
            }
            _fgCtrl = (FrameGrabbingControl) _player.getControl ("javax.media.control.FrameGrabbingControl");
            _formCtrl = (FormatControl) _player.getControl ("javax.media.control.FormatControl");
            if (_desiredFormat != null) {
                _formCtrl.setFormat (_desiredFormat);
            }
            _player.start();
        }
        catch (Exception e) {
            _ds = null;
            _playerListener = null;
            _player = null;
            _fgCtrl = null;
            throw e;
        }
    }


    /**
     * JMFImageCapture.StatusListener
     * <p>
     * @author Marco Carvalho <mcarvalho@ai.uwf.edu>
     */
    protected static class StatusListener implements ControllerListener
    {
        public boolean configured()
        {
            return _configured;
        }
        
        public boolean realized()
        {
            return _realized;
        }
        
        public boolean closed()
        {
            return _closed;
        }

        public void controllerUpdate (ControllerEvent ce)
        {
            if (ce instanceof ControllerClosedEvent) {
                _closed = true;
            }
            
            else if (ce instanceof RealizeCompleteEvent) {
                _realized = true;
            }

            else if (ce instanceof ConfigureCompleteEvent) {
                _configured = true;
            }
        }
        
        
        protected boolean _configured = false;
        protected boolean _realized = false;
        protected boolean _closed = false;
    }


    private Buffer _currentFrame;
    private BufferToImage _bti;
//    private boolean _debug = true;

    private static String _mediaLocator;
    private static DataSource _ds;
    private static Player _player;
    private static FrameGrabbingControl _fgCtrl;
    private static FormatControl _formCtrl;
    private static Format _desiredFormat;
    private static StatusListener _playerListener;
}
