package us.ihmc.cue.OSUStoIMSBridgePlugin.parser;


import mil.dod.th.core.log.Logging;
import mil.dod.th.core.observation.types.Detection;
import mil.dod.th.core.observation.types.Observation;
import mil.dod.th.core.types.SensingModality;
import mil.dod.th.core.types.SensingModalityEnum;
import mil.dod.th.core.types.Version;
import mil.dod.th.core.types.spatial.Coordinates;
import mil.dod.th.core.types.spatial.HaeMeters;
import mil.dod.th.core.types.spatial.LatitudeWgsDegrees;
import mil.dod.th.core.types.spatial.LongitudeWgsDegrees;
import org.osgi.service.log.LogService;
import us.ihmc.linguafranca.Message;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

/**
 * @author Blake Ordway (bordway@ihmc.us) on 8/30/2018
 */
public class TrackInfoParser
{
    public TrackInfoParser(){
        _assetUUIDs = new HashMap<>();
    }

    public Observation parseMessage(Message message){
        Observation observation = null;

        if (message.getLocation() != null) {
            UUID assetUUID = _assetUUIDs.computeIfAbsent(message.getObjectId(), s -> UUID.randomUUID());

            observation = new Observation()
                    .withAssetName(message.getDataName())
                    .withUuid(UUID.randomUUID())
                    .withAssetType("handheld")
                    .withVersion(new Version().withMajorNumber(5).withMinorNumber(2))
                    .withCreatedTimestamp(message.getSourceTimestamp())
                    .withAssetUuid(assetUUID)
                    .withModalities(new SensingModality(SensingModalityEnum.GPS, "Handheld GPS position"));


            Coordinates coordinates = new Coordinates();

            if (message.getLocation().getAltitude() != null) {
                HaeMeters altitude = new HaeMeters();
                altitude.withValue(message.getLocation().getAltitude());
                coordinates.withAltitude(altitude);
            }

            LatitudeWgsDegrees lat = new LatitudeWgsDegrees();
            lat.withValue(message.getLocation().getLatitude());
            coordinates.withLatitude(lat);
            LongitudeWgsDegrees lon = new LongitudeWgsDegrees();
            lon.withValue(message.getLocation().getLongitude());
            coordinates.withLongitude(lon);
            observation.withAssetLocation(coordinates);

        }
        else{
            Logging.log(LogService.LOG_ERROR, "TrackInfoParser::Received track info with null location?");
        }

        return observation;
    }


    private Map<String, UUID> _assetUUIDs;
}
