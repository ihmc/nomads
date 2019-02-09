package us.ihmc.media.imageDiff;

/**
 * Class chops up into blocks and averages the pixels in side those blocks. This 
 * allows images to be compared for image changes.  As a side effect the 
 * resulting image is shrunken.
 * 
 * @author Paul Groth
 */

import javax.media.format.RGBFormat;
import javax.media.Buffer;
import javax.media.Format;

import us.ihmc.media.imageDiff.ImageCompareEngine;
import us.ihmc.media.util.BufferScaler;


public class BlockEngine implements ImageCompareEngine
{
    /**
     * Compute the checksum of the image. Which is just a shrunken version of 
     * the image. The Buffer is expected to have an RGBFormat format.
     * <p>
     * @param image         the image, is expected to be a Buffer
     * @param granularity   the granularity of the resulting checksum the 
     *                      granularity is specified between 1 and 0.
     *                      1 being the orginal image's resolution and 0 being
     *                      equivalent to 1 pixel representing the entire image.
     * 
     * @return Object - this object is a Buffer containing an int[] in
     *                  the 32 bit format
     */
    public Object computeChecksum (Object image, float granularity)
    {
        Buffer bufImg = (Buffer) image;
        RGBFormat format = (RGBFormat)bufImg.getFormat();
        
        // Determine the new width and height of the image based
        // on the granularity
        int newWidth = (int)(format.getSize().width * granularity);
        int newHeight = (int)(format.getSize().height * granularity);
        
        // Determine whether the image is 8 or 32 bit and
        // call the appropriate method
        if (format.getBitsPerPixel() == 32) {
            return _bufScaler.scale32BitBuffer (bufImg, newWidth, newHeight);    
        }
        else {
            return _bufScaler.scale8BitBuffer (bufImg, newWidth, newHeight);    
        }        
    }
    
    /**
     * Compare the two checksums. Compare each block of the two images, 
     * returning the greatest difference.
     * <p>
     * @param Object checkSumOne - expected to be a Buffer 
     * @param Object checkSumTwo - expected to ba a Buffer
     * 
     * @return float - the largest difference between any
     *                 two blocks
     */
    public float compareChecksums (Object checkSumOne, Object checkSumTwo)
    {
        // If the either of the checksums are null return 0
        if (checkSumOne == null) {
            return 0.0f;
        }
        if (checkSumTwo == null) {
            return 0.0f;
        }
        
        // Get the data
        Buffer chkOneBuf = (Buffer)checkSumOne;
        Buffer chkTwoBuf = (Buffer)checkSumTwo;
        
        int cmpValue = 0;
        
        if (chkOneBuf.getFormat().getDataType() == Format.intArray) {
           int[] one = (int[])chkOneBuf.getData();
           int[] two = (int[])chkTwoBuf.getData();
           
           // Compute the difference between each block
           // Essentially a block is just a pixel
           for (int i = 0; i < one.length; i++) {
               int tmp = Math.abs (one[i] - two[i]);
               if (tmp > cmpValue) {
                   cmpValue = tmp;
               } 
           }
        }
        
        if (chkOneBuf.getFormat().getDataType() == Format.byteArray) {
           byte[] one = (byte[])chkOneBuf.getData();
           byte[] two = (byte[])chkTwoBuf.getData();
           
           // Compute the difference between each block
           // Essentially a block is just a pixel
           int i = 0;
           while (i < one.length) {
               int tmpR = one[i] - two[i];
               i++;
               int tmpG = one[i] - two[i];
               i++;
               int tmpB = one[i] - two[i];
               i++;
               int tmp = tmpR + tmpG + tmpB;
               
               if (tmp > cmpValue) {
                   cmpValue = tmp;
               } 
           }
        }
        
        return cmpValue;
    }
    
    
    private BufferScaler _bufScaler = new BufferScaler();
}
