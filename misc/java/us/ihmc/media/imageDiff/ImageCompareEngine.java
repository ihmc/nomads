package us.ihmc.media.imageDiff;

public interface ImageCompareEngine
{
    public Object computeChecksum (Object image, float granularity);
    public float compareChecksums (Object checkSumOne, Object checkSumTwo);    
}
