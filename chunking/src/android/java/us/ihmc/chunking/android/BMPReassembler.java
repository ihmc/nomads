package us.ihmc.chunking.android;

import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.chunking.android.BufferedImage;
import and.awt.Color;
import us.ihmc.chunking.BMPUtils;
/**
 * Reassembles image 'chunks' into a larger, higher resolution image.
 * Based on the idea that chunks are created by taking different 'stripes' 
 * of pixels from the original image:
 * 
 * original:	0,1,2,3
 * 				4,5,6,7
 * 
 * chunk 1: 	0,2		chunk 2:	1,3
 * 				4,6					5,7
 * 
 * Chunks 1 and 2 can be reassembled or 'woven' back together to recreate the original array of pixels.
 * 
 * This android-compatible version ultimately relies on android.graphics.Bitmap
 * 
 * @author lbunch
 *
 */
public class BMPReassembler {
	
	private static Logger logger = Logger.getLogger(BMPReassembler.class.getName());
	
	// the resulting higher resolution image
	private BufferedImage _pReassembledImage = null;
	// the total number of chunks the original image was broken into
	int _ui8TotalNoOfChunks = 0;
	// how many columns to skip for each chunk
	int _ui8XIncr = 0;
	// how many rows to skip for each chunk
	int _ui8YIncr = 0;
	// which chunks have already been reassembled into the resulting image
	// this array will contain true at the index of each chunk id 
	// that has already been woven back into the result
	// note that chunk ids start at 1 (not 0)
	boolean[] _abAddedChunks = null;
	
	/**
	 * Initialize this reassembler based on the total number of chunks the original image was broken into.
	 * 
	 * @param ui8TotalNoOfChunks - the total number of chunks the original image was broken into.
	 */
	public BMPReassembler(int ui8TotalNoOfChunks) {
		_ui8TotalNoOfChunks = ui8TotalNoOfChunks;
		_ui8XIncr = BMPUtils.computeXIncrement (ui8TotalNoOfChunks);
	    _ui8YIncr = BMPUtils.computeYIncrement (ui8TotalNoOfChunks);
	    _abAddedChunks = new boolean[ui8TotalNoOfChunks + 1];
	}
	
	/**
	 * Weave the given 'chunk' image into the resulting reassembled image.
	 * @param pChunk - the chunk image to weave into the larger resulting image
	 * @param ui8ChunkId - which of the total number of chunks is the given chunk image (so the pixels are placed in the right 'stripe')
	 * 						note that chunk ids start with 1 (not 0)
	 * @return 0 for success or a negative value for an error
	 */
	public int incorporateChunk (BufferedImage pChunk, int ui8ChunkId)
	{
		// ensure this reassembler has been properly initialized.
	    if ((_ui8XIncr == 0) || (_ui8YIncr == 0)) {
	    	logger.log(Level.WARNING, "cannot handle reassembling BMP with {0} chunks\n\nOnly 2, 4, 8, or 16 total chunks is supported.", _ui8TotalNoOfChunks);
	        return -1;
	    }
		// validate parameters
	    if (pChunk == null) {
	    	logger.log(Level.WARNING, "chunk parameter is null\n");
	        return -2;
	    }
	    if ((ui8ChunkId == 0) || (ui8ChunkId > _ui8TotalNoOfChunks)) {
	    	logger.log(Level.WARNING, "chunk id of {0} is invalid\n", ui8ChunkId);
	        return -3;
	    }
	    // create a blank resulting image if it hasn't been created already
	    if (_pReassembledImage == null) {
	    	_pReassembledImage = initBlankReassembledImage(pChunk);
	    }
	    // ensure the given chunk image is the right size to incorporate into the resulting image (based on the total number of chunks)
	    if ((_pReassembledImage.getWidth() != (pChunk.getWidth() * _ui8XIncr)) ||
	            (_pReassembledImage.getHeight() != (pChunk.getHeight() * _ui8YIncr))) {
	    	logger.log(Level.WARNING, "chunk size of {0}x{1} is not compatible with reassembled image size of {2}x{3} composed of {4} chunks\n", 
	    			new Object[]{pChunk.getWidth(), pChunk.getHeight(), _pReassembledImage.getWidth(), _pReassembledImage.getHeight(), _ui8TotalNoOfChunks});    
	    	return -5;
	    }
	    // which vertial stripe of pixels does this chunk go into
	    int ui8XOff = BMPUtils.computeXOffset (ui8ChunkId, _ui8TotalNoOfChunks);
	    // which horizontal stripe of pixels does this chunk go into
	    int ui8YOff = BMPUtils.computeYOffset (ui8ChunkId, _ui8TotalNoOfChunks);
	    int ui32XSrc = 0, ui32XDest = 0;
	    while (ui32XSrc < pChunk.getWidth()) {
	        int ui32YSrc = 0, ui32YDest = 0;
	        while (ui32YSrc < pChunk.getHeight()) {
	        	// copy the pixel from the given chunk image into the resulting reassembled image
	            int rgb = pChunk.getRGB (ui32XSrc, ui32YSrc);
	            _pReassembledImage.setRGB (ui32XDest+ui8XOff, ui32YDest+ui8YOff, rgb);
	            // next row in the chunk image
	            ui32YSrc++;
	            // next 'stripe' row in the larger result image
	            ui32YDest += _ui8YIncr;
	        }
	        // next column in the chunk image
	        ui32XSrc++;
	     // next 'stripe' column in the larger result image
	        ui32XDest += _ui8XIncr;
	    }
	    _pReassembledImage.finalizeSetRGB();
	    // record that this chunk id has been incorporated in the reassembled image
	    _abAddedChunks[ui8ChunkId] = true;
	    return 0;
	}

	
	/**
	 * Get the resulting reassembled image.
	 * @return
	 */
	public BufferedImage getReassembledImage() {
		return _pReassembledImage;
	}
	
	/**
	 * Set the starting reassembled image. 
	 * For use when starting with an image that already has some chunks incorporated
	 * and we want to add more chunks.
	 *
	 * @param partialResultImage - the partially reassembled image
	 * @param alreadyIncorporatedChunkIds - array of chunk ids for chunks that are already woven into the given partialResultImage
	 */
	public void setPartialResultImage (BufferedImage partialResultImage, int[] alreadyIncorporatedChunkIds) {
		_pReassembledImage = partialResultImage;
		for (int incorporatedId : alreadyIncorporatedChunkIds) {
			_abAddedChunks[incorporatedId] = true;
		}
	}

	/**
	 * When some but not all chunks have been woven into the reassembled image,
	 * this method 'fills in the blanks' of missing chunks by averaging the RGB values 
	 * of chunks that have been incorporated.
	 * 
	 * @return 0 for success, negative integer for error codes
	 */
	public int interpolate()
	{
		// ensure this reassembler has been properly initialized.
	    if ((_ui8XIncr == 0) || (_ui8YIncr == 0)) {
	    	logger.log(Level.WARNING, "BMPReassembler has not been initialized\n");
	        return -1;
	    }
	    if (_pReassembledImage == null) {
	    	logger.log(Level.WARNING, "no chunks have been incorporated\n");
	        return -2;
	    }
	    
	    // iterate over every horizontal and vertical 'stripe' where a stripe is the set of pixels from all chunks
	    int ui32X = 0;
	    while (ui32X < _pReassembledImage.getWidth()) {
	        int ui32Y = 0;
	        while (ui32Y < _pReassembledImage.getHeight()) {
	        	// the number of filled-in pixels found in the current stripe (vs. pixels left blank because some chunks haven't been incorporated yet)
	            int ui8SampleCount = 0;
	            // sum of red, green, blue, and alpha values across all filled-in pixels for the current stripe
	            int ui16SumR = 0, ui16SumG = 0, ui16SumB = 0, ui16SumA = 0;
	            // check every pixel in the current stripe (or really, grid block)
	            for (int ui8X = 0; ui8X < _ui8XIncr; ui8X++) {
	                for (int ui8Y = 0; ui8Y < _ui8YIncr; ui8Y++) {
	                	// if the pixel has been filled-in by an incorporated chunk, include the pixel ARGB value in the average
	                    if (_abAddedChunks[BMPUtils.computeChunkIdForOffset (ui8X, ui8Y, _ui8TotalNoOfChunks)]) {
	                        int rgb = _pReassembledImage.getRGB (ui32X + ui8X, ui32Y + ui8Y);
	                        ui16SumA += ((rgb >> 24) & 0xff);
	                        ui16SumR += ((rgb >> 16) & 0xff);
	                        ui16SumG += ((rgb >> 8) & 0xff);
	                        ui16SumB += ((rgb) & 0xff);
	                        ui8SampleCount++;
	                    }
	                }
	            }
	            if (ui8SampleCount == 0) {
	                // Should not have happened
	            	logger.log(Level.WARNING, "internal error - no samples found\n");
	                return -3;
	            }
	            // average all the ARGB values
	            int ui8AvgR = ui16SumR / ui8SampleCount;
	            int ui8AvgG = ui16SumG / ui8SampleCount;
	            int ui8AvgB = ui16SumB / ui8SampleCount;
	            int ui8AvgA = ui16SumA / ui8SampleCount;
	            // check every pixel in the current stripe/grid block
	            for (int ui8X = 0; ui8X < _ui8XIncr; ui8X++) {
	                for (int ui8Y = 0; ui8Y < _ui8YIncr; ui8Y++) {
	                	// if the pixel has not been filled-in by one of the incorporated chunks, fill it with the average ARGB of pixels that have been filled-in
	                    if (!_abAddedChunks[BMPUtils.computeChunkIdForOffset (ui8X, ui8Y, _ui8TotalNoOfChunks)]) {
	                    	_pReassembledImage.setRGB (ui32X + ui8X, ui32Y + ui8Y, new Color(ui8AvgR, ui8AvgG, ui8AvgB, ui8AvgA).getRGB());
	                    }
	                }
	            }
	            // next stripe row
	            ui32Y += _ui8YIncr;
	        }
	        // next stripe column
	        ui32X += _ui8XIncr;
	    }
	    _pReassembledImage.finalizeSetRGB();
	    // 0 means success
	    return 0;
	}

	
	/**
	 * Create a blank result image based on the size and image type 
	 * (e.g. alpha/no alpha) of the given smaller chunk image.
	 * The resulting image size will be a multiple larger than the given chunk based
	 * on the total number of chunks the original was fragmented into.
	 * @param pChunk
	 * @return
	 */
	private BufferedImage initBlankReassembledImage(BufferedImage pChunk) {
		BufferedImage blankReassembleImage = new BufferedImage (pChunk.getWidth() * _ui8XIncr, pChunk.getHeight() * _ui8YIncr, pChunk.getConfig());
		return blankReassembleImage;
	}
	

	
}
