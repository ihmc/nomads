package us.ihmc.chunking.android;

import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.chunking.BMPUtils;

/**
 * Fragments images into 2, 4, 8, or 16 smaller 'chunks' by: converting the
 * original image to a bitmap, creating a new smaller image of size width/N,
 * height/M, copying only every Nth column and Mth row from the original image
 * to the resulting smaller chunk.
 *
 * Each of the smaller chunk images is comprised of a different set of rows and
 * columns of pixels from the original image's pixels. Such that when all of the
 * chunk images are reassembled, it recreates the full quality of the original
 * image.
 *
 * columns of pixels to chunk numbers example for 4 chunks: ||||||||.....
 * 12341234.....
 *
 * rows of pixels to chunk numbers example for 4 chunks: - 1 - 2 - 3 - 4 - 1 - 2
 * - 3 - 4
 *
 * @author lbunch
 *
 */
public class BMPFragmenter {

    private static Logger logger = Logger.getLogger(BMPFragmenter.class.getName());

    /**
     * Extracts one of N total smaller 'chunk' images from the given original
     * image.
     *
     * @param pSourceImage - the original image from which to extract a smaller
     * chunk
     * @param ui8DesiredChunkId - which chunk to extract, determines which rows
     * and columns of the original image are copied into the chunk
     * @param ui8TotalNoOfChunks - the total number of chunks the original image
     * will be broken into
     * @return the BufferedImage containing the smaller chunk image comprised of
     * every Nth column and Mth row of the original image pixels.
     */
    public static BufferedImage fragmentBMP(BufferedImage pSourceImage, int ui8DesiredChunkId, int ui8TotalNoOfChunks) {
        // validate the parameters

        if (pSourceImage == null) {
            logger.log(Level.WARNING, "source image is null");
            return null;
        }
        if (ui8DesiredChunkId == 0) {
            // chunk id's start at 1
            logger.log(Level.WARNING, "desired chunk id of 0 is invalid\n");
            return null;
        }
        if (ui8DesiredChunkId > ui8TotalNoOfChunks) {
            logger.log(Level.WARNING, "desired chunk id {0} exceeds total number of chunks {1}\n", new Object[]{(int) ui8DesiredChunkId, (int) ui8TotalNoOfChunks});
            return null;
        }

        // columns of pixels to skip based on the number of chunks
        int ui8XIncr = BMPUtils.computeXIncrement(ui8TotalNoOfChunks);
        // rows of pixels to skip
        int ui8YIncr = BMPUtils.computeYIncrement(ui8TotalNoOfChunks);
        if ((ui8XIncr == 0) || (ui8YIncr == 0)) {
            logger.log(Level.WARNING, "cannot handle chunking BMP into {0} chunks.\nOnly 2, 4, 8, or 16 total chunks is supported.", ui8TotalNoOfChunks);
            return null;
        }
        logger.log(Level.FINEST, "image original size is \tx={0},y={1}", new Object[]{pSourceImage.getWidth(), pSourceImage.getHeight()});
        // determine the size of the smaller resulting chunk image
        long ui32NewWidth = BMPUtils.ceiling(pSourceImage.getWidth(), ui8XIncr);
        long ui32NewHeight = BMPUtils.ceiling(pSourceImage.getHeight(), ui8YIncr);
        logger.log(Level.FINEST, "image new size is \tx={0},y={1}", new Object[]{(int) ui32NewWidth, (int) ui32NewHeight});

        // create the resulting image - always use 3 colors model
        BufferedImage pChunkedImage = null;

        pChunkedImage = new BufferedImage((int) ui32NewWidth, (int) ui32NewHeight, BufferedImage.TYPE_INT_RGB);

        // which column to take from the original image based on the chunk id
        int ui8XOff = BMPUtils.computeXOffset(ui8DesiredChunkId, ui8TotalNoOfChunks);
        // which row to take from the original image based on the chunk id
        int ui8YOff = BMPUtils.computeYOffset(ui8DesiredChunkId, ui8TotalNoOfChunks);
        // the source (original) and destination (chunk) pixel X values
        long ui32XSrc = 0, ui32XDest = 0;
        while (ui32XSrc + ui8XOff < pSourceImage.getWidth() && ui32XDest < pChunkedImage.getWidth()) {
            // the source (original) and destination (chunk) pixel Y values
            long ui32YSrc = 0, ui32YDest = 0;
            while (ui32YSrc + ui8YOff < pSourceImage.getHeight() && ui32YDest < pChunkedImage.getHeight()) {
                try {
                    // get the pixel from the Nth column of the current row in the original image
                    int rgb = pSourceImage.getRGB((int) ui32XSrc + ui8XOff, (int) ui32YSrc + ui8YOff);
                    // copy the original pixel to the resulting chunk image
                    pChunkedImage.setRGB((int) ui32XDest, (int) ui32YDest, rgb);
                } catch (Exception e) {
                    logger.log(Level.WARNING, "problem converting image RGB for source x={0},y={1} and destination x={2},y={3}",
                            new Object[]{(int) ui32XSrc + ui8XOff, (int) ui32YSrc + ui8YOff, (int) ui32XDest, (int) ui32YDest});
                    logger.log(Level.WARNING, "  ", e);
                }
                // skip M rows of pixels in the source image
                ui32YSrc += ui8YIncr;
                // go to the next row in the resulting chunk image
                ui32YDest++;
            }
            // skip N columns of pixels in the source image
            ui32XSrc += ui8XIncr;
            // go to the next column in the resulting chunk image
            ui32XDest++;
        }
        pChunkedImage.finalizeSetRGB();
        return pChunkedImage;
    }

    //		int alpha = pixel & 0xff000000;
//    	int red = (pixel >> 16) & 0xff;
//    	int green = (pixel >> 8) & 0xff;
//    	int blue = (pixel) & 0xff;
}
