package us.ihmc.media;

import java.awt.Image;

import javax.media.Buffer;
import javax.media.format.RGBFormat;

import us.ihmc.media.ImageCompareListener;
import us.ihmc.media.imageDiff.*;
import us.ihmc.media.JMFImageCapture; 

/**
 * Compare two images for differences
 */
public class JMFImageCompareService implements Runnable
{
    public JMFImageCompareService (ImageCompareListener icl, ImageCompareEngine ice)
    {
         _imgCap = JMFImageCapture.getInstance();
         _ic = new ImageCompare (ice, _threshold, _granularity);
         _icl = icl;
         
    }
    
    /**
     * Start the comparing the images from the camera
     * <p>
     * @param int waitTime - the time to pause between images
     */
    public void run()
    {
        System.out.println ("Starting...");
        
        // Wait until the image sensor is ready
        boolean startingUp = true;
        while (startingUp) {
            _imgCap.grabFrame();
            Buffer buf1 = _imgCap.getFrameAsBuffer();
            RGBFormat format = (RGBFormat)buf1.getFormat();
            if (buf1 != null && format != null) {
                _ic.setBaselineImage (buf1);
                
                startingUp = false;
            }
        }
        
        while (true) 
        {
            try {
                // First we grab a frame
                _imgCap.grabFrame();
                Buffer buf = _imgCap.getFrameAsBuffer();
                RGBFormat format = (RGBFormat)buf.getFormat();
                if (buf != null && format != null) {
                    _currentImage = _imgCap.getFrameAsImage();
                    _icl.imageUpdated();
                    if (_ic.compare (buf)) {
                        _baselineImage = _currentImage;
                        _numCaptures++;
                        _icl.changeOccurred();  
                    }
                }
                
                // wait a while to grab the next image
                Thread.sleep (_waitTime);
                
            } catch (Exception e) {
                e.printStackTrace();
                System.exit (0);
            }
        }     
    }
    
    public Image getBaselineImage()
    {
        return _baselineImage;
    }
    
    public Image getCurrentImage()
    {
        return _currentImage;    
    }
    
    public int getNumberOfCaptures()
    {
        return _numCaptures;    
    }
    
    public int getWaitTime()
    {
        return _waitTime;    
    }
    
    public void setWaitTime (int waitTime) 
    {
        _waitTime = waitTime;
    }
    
    public float getThreshold()
    {
        return _threshold;
    }
    
    public void setThreshold (float threshold)
    {
        _threshold = threshold;
        _ic.setThreshold (threshold);
    }
    
    public float getGranularity()
    {
        return _granularity;    
    }
    
    public void setGranularity (float granularity)
    {
        System.out.println ("Setting granularity to: " + granularity);
         _granularity = granularity;
         _ic.setGranularity (_granularity);
    }
    
    public float getDifference()
    {
        return _ic.getDifference();    
    }
    
    private ImageCompare         _ic;
    private ImageCompareListener _icl;
    private JMFImageCapture      _imgCap;
    private Image                _currentImage;
    private Image                _baselineImage;
    private int                  _waitTime = 100;
    private int                  _numCaptures = 0;
    private float                _threshold = 100;
    private float                _granularity = .15f;
    
    
   
}