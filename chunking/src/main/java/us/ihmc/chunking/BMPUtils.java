package us.ihmc.chunking;

import java.util.Arrays;
import java.util.List;

/**
 * Created by lbunch on 4/18/17.
 */

public class BMPUtils {


    // file extensions for all supported image formats
    public static final String[] CHUNKABLE_IMAGE_FILE_EXTENSION_ARRAY = {"jpg", "jpeg", "png"};//{"jpg", "jpeg", "png", "bmp", "gif"};
    public static final List<String> CHUNKABLE_IMAGE_FILE_EXTENSIONS = Arrays.asList(CHUNKABLE_IMAGE_FILE_EXTENSION_ARRAY);


    /**
     * How many columns of pixels to skip based on the total number of chunks
     * the image will be broken down into
     *
     * @param ui8TotalNoOfChunks - the total number of chunks the image will be
     * broken down into
     * @return
     */
    public static int computeXIncrement(int ui8TotalNoOfChunks) {
        switch (ui8TotalNoOfChunks) {
            case 2:
                return 1;
            case 4:
                return 2;
            case 8:
                return 2;
            case 16:
                return 4;
        }
        return 0;
    }

    /**
     * How many rows of pixels to skip based on the total number of chunks the
     * image will be broken down into
     *
     * @param ui8TotalNoOfChunks - the total number of chunks the image will be
     * broken down into
     * @return
     */
    public static int computeYIncrement(int ui8TotalNoOfChunks) {
        switch (ui8TotalNoOfChunks) {
            case 2:
                return 2;
            case 4:
                return 2;
            case 8:
                return 4;
            case 16:
                return 4;
        }
        return 0;
    }

    /**
     * Which 'stripe' of pixels go into a chunk based on the id of the chunk and
     * the total number of chunks. Each chunk takes a different column (vertial
     * stripe) of pixels per X increment.
     *
     * @param ui8ChunkId - which chunk number
     * @param ui8TotalNoOfChunks - how many total chunks
     * @return
     */
    public static int computeXOffset(int ui8ChunkId, int ui8TotalNoOfChunks) {
        if ((ui8ChunkId == 0) || (ui8ChunkId > ui8TotalNoOfChunks)) {
            return 0;
        }
        if (computeXIncrement(ui8TotalNoOfChunks) == 0) {
            return 0;
        } else {
            return (ui8ChunkId - 1) % computeXIncrement(ui8TotalNoOfChunks);
        }
    }

    /**
     * Which 'stripe' of pixels go into a chunk based on the id of the chunk and
     * the total number of chunks. Each chunk takes a different row (horizontal
     * stripe) of pixels per X increment.
     *
     * @param ui8ChunkId - which chunk number
     * @param ui8TotalNoOfChunks - how many total chunks
     * @return
     */
    public static int computeYOffset(int ui8ChunkId, int ui8TotalNoOfChunks) {
        if ((ui8ChunkId == 0) || (ui8ChunkId > ui8TotalNoOfChunks)) {
            return 0;
        }
        if (computeYIncrement(ui8TotalNoOfChunks) == 0) {
            return 0;
        } else {
            return (ui8ChunkId - 1) / computeXIncrement(ui8TotalNoOfChunks);
        }
    }

    /**
     * Determine the chunk id based on the horizontal and vertical 'stripes'
     * (aka offsets) are being used
     *
     * @param ui8XOffset - which column
     * @param ui8YOffset - which row
     * @param ui8TotalNoOfChunks - how many total chunks
     * @return
     */
    public static int computeChunkIdForOffset(int ui8XOffset, int ui8YOffset, int ui8TotalNoOfChunks) {
        return ui8XOffset + (ui8YOffset * computeXIncrement(ui8TotalNoOfChunks)) + 1;
    }

    /**
     * Divide the numerator by the denominator and round the result up to the
     * next integer.
     *
     * ceiling (12, 2) = 6 ceiling (11, 2) = 6 ceiling (13, 2) = 7 ceiling (22,
     * 3) = 8 ceiling (22, 7) = 4
     *
     * @param ui32Numerator - the numerator
     * @param ui32Denominator - the denominator
     * @return
     */
    public static int ceiling(int ui32Numerator, int ui32Denominator) {
        int ui32ModValue = ui32Numerator % ui32Denominator;
        if (ui32ModValue == 0) {
            // Nothing to be done
            return ui32Numerator / ui32Denominator;
        } else {
            return (ui32Numerator / ui32Denominator) + 1;
        }
    }
}
