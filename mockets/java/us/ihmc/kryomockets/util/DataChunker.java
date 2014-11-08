/*
 * DataChunker.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets.util;


import org.apache.log4j.Logger;

import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * DataChunker.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DataChunker
{
    public final static int CHUNK_SIZE = 1024;
    private final static org.apache.log4j.Logger LOG = Logger.getLogger(DataChunker.class);

    public static Data[] split (Data source)
    {
        if (source == null || source.getType() == null || source.getSenderUUID() == null)
            throw new IllegalArgumentException("Data, type and sender can't be null");

        Data[] chunked;
        if (source.getRawData() == null || source.getRawData().length <= CHUNK_SIZE) {
            chunked = new Data[1];
            chunked[0] = source;  //nothing to be done
            return chunked;
        }

        chunked = divideArray(source.getId(), source.getType(), source.getSenderUUID(), source.getRawData(),
                CHUNK_SIZE);
        LOG.debug("Source data chunked in " + chunked.length + " parts");

        return chunked;
    }

    public static Data reassemble (Data[] chunked)
    {
        if (chunked == null ||
                chunked[0] == null ||
                chunked[0].getType() == null ||
                chunked[0].getSenderUUID() == null ||
                chunked[0].getTotalLength() <= 0)
            throw new IllegalArgumentException("Chunked data, type and sender can't be null");

        Data d = new Data(chunked[0].getId());
        d.setType(chunked[0].getType());
        d.setSenderUUID(chunked[0].getSenderUUID());
        d.setChunked(false);

        //rebuilding data
        byte[] rawData = new byte[chunked[0].getTotalLength()];
        ByteBuffer buffer = ByteBuffer.wrap(rawData);
        for (int i = 0; i < chunked.length; i++) {
            byte[] chunkedRawData = chunked[i].getRawData();
            LOG.debug("Chunked raw data for chunk #" + (i + 1) + " is long " + chunkedRawData.length + " / " +
                    chunked[0].getTotalLength());
            if (chunked[i] == null) {
                throw new IllegalArgumentException("Missing chunk #" + (i + 1));
            }

            buffer.put(chunked[i].getRawData());
        }

        d.setRawData(buffer.array());

        return d;
    }

    private static Data[] divideArray (String id, DataType type, String sender, byte[] source, int chunksize)
    {
        byte[][] ret = new byte[(int) Math.ceil(source.length / (double) chunksize)][chunksize];
        int start = 0;
        Data[] newData = new Data[ret.length];

        for (int i = 0; i < ret.length; i++) {
            ret[i] = Arrays.copyOfRange(source, start, start + chunksize);
            start += chunksize;

            LOG.debug("Doing split. Generated chunk #" + (i + 1) + "/" + ret.length + " of length " + chunksize + "/"
                    + (chunksize * ret.length));

            Data d = new Data(id);
            d.setChunked(true);
            d.setTotalChunks(ret.length);
            d.setTotalLength((chunksize * ret.length));
            d.setChunkNumber(i);
            d.setRawData(ret[i]);
            d.setType(type);
            d.setSenderUUID(sender);
            newData[i] = d;
        }

        return newData;
    }
}
