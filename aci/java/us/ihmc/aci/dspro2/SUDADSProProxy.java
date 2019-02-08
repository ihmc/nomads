/*
 * SUDADSProProxy.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import java.util.HashMap;
import java.util.Map;

import org.slf4j.Logger;

import us.ihmc.aci.util.dspro.LogUtils;
import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.aci.util.dspro.soi.ApplicationMetadataFormat;
import us.ihmc.aci.util.dspro.soi.DSProDescription;
import us.ihmc.aci.util.dspro.soi.DSProMimeType;
import us.ihmc.aci.util.dspro.soi.ExchangeFormat;
import us.ihmc.aci.util.dspro.soi.Track;
import us.ihmc.aci.util.dspro.soi.TrackExchangeFormat;
import us.ihmc.aci.util.dspro.soi.TrackUtils;
import us.ihmc.comm.CommException;
import us.ihmc.util.Base64Encoder;

import java.util.Properties;

import java.util.ConcurrentModificationException;


/**
 * DSProProxy version that cancel the received Soi Native Tracks from DSPro
 * and DS data caches and pushes generated Tracks at every setActualPosition().
 *
 * @author Giacomo Benincasa (gbenincasa@ihmc.us), Enrico Casini (ecasini@ihmc.us)
 */
public class SUDADSProProxy extends AbstractDSProProxy
{
    public SUDADSProProxy (DSProProxyInterface proxy)
    {
        super(proxy);
    }

    //I am commenting out just to keep track of the code
    @Override
    public synchronized int init()
    {
        int rc = proxy.init();
        if (rc >= 0) {
            try {
                nodeId = getNodeId();
            }
            catch (CommException ex) {
                LOG.warn (ex.getMessage());
            }
        }
        return rc;
    }

    public String addTrack(String group, Track track, String objectId, String instanceId) throws CommException {
        Map metadata = new HashMap();

        metadata.put(MetadataElement.dataName.name(), track.getName());
        metadata.put(MetadataElement.description.toString(), DSProDescription.track.value());
        metadata.put(MetadataElement.dataFormat.name(), DSProMimeType.soiTrackInfo.value());

        double lat = track.getLatitude();
        double lon = track.getLongitude();
        double padding = 0.00001;
        metadata.put(MetadataElement.leftUpperLatitude.name(), lat + padding);
        metadata.put(MetadataElement.leftUpperLongitude.name(), lon - padding);
        metadata.put(MetadataElement.rightLowerLatitude.name(), lat = padding);
        metadata.put(MetadataElement.rightLowerLongitude.name(), lon + padding);

        metadata.put(MetadataElement.applicationMetadataFormat.name(), ApplicationMetadataFormat.Soigen2_Json_Base64.toString());
        metadata.put(MetadataElement.applicationMetadata.name(),  new Base64Encoder(track.toJson()).processString());

        return addMessage (group, objectId, instanceId, metadata, null, 0);
    }


    @Override
    public synchronized boolean setCurrentPosition (float fLatitude, float fLongitude, float fAltitude,
                                                   String location, String note) throws CommException
    {
        Properties dataProp = TrackUtils.getTrackDataProperties(nodeId, fLatitude, fLongitude);
        dataProp.setProperty(TrackExchangeFormat.RESOURCE_INSTANCE_ID, Long.toString(System.currentTimeMillis()));
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
                messageId = proxy.addMessage("track",
                        dataProp.getProperty(TrackExchangeFormat.RESOURCE_OBJECT_ID),
                        dataProp.getProperty(TrackExchangeFormat.RESOURCE_INSTANCE_ID),
                        metaDataAttributes, metaDataValues,
                        null, (long) 0);
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
        return proxy.setCurrentPosition (fLatitude, fLongitude, fAltitude, location, note);
    }

    //private TrackHandler _trackHandler;
    private String nodeId = "";
    private final static Logger LOG = LogUtils.getLogger (SUDADSProProxy.class);
}
