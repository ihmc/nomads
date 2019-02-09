package us.ihmc.media.util;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferByte;
import java.awt.image.DataBufferInt;
import java.awt.image.WritableRaster;

/**
 * Scales down an image in the form of a BufferedImage and returns a new BufferedImage.
 */
public class ImageScaler
{
    /**
     * Scale an image to a new size. The original size of the image must a multiple of the new size.
     * 
     * @param image     the original image
     * @param newWidth  the new width for the image
     * @param newHeight the new height for the image
     * 
     * @return          the newly scaled image
     * 
     * @exception  InvalidOperationException     if the operation is not permitted (in terms of the image sizes)
     * @exception  UnsupportedFormatException    if the image data is not represented as byte or int values
     */
    public static BufferedImage scaleImage (BufferedImage image, int newWidth, int newHeight)
        throws InvalidOperationException, UnsupportedFormatException
    {
        WritableRaster raster = image.getRaster();
        int width = raster.getWidth();
        int height = raster.getHeight();
        if ((newWidth <= 0) || (newHeight <= 0)) {
            throw new InvalidOperationException ("new width and height must be > 0");
        }
        if (((width % newWidth) > 0) || ((height % newHeight) > 0)) {
            throw new InvalidOperationException ("original width and height (" + width + "," + height + ") must be a multiple of " +
                                                 "new width and height (" + newWidth + "," + newHeight + ")");
        }

        if (raster.getDataBuffer() instanceof DataBufferByte) {
            return scaleByteBufferImage (image, newWidth, newHeight);
        }
        else if (raster.getDataBuffer() instanceof DataBufferInt) {
            return scaleIntBufferImage (image, newWidth, newHeight);
        }
        else {
            throw new UnsupportedFormatException ("cannot handle images with data of type " + raster.getDataBuffer().getClass().getName());
        }
    }

    private static BufferedImage scaleByteBufferImage (BufferedImage image, int newWidth, int newHeight)
    {
        WritableRaster raster = image.getRaster();
        WritableRaster newRaster = raster.createCompatibleWritableRaster (newWidth, newHeight);

        int width = raster.getWidth();
        int height = raster.getHeight();

        byte[] src = ((DataBufferByte)raster.getDataBuffer()).getData();
        byte[] dest = ((DataBufferByte)newRaster.getDataBuffer()).getData();

        int blockSizeX = width / newWidth;
        int blockSizeY = height / newHeight;
        int blockArea = blockSizeX * blockSizeY;

        for (int x = 0; x < newWidth; x++) {
            for (int y = 0; y < newHeight; y++) {
                int totalRedValue = 0;
                int totalGreenValue = 0;
                int totalBlueValue = 0;

                for (int i = 0; i < blockSizeX; i++) {
                    for (int j = 0; j < blockSizeY; j++) {
                        int col = x * blockSizeX + i;
                        int line = y * blockSizeY + j;
                        int pixelIndex = ((line * width) + col) * 3;

                        int redValue = src[pixelIndex+2];
                        int greenValue = src[pixelIndex+1];
                        int blueValue = src[pixelIndex+0];

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

                int newPixelIndex = ((y * newWidth) + x) * 3;
                dest [newPixelIndex+2] = (byte) (totalRedValue / blockArea);
                dest [newPixelIndex+1] = (byte) (totalGreenValue / blockArea);
                dest [newPixelIndex+0] = (byte) (totalBlueValue / blockArea);
            }
        }

        return new BufferedImage (image.getColorModel(), newRaster, false, null);
    }

    private static BufferedImage scaleIntBufferImage (BufferedImage image, int newWidth, int newHeight)
    {
        WritableRaster raster = image.getRaster();
        WritableRaster newRaster = raster.createCompatibleWritableRaster (newWidth, newHeight);

        int width = raster.getWidth();
        int height = raster.getHeight();

        int[] src = ((DataBufferInt)raster.getDataBuffer()).getData();
        int[] dest = ((DataBufferInt)newRaster.getDataBuffer()).getData();

        int blockSizeX = width / newWidth;
        int blockSizeY = height / newHeight;
        int blockArea = blockSizeX * blockSizeY;

        for (int x = 0; x < newWidth; x++) {
            for (int y = 0; y < newHeight; y++) {
                int totalRedValue = 0;
                int totalGreenValue = 0;
                int totalBlueValue = 0;

                for (int i = 0; i < blockSizeX; i++) {
                    for (int j = 0; j < blockSizeY; j++) {
                        int col = x * blockSizeX + i;
                        int line = y * blockSizeY + j;
                        int pixelIndex = (line * width) + col;

                        int redValue = (src[pixelIndex] >> 16) & 0x000000FF;
                        int greenValue = (src[pixelIndex] >> 8) & 0x000000FF;
                        int blueValue = (src[pixelIndex] >> 0) & 0x000000FF;

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

                int newPixelIndex = (y * newWidth) + x;
                dest[newPixelIndex] = ((totalRedValue / blockArea) << 16) +
                                      ((totalGreenValue / blockArea) << 8) +
                                      ((totalBlueValue / blockArea) << 0);
            }
        }

        return new BufferedImage (image.getColorModel(), newRaster, false, null);
    }
}
