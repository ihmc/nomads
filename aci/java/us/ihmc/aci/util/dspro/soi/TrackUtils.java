package us.ihmc.aci.util.dspro.soi;

import java.util.HashMap;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicLong;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.aci.disServiceProProxy.util.PointToBoundingBox;
import us.ihmc.util.SortedProperties;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class TrackUtils
{
    private static AtomicLong _sequenceId = new AtomicLong (System.currentTimeMillis());

    public static Properties addTrackPropertiesToTrackData (Properties dataProp, String trackAction)
    {
        //add sequence id
        dataProp.setProperty(TrackExchangeFormat.DSPRO_SEQUENCE_ID, String.valueOf (_sequenceId.getAndSet(System.currentTimeMillis())));

        //add action
        dataProp.setProperty(TrackExchangeFormat.DSPRO_ACTION, trackAction);

        return dataProp;
    }

    private static String getSymbol (String nodeId)
    {
        if (nodeId != null && nodeId.toUpperCase().contains("UAV")) {
            return TrackExchangeFormat.DSPRO_AIR_2525_SYMBOL_ID;
        }
        return TrackExchangeFormat.DSPRO_GROUND_2525_SYMBOL_ID;
    }

    public static boolean isSoiMetadata (HashMap<String, Object> properties)
    {
        String dataFormat = (String) properties.get(MetadataElement.Data_Format.toString());
        if (dataFormat == null) {
            return false;
        }
        return DSProMimeType.track.equals (DSProMimeType.getMatchIgnoreCase (dataFormat));
//        return TrackExchangeFormat.DSPRO_TRACK_MIME_TYPE.equalsIgnoreCase(dataFormat);
    }

    public static Properties getTrackDataProperties (String nodeId, float fLatitude, float fLongitude)
    {
        Properties data = new SortedProperties();

        Logger.getLogger(TrackUtils.class.getName()).log(Level.INFO,
                "Position updated with latitude {0}, longitude {1} from {2}",
                new Object[]{fLatitude, fLongitude, nodeId});

        //fill data
        data.setProperty(TrackExchangeFormat.RESOURCE_OBJECT_ID, nodeId);
        data.setProperty(TrackExchangeFormat.TRACK_NAME, nodeId);
        data.setProperty(TrackExchangeFormat.TRACK_MIL_STD_2525_SYMBOL_ID, getSymbol (nodeId)); //standard

        // rectangle
        data.setProperty(TrackExchangeFormat.EVENT_LATITUDE, String.valueOf(fLatitude));
        data.setProperty(TrackExchangeFormat.EVENT_LONGITUDE, String.valueOf(fLongitude));
        data.setProperty(TrackExchangeFormat.EVENT_DGT, String.valueOf (System.currentTimeMillis()));

        return data;
    }

    public static Properties getTrackMetadataProperties (Properties dataProp, String trackAction)
    {
        return getTrackMetadataProperties (trackAction,
                                           dataProp.getProperty (TrackExchangeFormat.TRACK_NAME),
                                           Double.parseDouble (dataProp.getProperty(TrackExchangeFormat.EVENT_LATITUDE)),
                                           Double.parseDouble (dataProp.getProperty(TrackExchangeFormat.EVENT_LONGITUDE)));
        
    }

    public static Properties getTrackMetadataProperties (String trackAction, String trackName,
                                                         double lat, double lon)
    {
        PointToBoundingBox bbox = new PointToBoundingBox (lat, lon);
        return getTrackMetadataProperties (trackAction, trackName,
                                           bbox.getLefUpperLatitude(),
                                           bbox.getLefUpperLongitude(),
                                           bbox.getRightLowerLatitude(),
                                           bbox.getRightLowerLongitude());
    }

    public static Properties getTrackMetadataProperties (String trackAction, String trackName,
                                                         double leftUpperLat, double leftUpperLong,
                                                         double rightLowerLat, double rightLowerLong)
    {
        Properties metaDataProp = new Properties();

        //content, description and format fields
        metaDataProp.put(MetadataElement.Data_Content.toString(), "Soi-" +
                         DSProDescription.track.value() + "-" +
                         trackAction + "-" + trackName);
        metaDataProp.put(MetadataElement.Description.toString(), DSProDescription.track.value());
//        metaDataProp.put(MetadataElement.Data_Format.toString(), TrackExchangeFormat.DSPRO_TRACK_MIME_TYPE);
        metaDataProp.put(MetadataElement.Data_Format.toString(), DSProMimeType.trackInfo.value());

        if (trackAction.equals(TrackExchangeFormat.Action.Delete.toString())) {
            //use the whole world as a bounding box
            metaDataProp.put(MetadataElement.Left_Upper_Latitude.toString(), TrackExchangeFormat.DSPRO_MAX_LATITUDE);
            metaDataProp.put(MetadataElement.Right_Lower_Latitude.toString(), TrackExchangeFormat.DSPRO_MIN_LATITUDE);
            metaDataProp.put(MetadataElement.Left_Upper_Longitude.toString(), TrackExchangeFormat.DSPRO_MIN_LONGITUDE);
            metaDataProp.put(MetadataElement.Right_Lower_Longitude.toString(), TrackExchangeFormat.DSPRO_MAX_LONGITUDE);
        }
        else {            
            //calculate bounding box
            double epsilon = 0.00001;
            if (Math.abs (leftUpperLat - rightLowerLat) < epsilon) {
                leftUpperLat += epsilon;
                rightLowerLat -= epsilon;
            }
            if (Math.abs (leftUpperLong - rightLowerLong) < epsilon) {
                leftUpperLong -= epsilon;
                rightLowerLong += epsilon;
            }
            metaDataProp.put(MetadataElement.Left_Upper_Latitude.toString(), (float) leftUpperLat);
            metaDataProp.put(MetadataElement.Right_Lower_Latitude.toString(), (float) rightLowerLat);
            metaDataProp.put(MetadataElement.Left_Upper_Longitude.toString(), (float) leftUpperLong);
            metaDataProp.put(MetadataElement.Right_Lower_Longitude.toString(), (float) rightLowerLong);
        }

        return metaDataProp;
    }
}
