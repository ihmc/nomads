package us.ihmc.chunking;

import java.util.Collection;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public interface Reassembler
{
    public byte[] reassemble (Collection<ChunkWrapper> chunks, Collection<AnnotationWrapper> annotations,
                              String mimeType, byte totalNumberOfChunks, byte compressionQuality);
}
