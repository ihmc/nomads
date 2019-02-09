package us.ihmc.cue.imsbridgepublisher.decorators.resources;

import mil.dod.th.core.log.Logging;
import org.osgi.service.log.LogService;
import us.ihmc.cue.imsbridgepublisher.decorators.LFDecorator;
import us.ihmc.linguafranca.MimeType;
import us.ihmc.linguafranca.Resource;

import java.util.Base64;

public class ImageDecorator extends AbstractResourceDecorator
{
    @Override
    public void generateObject () {
        // Create an image resource
        if (_currentObservation.isSetDigitalMedia()){
            MimeType mimeType = null;
            if (_currentObservation.getDigitalMedia().getEncoding().contains("jpeg") || _currentObservation.getDigitalMedia().getEncoding().contains("jpg")) {
                mimeType = MimeType.jpegImage;
            }
            else if (_currentObservation.getDigitalMedia().getEncoding().contains("png")){
                mimeType = MimeType.pngImage;
            }

            if (mimeType != null) {
                byte[] imageData = _currentObservation.getDigitalMedia().getValue();

                String imageBase64 = Base64.getEncoder().withoutPadding().encodeToString(imageData);

                _object = new Resource(mimeType.value(), null, null, null, imageBase64);

                // Remove the digital media from the observation since we're sending it as its own resource. No need
                // to duplicate the data.
                _currentObservation.withDigitalMedia(null);
            }
            else {
                Logging.log(LogService.LOG_DEBUG, "ObservationPublisher::Not adding digital media because encoding "
                        + _currentObservation.getDigitalMedia().getEncoding() + " is not valid.");
            }
        }
    }

}
