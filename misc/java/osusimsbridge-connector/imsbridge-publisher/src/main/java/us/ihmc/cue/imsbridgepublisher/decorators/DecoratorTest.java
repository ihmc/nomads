package us.ihmc.cue.imsbridgepublisher.decorators;


import mil.dod.th.core.log.Logging;
import mil.dod.th.core.observation.types.Observation;
import org.osgi.service.log.LogService;
import us.ihmc.cue.imsbridgepublisher.decorators.resources.ResourceListDecorator;
import us.ihmc.linguafranca.LinguaFranca;
import us.ihmc.linguafranca.Message;
import us.ihmc.linguafranca.MessageType;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Unmarshaller;
import java.io.IOException;
import java.io.StringReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Base64;
import java.util.List;

public class DecoratorTest
{
    public static void main (String[] args) throws JAXBException, IOException {
        _unmarshaller = JAXBContext.newInstance(Observation.class).createUnmarshaller();
        Observation obs = generateObservation();

        DataNameDecorator   dataNameDecorator   = new DataNameDecorator();
        ObjectIDDecorator   objectIDDecorator   = new ObjectIDDecorator();
        InstanceIDDecorator instanceIDDecorator = new InstanceIDDecorator();
        LocationDecorator   locationDecorator   = new LocationDecorator();
        TimestampDecorator  timestampDecorator  = new TimestampDecorator();


        ResourceListDecorator resourceListDecorator = new ResourceListDecorator();

        List<LFDecorator> decorators = new ArrayList<>(Arrays.asList(objectIDDecorator,
                dataNameDecorator,
                instanceIDDecorator,
                locationDecorator,
                timestampDecorator,
                resourceListDecorator));

        for (LFDecorator decorator : decorators){
            decorator.setObservation(obs);
            decorator.generateObject();
        }

        // If the resources has more than 1 or there are properties, then it is special and should have an ID
        // appended to the objectID. This way it will have it's own icon on the ATAK map.
        if (resourceListDecorator.getObject().size() > 1 ||
                (resourceListDecorator.getObject().get(0).getProperties() != null &&
                        resourceListDecorator.getObject().get(0).getProperties().size() > 0)) {
           objectIDDecorator.setSpecial(true);
           objectIDDecorator.generateObject();
        }

        Message message = new Message(MessageType.osus.toString(), objectIDDecorator.getObject(),
                instanceIDDecorator.getObject().toString(), dataNameDecorator.getObject(), null,
                null, obs.getCreatedTimestamp(), 360_000L,
                locationDecorator.getObject(), resourceListDecorator.getObject());

        String val = LinguaFranca.Factory.toJSON(message);

        System.out.println(val);

    }

    private static Observation generateObservation() throws IOException {
        byte[] encoded = Files.readAllBytes(Paths.get("xmlObs.log"));
        String xmlObs  = new String(encoded, Charset.defaultCharset());

        Observation obs = null;

        try {
            obs = (Observation) _unmarshaller.unmarshal(new StringReader(xmlObs));
        } catch (JAXBException e) {
            Logging.log(LogService.LOG_DEBUG, e.getMessage());
        }


        return obs;
    }

    private static String base64String;
    private static Unmarshaller _unmarshaller;

}