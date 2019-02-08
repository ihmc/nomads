package us.ihmc.aci.util.dspro.soi;

import java.util.ConcurrentModificationException;
import java.util.Properties;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyInterface;
import us.ihmc.aci.util.dspro.LogUtils;
import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.comm.CommException;
import us.ihmc.util.Base64Encoder;
import us.ihmc.util.StringUtil;

/**
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class TrackHandler implements Runnable
{
    private final static Logger LOG = LogUtils.getLogger(TrackHandler.class);
    private final BlockingQueue<Properties> _dataToPuh = new LinkedBlockingQueue<>();

    protected final DisServiceProProxyInterface _dspro;

    public TrackHandler (DisServiceProProxyInterface dspro)
    {
        _dspro = dspro;
    }

    @Override
    public void run ()
    {
        while (true) {
            try {
                Properties properties = _dataToPuh.poll(Long.MAX_VALUE, TimeUnit.SECONDS);
                if (properties != null) {
                    try {
                        doPushPro(properties);
                    }
                    catch (CommException ex) {
                        LOG.warn(ex.getMessage());
                    }
                }
            }
            catch (InterruptedException ex) {}
            catch (Exception ex) {
                LOG.error(StringUtil.getStackTraceAsString(ex));
            }
        }
    }

    public synchronized boolean setActualPosition (String nodeId, float fLatitude, float fLongitude, float fAltitude,
                                                   String location, String note) throws CommException
    {
        Properties properties = TrackUtils.getTrackDataProperties(nodeId, fLatitude, fLongitude);
        properties.setProperty(TrackExchangeFormat.RESOURCE_INSTANCE_ID, Long.toString(System.currentTimeMillis()));

        _dataToPuh.add(properties);

        return true;
    }

    /**
     * This method fulfills the DSPro Message with the proper metadata.
     *
     * @param dataProp    properties object containing the parsed message
     */
    protected void doPushPro (Properties dataProp) throws CommException
    {
        dataProp = TrackUtils.addTrackPropertiesToTrackData(dataProp, ExchangeFormat.Action.Insert.toString());

        Properties metaDataProp = TrackUtils.getTrackMetadataProperties(dataProp, ExchangeFormat.Action.Insert
                .toString());
        String[] metaDataAttributes = {
                MetadataElement.dataName.toString(),
                MetadataElement.description.toString(),
                MetadataElement.dataFormat.toString(),
                MetadataElement.leftUpperLatitude.toString(),
                MetadataElement.rightLowerLatitude.toString(),
                MetadataElement.leftUpperLongitude.toString(),
                MetadataElement.rightLowerLongitude.toString(),
                MetadataElement.applicationMetadataFormat.toString(),
                MetadataElement.applicationMetadata.toString()};

        Track track = new Track (dataProp.getProperty(TrackExchangeFormat.RESOURCE_OBJECT_ID),
                                 Double.parseDouble(dataProp.getProperty(TrackExchangeFormat.EVENT_LATITUDE)),
                                 Double.parseDouble(dataProp.getProperty(TrackExchangeFormat.EVENT_LONGITUDE)),
                                 dataProp.getProperty(TrackExchangeFormat.TRACK_MIL_STD_2525_SYMBOL_ID));

        String[] metaDataValues = {
                metaDataProp.get(MetadataElement.dataName.toString()).toString(),
                metaDataProp.get(MetadataElement.description.toString()).toString(),
                metaDataProp.get(MetadataElement.dataFormat.toString()).toString(),
                metaDataProp.get(MetadataElement.leftUpperLatitude.toString()).toString(),
                metaDataProp.get(MetadataElement.rightLowerLatitude.toString()).toString(),
                metaDataProp.get(MetadataElement.leftUpperLongitude.toString()).toString(),
                metaDataProp.get(MetadataElement.rightLowerLongitude.toString()).toString(),
                ApplicationMetadataFormat.Soigen2_Json_Base64.toString(),
                new Base64Encoder(track.toJson()).processString()
        };

        String messageId = null;
        while (true) {
            try {
                messageId = _dspro.pushPro((short) 0, "track",
                        dataProp.getProperty(TrackExchangeFormat.RESOURCE_OBJECT_ID),
                        dataProp.getProperty(TrackExchangeFormat.RESOURCE_INSTANCE_ID),
                        metaDataAttributes, metaDataValues,
                        null, (long) 0, (short) 0, (short) 0);
                break;
            }
            catch (ConcurrentModificationException e) {
            }
        }

        if (messageId == null) {
            LOG.warn("DSPro: error while pushing the message with pushPro, messageId is null");
        }
        else {
            LOG.info("Track submitted at time " + System.currentTimeMillis() + ", " + messageId
                    + ", " + dataProp.getProperty(TrackExchangeFormat.RESOURCE_OBJECT_ID)
                    + " , " + dataProp.getProperty(TrackExchangeFormat.RESOURCE_INSTANCE_ID));
        }
    }

    /**
     * Configure which implementation of StAX that will be used by the
     * application. This is a hack to use the standard Java XML processor,
     * not available on the Android API
     */
    protected void setFakeContextClassLoader ()
    {
        System.setProperty("us.ihmc.stream.XMLInputFactory",
                "us.ihmc.com.sun.xml.stream.ZephyrParserFactory");
        System.setProperty("us.ihmc.stream.XMLOutputFactory",
                "us.ihmc.com.sun.xml.stream.ZephyrWriterFactory");
        System.setProperty("us.ihmc.stream.XMLEventFactory",
                "us.ihmc.com.sun.xml.stream.events.ZephyrEventFactory");

        // Make sure the current thread is using the correct XML processor.
        Thread.currentThread().setContextClassLoader(getClass().getClassLoader());
    }
}
