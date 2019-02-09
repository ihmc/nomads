package us.ihmc.cue.OSUStoIMSBridgePlugin.parser;

import mil.dod.th.core.log.Logging;
import mil.dod.th.core.observation.types.ImageMetadata;
import mil.dod.th.core.observation.types.Observation;
import mil.dod.th.core.types.DigitalMedia;
import mil.dod.th.core.types.Version;
import mil.dod.th.core.types.image.*;
import mil.dod.th.core.types.spatial.Coordinates;
import mil.dod.th.core.types.spatial.HaeMeters;
import mil.dod.th.core.types.spatial.LatitudeWgsDegrees;
import mil.dod.th.core.types.spatial.LongitudeWgsDegrees;
import org.osgi.service.log.LogService;
import us.ihmc.linguafranca.Message;
import us.ihmc.linguafranca.MimeType;
import us.ihmc.linguafranca.Resource;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Base64;
import java.util.List;
import java.util.UUID;

/**
 * @author Blake Ordway (bordway@ihmc.us) on 8/31/2018
 */
public class ImageParser
{
    public ImageParser(){

    }

    @SuppressWarnings("ConstantConditions")
    public Observation parseMessage(Message message){
        Observation observation = null;
        List<Resource> resourceList = message.getResources();
        if (resourceList != null){
            if (resourceList.size() == 0){
                return null;
            }
            observation = new Observation();
            observation.withAssetName(message.getDataName())
                    .withUuid(UUID.randomUUID())
                    .withAssetType("camera")
                    // The LF message for images from ATAK do not include the name of the device, just the name of the image
                    .withAssetUuid(UUID.randomUUID())
                    .withCreatedTimestamp(message.getSourceTimestamp())
                    .withVersion(new Version().withMajorNumber(5).withMinorNumber(2));


            if (message.getLocation() != null) {
                Coordinates coordinates = new Coordinates();

                if (message.getLocation().getAltitude() != null) {
                    HaeMeters altitude = new HaeMeters();
                    altitude.withValue((double) message.getLocation().getAltitude());
                    coordinates.setAltitude(altitude);
                }

                LatitudeWgsDegrees lat = new LatitudeWgsDegrees();
                lat.withValue(message.getLocation().getLatitude());
                coordinates.setLatitude(lat);
                LongitudeWgsDegrees lon = new LongitudeWgsDegrees();
                lon.withValue(message.getLocation().getLongitude());
                coordinates.setLongitude(lon);
                observation.setAssetLocation(coordinates);
            }

            // Create the image!
            Resource imageResource = resourceList.get(0);
            String expectedType = MimeType.jpegImage.value() + ".base64";
            if (imageResource.getMimeType().equals(expectedType)){
                String imageString = imageResource.getData();
                if (imageString != null) {
                    byte[] decodedImage = Base64.getDecoder().decode(imageString);
                    InputStream in = new ByteArrayInputStream(decodedImage);
                    int width = 0;
                    int height = 0;
                    try {
                        BufferedImage bImageFromConvert = ImageIO.read(in);
                        width = bImageFromConvert.getWidth();
                        height = bImageFromConvert.getHeight();
                    } catch (IOException e) {
                       Logging.log(LogService.LOG_DEBUG, "ImageParser::Failed to create image from byte[]");
                    }

                    observation.withDigitalMedia(new DigitalMedia(decodedImage, "image/jpeg"));
                    observation.withImageMetadata(new ImageMetadata()
                            .withCaptureTime(message.getSourceTimestamp())
                            .withResolution(new PixelResolution(width, height))
                            .withImager(new Camera().withDescription("Tablet camera").withType(CameraTypeEnum.OTHER))
                            .withImageCaptureReason(new ImageCaptureReason(ImageCaptureReasonEnum.MANUAL, "Manual image")));
                }
            }
            else{
                Logging.log(LogService.LOG_ERROR, "ImageParser::This mime type is not " + expectedType + " image! Its type is " + imageResource.getMimeType());
            }
        }
        return observation;
    }
}
