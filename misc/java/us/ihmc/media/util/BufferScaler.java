package us.ihmc.media.util;

/**
 * Takes Images encoded as buffers and scales
 * them down. The class does not scale images larger.
 * 
 * @author Paul Groth
 */

import java.awt.Dimension;
import javax.media.format.RGBFormat;
import javax.media.Buffer;
import javax.media.Format;


public class BufferScaler
{
    public static Buffer scaleBuffer (Buffer bufImg, int newWidth, int newHeight)
    {
        Object imgData = bufImg.getData();
        if (imgData instanceof byte[]) {
            return scale8BitBuffer (bufImg, newWidth, newHeight);
        }
        else {
            return scale32BitBuffer (bufImg, newWidth, newHeight);
        }
    }

    /**
     * Scales an 8 Bit RGB Java Media Framework Buffer given a new, smaller, 
     * height and width. If either of the new height or width values are 0 then 
     * a 1 is substituted and the operation is performed. 
     * <p>
     * @param Buffer bufImg - a buffer containing an image in an 8 bit RGB. 
     *                        format the data is expected to be a byte[].
     * @param int newWidth -  the new width of the image.
     * @param int newHeight - the new height of the image.
     * 
     * @return Buffer - a new buffer containing the reduced image in 8 bit RGB 
     *                  format the data is in a int[].
     */
    public static Buffer scale8BitBuffer (Buffer bufImg, int newWidth, int newHeight)
    {
        // Get the format of the image.
        RGBFormat format = (RGBFormat)bufImg.getFormat();
        int height = format.getSize().height;
        int width = format.getSize().width;
        
        // Get the image data.
        byte[] imgData = (byte[]) bufImg.getData(); 
        
        if (newWidth == 0) {
            newWidth = 1;
        }
        
        if (newHeight == 0) {
            newHeight = 1;
        }
        
        int blockSizeX = (int)(width / newWidth);
        int blockSizeY = (int)(height / newHeight);
        
        // Compute the number of blocks in the x and y direction.
        int numBlockX = newWidth;
        int numBlockY = newHeight;
        
        // Create the array to hold the new image.
        byte[] reducedImgData = new byte[(numBlockX * numBlockY)*3];
        int resultArrayLoc = 0;
        
        for (int b = 0; b < numBlockY; b++) {
            for (int a = 0; a < numBlockX; a++) {
                int totalRedValue = 0;
                int totalGreenValue = 0;
                int totalBlueValue = 0;

                for (int i = 0; i < blockSizeX; i++) {
                    for (int j = 0; j < blockSizeY; j++) {
                        int line = b * blockSizeX + i;
                        int col = a * blockSizeY + j;
                        
                        int pixel = (line * format.getLineStride() + col*3);
                         
                        int redValue = imgData[pixel+2];
                        int greenValue = imgData[pixel+1];
                        int blueValue = imgData[pixel+0];
                        
                        
                        if (redValue < 0) {
                            redValue = redValue + 255;
                        }
                        if (blueValue < 0) {
                            blueValue = blueValue + 255;
                        }
                        if (greenValue < 0) {
                            greenValue = greenValue + 255;
                        }
                        
                        totalRedValue = totalRedValue + redValue;
                        totalGreenValue = totalGreenValue + greenValue;
                        totalBlueValue = totalBlueValue + blueValue;
                    }
                }
                

                totalRedValue = (totalRedValue / ((blockSizeX*blockSizeY)));
                totalGreenValue = (totalGreenValue / ((blockSizeX*blockSizeY)));
                totalBlueValue = (totalBlueValue / ((blockSizeX*blockSizeY)));

                reducedImgData[resultArrayLoc+2] = (byte)totalRedValue;
                reducedImgData[resultArrayLoc+1] = (byte)totalGreenValue;
                reducedImgData[resultArrayLoc+0] = (byte)totalBlueValue;
                
                resultArrayLoc = resultArrayLoc + 3;
                
            }            
        }
        
        int flipped = Format.TRUE;
        if (format.getFlipped() != Format.TRUE) {
            flipped = Format.FALSE;    
        }
        
        // Create a new RGBFormat and then create a new Buffer and return it
        RGBFormat ourFormat = new RGBFormat (new Dimension(numBlockX, numBlockY), 
                                                 reducedImgData.length, 
                                                 reducedImgData.getClass(), 
                                                 0, 
                                                 24, 
                                                 3, 
                                                 2, 
                                                 1,
                                                 3,
                                                 numBlockX * 3,
                                                 flipped,
                                                 format.getEndian());
     
        Buffer resultBuffer = new Buffer();
        resultBuffer.setFormat (ourFormat);
        resultBuffer.setData (reducedImgData);
        
        return resultBuffer;    
    }
     
    /**
     * Scales an 32 Bit RGB Java Media Framework Buffer given
     * a new, smaller, height and width. If either of the new height or
     * width values are 0 then a 1 is substituted and the operation is 
     * performed. 
     * 
     * @param Buffer bufImg - a buffer containing an image in an 32 bit RGB format
     *                        the data is expected to be a int[]
     * @param int newWidth - the new width of the image
     * @param int newHeight - the new height of the image
     * 
     * @return Buffer - a new buffer containing the reduced image in 32 bit RGB format
     *                  the data is in a int[]
     */
    public static Buffer scale32BitBuffer (Buffer bufImg, int newWidth, int newHeight)
    {
        RGBFormat format = (RGBFormat)bufImg.getFormat();
        int[] imgData = (int[])bufImg.getData(); 
        
        if (newWidth == 0) {
            newWidth = 1;
        }
        if (newHeight == 0) {
            newHeight = 1;
        }
        
        int height = format.getSize().height;
        int width = format.getSize().width;
        
        int blockSizeX = (int)(width / newWidth);
        int blockSizeY = (int)(height / newHeight);
        
        int numBlockX = newWidth;
        int numBlockY = newHeight;
        
        int[] reducedImgData = new int[numBlockX * numBlockY];
        int resultArrayLoc = 0;
        
        for (int b = 0; b < numBlockY; b++) {
            for (int a = 0; a < numBlockX; a++) {
                int totalRedValue = 0;
                int totalGreenValue = 0;
                int totalBlueValue = 0;
                for (int i = 0; i < blockSizeX; i++) {
                    for (int j = 0; j < blockSizeY; j++) {
                         int line = b * blockSizeX + j;
                         int col = a * blockSizeY + i;
                         int pixel = line * format.getLineStride() + col;
                         
                         int redValue = (imgData[pixel] & format.getRedMask()) >> 16;
                         int greenValue = (imgData[pixel] & format.getGreenMask()) >> 8;
                         int blueValue = (imgData[pixel] & format.getBlueMask());
                         
                       
                         totalRedValue = totalRedValue + redValue;
                         totalGreenValue = totalGreenValue + greenValue;
                         totalBlueValue = totalBlueValue + blueValue;
                         
                    }
                }
                
                totalRedValue = Math.abs (totalRedValue / (blockSizeX*blockSizeY));
                totalGreenValue = Math.abs (totalGreenValue / (blockSizeX*blockSizeY));
                totalBlueValue = Math.abs (totalBlueValue / (blockSizeX*blockSizeY));
                
                int value = (0 << 24) | (totalRedValue << 16) | (totalGreenValue << 8) | totalBlueValue;
                
                reducedImgData[resultArrayLoc] = value;
                resultArrayLoc++;
                
            }            
        }
        
        RGBFormat ourFormat = new RGBFormat (new Dimension(numBlockX, numBlockY), 
                                             reducedImgData.length, 
                                             reducedImgData.getClass(), 
                                             0, 
                                             32, 
                                             0x00FF0000, 
                                             0x0000FF00, 
                                             0x000000FF);
        
        
        Buffer resultBuffer = new Buffer();
        resultBuffer.setFormat (ourFormat);
        resultBuffer.setData (reducedImgData);
        
        return resultBuffer;
    }
}