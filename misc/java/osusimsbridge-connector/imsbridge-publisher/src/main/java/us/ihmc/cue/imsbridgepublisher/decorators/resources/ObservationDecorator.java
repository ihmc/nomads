package us.ihmc.cue.imsbridgepublisher.decorators.resources;


import mil.dod.th.core.observation.types.Observation;
import us.ihmc.cue.imsbridgepublisher.decorators.resources.property.IconPropertyDecorator;
import us.ihmc.linguafranca.Message;
import us.ihmc.linguafranca.MimeType;
import us.ihmc.linguafranca.Property;
import us.ihmc.linguafranca.Resource;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Marshaller;
import java.io.StringWriter;
import java.util.Base64;
import java.util.List;

public class ObservationDecorator extends AbstractResourceDecorator
{
    public ObservationDecorator () {
        _iconPropertyDecorator = new IconPropertyDecorator();
        try {
            JAXBContext context = JAXBContext.newInstance(Observation.class);
            _marshaller = context.createMarshaller();
            _marshaller.setProperty(Marshaller.JAXB_ENCODING, "UTF-8");

        } catch (JAXBException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void setObservation(Observation observation){
        _currentObservation = observation;
        _iconPropertyDecorator.setObservation(observation);
    }


    @Override
    public void generateObject () {
        // Serialize the xml to encode and store into json
        StringWriter writer = new StringWriter();
        try {
            _marshaller.marshal(_currentObservation, writer);
        } catch (JAXBException e) {
            e.printStackTrace();
        }

        String obsXMLString = writer.toString();
        String base64json = Base64.getEncoder().withoutPadding().encodeToString(obsXMLString.getBytes());

        _iconPropertyDecorator.generateObject();

        List<Property> icons = (List<Property>)_iconPropertyDecorator.getObject();

       _object = new Resource(MimeType.jsonBase64.value(), null, null, icons, base64json);
    }

    private IconPropertyDecorator _iconPropertyDecorator;

    private Marshaller _marshaller = null;
}
