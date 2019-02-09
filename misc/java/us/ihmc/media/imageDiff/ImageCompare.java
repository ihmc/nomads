package us.ihmc.media.imageDiff;

import javax.media.Buffer;

/**
 * Methods for comparing two images
 */
public class ImageCompare
{
    
    
    public ImageCompare (ImageCompareEngine ice, float threshold, float granularity)
    {
        _ice = ice;
        _threshold = threshold;
        _granularity = granularity;
    }

    public void setThreshold (float threshold)
    {
        _threshold = threshold;
    }
    
    public float getThreshold()
    {
        return _threshold;
    }
   
    public void setGranularity (float granularity)
    {
        _granularity = granularity;
        // if the granularity changes recompute the checksum
        if (_baselineChecksum != null) {
            _baselineChecksum = _ice.computeChecksum (_baselineImage, _granularity);
        }
    }
    
    public float getGranularity()
    {
        return _granularity;
    }

    public void setBaselineImage (Buffer baselineImage)
    {
        _baselineImage = baselineImage;
        _baselineChecksum = _ice.computeChecksum (baselineImage, _granularity);
    }
    
   
    // Compare newImage with initImage
    // If the difference is larger than the specified threshold, replace initImage with newImage and return true
    // Otherwise, return false
    public boolean compare (Buffer newImage)
    {
        Object newCheckSum = _ice.computeChecksum (newImage, _granularity);
        _difference = _ice.compareChecksums (_baselineChecksum, newCheckSum);
        
        if (_difference > _threshold) {
            _baselineImage = newImage;
            _baselineChecksum = newCheckSum;
            return true;
        } else {
            return false;    
        }
    }

    public float getDifference()
    {
        return _difference;
    }
    
    // PRIVATE CLASS VARIABLE
    ImageCompareEngine _ice;
    Buffer             _baselineImage = null;
    Object             _baselineChecksum = null;
    float              _threshold = 0;
    float              _granularity = 0;
    float              _difference = 0;
    
}
