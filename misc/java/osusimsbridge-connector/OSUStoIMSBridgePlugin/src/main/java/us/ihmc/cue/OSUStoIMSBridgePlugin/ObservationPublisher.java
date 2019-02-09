package us.ihmc.cue.OSUStoIMSBridgePlugin;


import mil.dod.th.core.log.Logging;
import mil.dod.th.core.observation.types.Observation;
import mil.dod.th.core.persistence.ObservationStore;
import mil.dod.th.core.types.MapEntry;
import org.osgi.service.log.LogService;
import us.ihmc.linguafranca.*;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Marshaller;
import java.io.StringWriter;
import java.util.*;
import java.util.concurrent.BlockingQueue;

public class ObservationPublisher extends Thread
{
    public ObservationPublisher (BlockingQueue<UUID> eventQueue) {
        try {
            JAXBContext context = JAXBContext.newInstance(Observation.class);
            _marshaller = context.createMarshaller();
            _marshaller.setProperty(Marshaller.JAXB_ENCODING, "UTF-8");

        } catch (JAXBException e) {
            e.printStackTrace();
        }
        _eventQueue = eventQueue;
        _publisher = new FederationPublisher();
    }

    public void handleObs(Observation obs){
        double lat = -1000;
        double lon = -1000;
        double alt = -6000000;

        Location location = null;

        // If there's coordinates, set the location for the linguafranca
        if (obs.isSetAssetLocation()){
            if(obs.getAssetLocation().isSetLatitude()) {
                lat = obs.getAssetLocation().getLatitude().getValue();
            }
            if (obs.getAssetLocation().isSetLongitude()){
                lon = obs.getAssetLocation().getLongitude().getValue();
            }
            if (obs.getAssetLocation().isSetAltitude()){
                alt = obs.getAssetLocation().getAltitude().getValue();
            }
        }

        // Check if the detection has coordinates
        if (obs.isSetDetection()){
            if (obs.getDetection().isSetTargetLocation()) {
                if (obs.getDetection().getTargetLocation().isSetLatitude()) {
                    lat = obs.getDetection().getTargetLocation().getLatitude().getValue();
                }

                if (obs.getDetection().getTargetLocation().isSetLongitude()) {
                    lon = obs.getDetection().getTargetLocation().getLatitude().getValue();
                }
                if (obs.getDetection().getTargetLocation().isSetAltitude()) {
                    alt = obs.getDetection().getTargetLocation().getLatitude().getValue();
                }
            }
        }

        if (lat != -1 && lon != -1){
            location = new Location(lat, lon, alt, null);
        }

        List<Resource> resources = new ArrayList<>();

        // Create an image resource
        if (obs.isSetDigitalMedia()){
            MimeType mimeType = null;
            if (obs.getDigitalMedia().getEncoding().contains("jpeg") || obs.getDigitalMedia().getEncoding().contains("jpg")) {
                mimeType = MimeType.jpegImage;
            }
            else if (obs.getDigitalMedia().getEncoding().contains("png")){
                mimeType = MimeType.pngImage;
            }

            if (mimeType != null) {
                byte[] imageData = obs.getDigitalMedia().getValue();

                String imageBase64 = Base64.getEncoder().withoutPadding().encodeToString(imageData);

                Resource imageResource = new Resource(mimeType.value(), null, null, null, imageBase64);
                resources.add(imageResource);

                // Remove the digital media from the observation since we're sending it as its own resource. No need
                // to duplicate the data.
                obs.withDigitalMedia(null);
            }
            else {
                Logging.log(LogService.LOG_DEBUG, "ObservationPublisher::Not adding digital media because encoding "
                        + obs.getDigitalMedia().getEncoding() + " is not valid.");
            }
        }

        // Serialize the xml to encode and store into json
        StringWriter writer = new StringWriter();
        try {
            _marshaller.marshal(obs, writer);
        } catch (JAXBException e) {
            e.printStackTrace();
        }

        String obsXMLString = writer.toString();

        String base64json = Base64.getEncoder().withoutPadding().encodeToString(obsXMLString.getBytes());

        UUID obsID = obs.getUuid();

        Resource resource = new Resource(MimeType.jsonBase64.value(), null, null, null, base64json);
        resources.add(resource);

        // If the resources has more than 1, then it is special and should have an ID appended to the objectID.
        // This way it will have it's own icon on the ATAK map.
        String objectID = obs.getAssetName();
        if (resources.size() > 1) {
            Logging.log(LogService.LOG_INFO, getClass().getSimpleName() + "::Additional resource found, appending " +
                    "unique ID");
            objectID += "-" + obsID.toString();
        }

        Message message = new Message(MessageType.osus.toString(), objectID, obsID.toString(), obs.getAssetName(),
                null, null, obs.getCreatedTimestamp(), 360_000L,
                location, resources);

        String lfJSON = LinguaFranca.Factory.toJSON(message);

        Logging.log(LogService.LOG_DEBUG, lfJSON);
        Logging.log(LogService.LOG_INFO, "ObservationPublisher::Attempting to publish observation: " + obs.getUuid());
        _publisher.tryPublish(lfJSON);
    }

    public void setObservationStore(ObservationStore store){
        _obsStore = store;
    }

    public void updatePublishURL(String host, int port){
        _publisher.updatePubURL(host, port);
    }

    public void updateFCID(String cid){
        _publisher.setFederationCIDString(cid);
    }

    @Override
    public void run () {
        Logging.log(LogService.LOG_INFO, "ObservationPublisher: running");

        while (!kill) {
            Observation obs;
            try {
                // Wait for an observation uuid from handleEvent().
                UUID obsUUID = _eventQueue.take();

                // Retrieve the observation from the persistent store.
                obs = _obsStore.find(obsUUID);
                Logging.log(LogService.LOG_DEBUG, "ObservationPublisher: got Observation from " + obs.getAssetName());

                // Handle the observation
                handleObs(obs);
            } catch (InterruptedException e) {
                Logging.log(LogService.LOG_ERROR, "Error parsing json");
            }
        }
    }

    private Marshaller _marshaller = null;
    public boolean kill = false;
    private BlockingQueue<UUID> _eventQueue;
    private ObservationStore _obsStore;

    private FederationPublisher _publisher;
}
