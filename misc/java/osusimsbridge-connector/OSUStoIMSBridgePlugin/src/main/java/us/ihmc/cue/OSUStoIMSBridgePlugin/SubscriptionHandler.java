package us.ihmc.cue.OSUStoIMSBridgePlugin;

import mil.dod.th.core.log.Logging;
import mil.dod.th.core.observation.types.Observation;
import mil.dod.th.core.persistence.ObservationStore;
import mil.dod.th.core.types.DigitalMedia;
import mil.dod.th.core.validator.ValidationFailedException;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.osgi.service.log.LogService;
import us.ihmc.cue.OSUStoIMSBridgePlugin.parser.ImageParser;
import us.ihmc.cue.OSUStoIMSBridgePlugin.parser.LoRaParser;
import us.ihmc.cue.OSUStoIMSBridgePlugin.parser.TrackInfoParser;
import us.ihmc.linguafranca.*;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Marshaller;
import javax.xml.bind.Unmarshaller;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.Base64;
import java.util.List;
import java.util.UUID;

public class SubscriptionHandler
{
    public SubscriptionHandler () {
        try {
            _context = JAXBContext.newInstance(Observation.class);
            _unmarshaller = _context.createUnmarshaller();
        } catch (JAXBException e) {
            e.printStackTrace();
        }

        String nameMessage = "{\n\"name\": \"OSUStoIMSBridge\"}";
        _subscriber = new FederationSubscriber(nameMessage, subsMessage, this::handleSubscriptionString);

        _trackInfoParser = new TrackInfoParser();
        _imageParser = new ImageParser();
        _loraParser = new LoRaParser();
    }

    public boolean isConnected () {
        if (_subscriber == null) {
            return false;
        }
        return _subscriber.isConnected();
    }

    public void connect (String host, int port) {
        _subscriber.connect(host, port);
    }

    public void closeConnection () {
        _subscriber.endConnection();
    }

    public void handleSubscriptionString (String string) {
        if (!isValidJSON(string)) {
            Logging.log(LogService.LOG_ERROR, "SubscriptionHandler::String received from IMSBridge is not in valid JSON format");
            return;
        }

        if (!isSubscriptionMessage(string)) {
            return;
        }

        Observation obs = createObservationFromString(string);
        if (obs == null) {
            Logging.log(LogService.LOG_DEBUG, "SubscriptionHandler::Observation was null??");
            return;
        }
        obs.withSystemId(_systemID);

        UUID receivedUUID = obs.getUuid();

        _uuidHandler.handleUUID(receivedUUID);

        Logging.log(LogService.LOG_INFO, "SubscriptionHandler::Storing observation with uuid: " + receivedUUID);

        try {
            _obsStore.persist(obs);
        } catch (ValidationFailedException e) {
            e.printStackTrace();
        }

    }

    public void setUUIDHandler (UUIDHandler handler) {
        _uuidHandler = handler;
    }

    public Observation createObservationFromString (String string) {
        Message message = LinguaFranca.Factory.fromJSON(string);

        if (message.getMessageType().equals(MessageType.trackInfo.toString())) {
            Logging.log(LogService.LOG_DEBUG, "SubscriptionHandler::Parsing trackInfo");
            return parseTrackInfo(message);
        }
        else if (message.getMessageType().equals(MessageType.osus.toString())) {
            Logging.log(LogService.LOG_DEBUG, "SubscriptionHandler::Parsing osus message");
            return parseOsusObs(message);
        }
        else if (message.getMessageType().equals(MessageType.image.toString())) {
            Logging.log(LogService.LOG_DEBUG, "SubscriptionHandler::Parsing image message");
            return parseImage(message);
        }
        else if (message.getMessageType().equals(MessageType.loraReport.toString())) {
            Logging.log(LogService.LOG_DEBUG, "SubscriptionHandler::Parsing LoRaReport message");
            return parseLoraReport(message);
        }

        return null;
    }

    public void setControllerID (int id) {
        _systemID = id;
    }

    public void setObservationStore (ObservationStore obsStore) {
        _obsStore = obsStore;
    }

    public void setFCIDHandler (StringHandler handler) {
        _stringHandler = handler;
    }

    private Observation parseLoraReport (Message message) {
        return _loraParser.parseMessage(message);
    }

    private Observation parseTrackInfo (Message message) {
        return _trackInfoParser.parseMessage(message);
    }

    private Observation parseImage (Message message) {
        return _imageParser.parseMessage(message);
    }

    private Observation parseOsusObs (Message message) {
        Observation observation = null;

        List<Resource> resources = message.getResources();
        if (resources != null) {
            Resource obsResource;
            if (resources.size() == 2) {
                obsResource = resources.get(1);
                if (!obsResource.getMimeType().equals(MimeType.jsonBase64.value())) {
                    Logging.log(LogService.LOG_DEBUG, "SubscriptionHandler::The second resource was not the observation, using the first");
                    obsResource = resources.get(0);
                }
            }
            else {
                obsResource = resources.get(0);
            }
            String base64String = obsResource.getData();

            // Create observation
            if (base64String != null) {
                String xmlObs = new String(Base64.getDecoder().decode(base64String));
                try {
                    observation = (Observation) _unmarshaller.unmarshal(new StringReader(xmlObs));
                } catch (JAXBException e) {
                    Logging.log(LogService.LOG_DEBUG, e.getMessage());
                }
            }

            // Add image to observation if there's a resource for the image
            if (observation != null && resources.size() == 2) {
                Resource imageResource = resources.get(0);
                if (!imageResource.getMimeType().equals(MimeType.jpegImage.value())) {
                    Logging.log(LogService.LOG_DEBUG, "SubscriptionHandler::The first resource was not the image, using the second");

                    imageResource = resources.get(1);
                }

                String imageString = imageResource.getData();
                if (imageString != null) {
                    byte[] decodedImage = Base64.getDecoder().decode(imageString);
                    observation.withDigitalMedia(new DigitalMedia(decodedImage, "image/jpeg"));
                }
            }
            //printXMLObs(observation);
        }

        return observation;
    }

    private void printXMLObs (Observation obs) {
        try {
            JAXBContext context = JAXBContext.newInstance(Observation.class);
            Marshaller marshaller = context.createMarshaller();
            marshaller.setProperty(Marshaller.JAXB_ENCODING, "UTF-8");
            StringWriter writer = new StringWriter();

            marshaller.marshal(obs, writer);
            String obsXMLString = writer.toString();
            Logging.log(LogService.LOG_INFO, obsXMLString);
        } catch (JAXBException e) {
            e.printStackTrace();
        }
    }

    private boolean isSubscriptionMessage (String jsonString) {
        return !tryParseClientID(jsonString) && !tryParsePing(jsonString);
    }

    /**
     * Try to parse the client ID from the json
     *
     * @param message The json string
     * @return Whether or not this was a client ID message
     */
    private boolean tryParseClientID (String message) {
        JSONObject object;
        try {
            object = new JSONObject(message);
        } catch (JSONException e) {
            Logging.log(LogService.LOG_ERROR, "SubscriptionHandler::String received from Federation is not in valid JSON format");
            return false;
        }

        try {
            _stringHandler.handleString(object.getString("clientId"));
        } catch (JSONException exception) {
            // This wasn't a clientId json
            return false;
        }
        return true;
    }

    /**
     * Check if this is a ping message (federation occasionally sends a 'ping' message to check if the connection is
     * still open)
     *
     * @param message The json string
     * @return Whether or not the json string was a ping type
     */
    private boolean tryParsePing (String message) {
        JSONObject object;
        try {
            object = new JSONObject(message);
        } catch (JSONException e) {
            Logging.log(LogService.LOG_ERROR, "SubscriptionHandler::String received from Federation is not in valid JSON format");
            return false;
        }

        try {
            return object.getString("messageType").equals("ping");
        } catch (JSONException exception) {
            return false;
        }
    }

    private boolean isValidJSON (String unknownTypeString) {
        try {
            new JSONObject(unknownTypeString);
        } catch (JSONException ex) {
            try {
                new JSONArray(unknownTypeString);
            } catch (JSONException ex1) {
                return false;
            }
        }
        return true;
    }

    private static final String subsMessage = "{\n" +
            " \"add\": [\"trackInfo\", \"image\", \"loraReport\", \"osus\"]\n" +
            "}";

    private JAXBContext _context = null;
    private LoRaParser _loraParser;
    private ObservationStore _obsStore;
    private int _systemID;
    private Unmarshaller _unmarshaller = null;
    private FederationSubscriber _subscriber;
    private StringHandler _stringHandler;
    private UUIDHandler _uuidHandler;
    private TrackInfoParser _trackInfoParser;
    private ImageParser _imageParser;
}
