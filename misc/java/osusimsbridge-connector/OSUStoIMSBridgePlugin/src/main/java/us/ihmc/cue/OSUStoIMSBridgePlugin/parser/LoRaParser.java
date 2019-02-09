package us.ihmc.cue.OSUStoIMSBridgePlugin.parser;


import arl.lora.Report;
import com.fasterxml.jackson.databind.ObjectMapper;
import mil.dod.th.core.log.Logging;
import mil.dod.th.core.observation.types.Detection;
import mil.dod.th.core.observation.types.Observation;
import mil.dod.th.core.observation.types.Weather;
import mil.dod.th.core.types.*;
import mil.dod.th.core.types.detection.AcousticSignature;
import mil.dod.th.core.types.detection.DetectionTypeEnum;
import mil.dod.th.core.types.spatial.Coordinates;
import mil.dod.th.core.types.spatial.LatitudeWgsDegrees;
import mil.dod.th.core.types.spatial.LongitudeWgsDegrees;
import org.osgi.service.log.LogService;
import us.ihmc.linguafranca.Message;
import us.ihmc.linguafranca.MimeType;
import us.ihmc.linguafranca.Resource;

import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.*;

/**
 * @author Blake Ordway (bordway@ihmc.us) on 8/31/2018
 */
public class LoRaParser
{
    public LoRaParser () {
        _loraUUIDs = new HashMap<>();
    }

    public Observation parseMessage (Message msg) {
        Observation obs = null;
        if (msg.getResources() == null) {
            Logging.log(LogService.LOG_ERROR, "Null resources in the LoRa Report message from the imsbridge - Ignoring");
            return null;
        }

        String source = msg.getObjectId();

        UUID sourceUUID = _loraUUIDs.computeIfAbsent(source, s -> UUID.randomUUID());

        for (Resource resource : msg.getResources()) {
            if (!resource.getMimeType().equals(MimeType.jsonBase64.value())) {
                continue;
            }
            if (resource.getData() == null) {
                Logging.log(LogService.LOG_WARNING, "Null data for the LoRa Report resource");
                continue;
            }
            String decoded = new String(Base64.getDecoder().decode(resource.getData()));

            Report report;
            try {
                report = parseJson(decoded, Report.class);
            } catch (IOException e) {
                Logging.log(LogService.LOG_ERROR, "Impossible to parse the Report json resource message - Ignoring resource\n");
                continue;
            }

            if (report.getArl() == null) {
                Logging.log(LogService.LOG_WARNING, "The LoRa report doesn't contain any arl object - Ignoring");
                continue;
            }

            obs = new Observation()
                    .withCreatedTimestamp(System.currentTimeMillis())
                    .withAssetName(source)
                    .withAssetUuid(sourceUUID)
                    .withUuid(UUID.randomUUID())
                    .withAssetType("LoRaGateway")
                    .withVersion(new Version().withMajorNumber(5).withMinorNumber(2));


            // Set location
            if (report.getArl().getGps() != null &&
                    report.getArl().getGps().getLat() != null &&
                    report.getArl().getGps().getLong() != null) {

                LatitudeWgsDegrees lat = new LatitudeWgsDegrees();
                lat.withValue(report.getArl().getGps().getLat());
                LongitudeWgsDegrees lon = new LongitudeWgsDegrees();
                lon.withValue(report.getArl().getGps().getLong());

                obs.withAssetLocation(new Coordinates().withLatitude(lat).withLongitude(lon));
                obs.withModalities(new SensingModality(SensingModalityEnum.GPS, "GPS value"));
                Logging.log(LogService.LOG_DEBUG, "LoRaParser::Setting latitude and longitude");

            }

            if (report.getTime() != null) {
                long millis;
                try {
                    SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
                    Date date = sdf.parse(report.getTime().replace("T", " ").replace("Z", ""));
                    millis = date.getTime();

                    obs.withObservedTimestamp(millis);
                    Logging.log(LogService.LOG_DEBUG, "LoRaParser::Setting created and observed timestamp");
                } catch (ParseException e) {
                    Logging.log(LogService.LOG_ERROR, "Impossible to parse the lora message timestamp");
                }
            }

            // Temp/humidity/pressure
            if (report.getArl().getDht11() != null || report.getArl().getClimate() != null) {
                Weather weather = new Weather();
                if (report.getArl().getDht11() != null) {
                    if (report.getArl().getDht11().getHumidity() != null) {
                        weather.setRelativeHumidity(report.getArl().getDht11().getHumidity());
                        Logging.log(LogService.LOG_DEBUG, "LoRaParser::Setting humidity from Dhtl1");
                    }
                    if (report.getArl().getDht11().getTemp() != null) {
                        weather.setTemperature(new TemperatureCelsius().withValue( convertToCelsius(report.getArl().getDht11().getTemp())));
                        Logging.log(LogService.LOG_DEBUG, "LoRaParser::Setting humidity from Dhtl1");
                    }
                }
                if (report.getArl().getClimate() != null) {
                    if (report.getArl().getClimate().getTemp() != null) {
                        Logging.log(LogService.LOG_DEBUG, "LoRaParser::Setting obs temperature from climate");
                        weather.setTemperature(new TemperatureCelsius().withValue(convertToCelsius(report.getArl().getClimate().getTemp())));
                    }
                    if (report.getArl().getClimate().getHumidity() != null) {
                        Logging.log(LogService.LOG_DEBUG, "LoRaParser::Setting obs humidity from climate");
                        // Convert to decimal
                        weather.setRelativeHumidity(report.getArl().getClimate().getHumidity() / 100);
                    }
                    if (report.getArl().getClimate().getPressure() != null) {
                        Logging.log(LogService.LOG_DEBUG, "LoRaParser::Setting obs pressure from climate");
                        weather.setPressure(new PressureMillibars().withValue(convertToMillibars(report.getArl().getClimate().getPressure())));
                    }
                }
                obs.withWeather(weather).withModalities(
                        new SensingModality(SensingModalityEnum.WEATHER,
                        "Weather report"));
            }

            // Detections
            if (report.getArl().getPir() != null) {
                Logging.log(LogService.LOG_DEBUG, "LoRaParser::Setting PIR detection.");
                Detection detection = new Detection();
                detection.withType(DetectionTypeEnum.ALARM);
                obs.withDetection(detection);
                obs.withModalities(new SensingModality(SensingModalityEnum.PIR,
                        "PIR detection"));
            }

            if (report.getArl().getLightChangeAlert() != null){
                Logging.log(LogService.LOG_DEBUG, "No good observation detection status for the light sensing");
            }

        }

        return obs;
    }

    private double convertToCelsius(double val){
        return 5/9 * (val - 32);
    }

    private double convertToMillibars(double pascalVal){
        return  pascalVal / 100.0;
    }

    private <T> T parseJson (String jsonString, Class<T> valueType) throws IOException {
        if ((jsonString == null) || (valueType == null)) {
            return null;
        }

        ObjectMapper objectMapper = new ObjectMapper();
        return objectMapper.readValue(jsonString, valueType);
    }

    private HashMap<String, UUID> _loraUUIDs;
}
