package us.ihmc.media.util;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.Panel;

import java.awt.image.ImageObserver;

/**
 * A component that is used to show images. Handles using an ImageObserver to wait for an image to be
 * loaded and then repainting to guarantee that the image is drawn.
 */
public class ImageComponent extends Panel
{
    public ImageComponent()
    {
        _maintainAspectRatio = true;
    }

    /**
     * Set the image to display
     * 
     * @param i - the image to display
     */
    public void setImage (Image i)
    {
        _image = i;
        repaint();
    }

    /**
     * Set the preferred size for this component. If the preferred size is set, the component returns
     * this value when the layout manager queries the component for the preferred size.
     * 
     * @param d - the dimension to use as the preferred size
     */
    public void setPreferredSize (Dimension d)
    {
        _preferredSize = d;
    }
    
    /**
     * Clears the preferred size (if set) for this component. If the preferred size if cleared, the
     * component relies on the superclass to determine the preferred size
     */
    public void clearPreferredSize()
    {
        _preferredSize = null;
    }

    public void update (Graphics g)
    {
        paint (g);
    }
    
    public void paint (Graphics g)
    {
        Dimension d = getSize();
        if (_image != null) {
            if (_maintainAspectRatio) {
                int compWidth = d.width;
                int compHeight = d.height;
                int imageWidth = _image.getWidth (this);
                int imageHeight = _image.getHeight (this);
                float scale = 1.0f;
                if ((imageWidth > 0) && (imageHeight > 0)) {
                    /*if ((imageWidth > compWidth) && (imageHeight > compHeight)) {
                        // The image is larger in both dimensions
                        // Find which dimension is larger and needs to dictate the size reduction
                        if ((((float)imageWidth) / (float) (compWidth)) > (((float) imageHeight) / (float) (compHeight))) {
                            // The width is larger - scale the image to fit the width
                            scale = ((float) imageWidth) / ((float) compWidth);
                        }
                        else {
                            // The height is larger - scale the image to fit the height
                            scale = ((float) imageHeight) / ((float) compHeight);
                        }
                    }
                    else if (imageWidth > compWidth) {
                        // The width is larger but the height is smaller - use the width to scale the image
                        scale = ((float) imageWidth) / ((float) compWidth);
                    }
                    else if (imageHeight > compHeight) {
                        // The height is larger but the width is smaller - use the height to scale the image
                        scale = ((float) imageHeight) / ((float) compHeight);
                    }
                    else {
                        // The image is smaller in both dimensions
                        // Find which dimension is the larger and needs to dictate the size increase
                        if ((((float) imageWidth) / (float) compWidth) > (((float) imageHeight) / (float) (compHeight))) {
                            // The width is larger - scale the image to fit the width
                            scale = ((float) imageWidth) / ((float) compWidth);
                        }
                        else {
                            // The height is larger - scale the image to fit the height
                            scale = ((float) imageHeight) / ((float) compHeight);
                        }
                    }*/
                    
                    if ((((float) imageWidth) / (float) compWidth) > (((float) imageHeight) / (float) (compHeight))) {
                        // The width is larger - scale the image to fit the width
                        scale = ((float) imageWidth) / ((float) compWidth);
                    }
                    else {
                        // The height is larger - scale the image to fit the height
                        scale = ((float) imageHeight) / ((float) compHeight);
                    }
                    int newImageWidth = ((int) (imageWidth / scale));
                    int newImageHeight = ((int) (imageHeight / scale));
                    //g.clearRect (0, 0, d.width, d.height);
                    g.drawImage (_image,
                                 (compWidth - newImageWidth) / 2,
                                 (compHeight - newImageHeight) / 2,
                                 newImageWidth,
                                 newImageHeight,
                                 this);
                }
                else {
                    g.clearRect (0, 0, d.width, d.height);
                    g.drawRect (0, 0, d.width-1, d.height-1);
                }
            }
        }
        else {
            g.clearRect (0, 0, d.width, d.height);
            g.drawRect (0, 0, d.width-1, d.height-1);
        }
    }

    public Dimension getPreferredSize()
    {
        if (_preferredSize != null) {
            return _preferredSize;
        }
        else {
            return super.getPreferredSize();
        }
    }

    /**
     * Listener defined by ImageObserver
     */
    public boolean imageUpdate (Image image, int infoFlags, int x, int y, int width, int height)
    {
        if ((infoFlags & ImageObserver.ALLBITS) != 0) {
            // Entire image has been loaded - repaint and stop further updates
            repaint();
            return false;
        }
        else {
            // We don't have the whole image yet - continue to get updates
            return true;
        }
    }

    protected Image _image;
    protected Dimension _preferredSize;
    protected boolean _maintainAspectRatio;
}
