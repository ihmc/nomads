package us.ihmc.cue.imsbridgepublisher.decorators.resources.property;


import mil.dod.th.core.log.Logging;
import mil.dod.th.core.observation.types.TargetClassification;
import org.osgi.service.log.LogService;
import us.ihmc.cue.imsbridgepublisher.decorators.LFDecorator;
import us.ihmc.linguafranca.MutabilityType;
import us.ihmc.linguafranca.Property;

import java.util.ArrayList;
import java.util.List;

public class IconPropertyDecorator extends LFDecorator
{

    @Override
    public void generateObject () {
        List<Property> icons = null;

        if (_currentObservation.isSetDetection()) {
            if (_currentObservation.getDetection().isSetTargetClassifications()) {
                List<TargetClassification> targetClassifications = _currentObservation.getDetection().getTargetClassifications();

                for (TargetClassification classification : targetClassifications) {
                    if (classification == null) {
                        Logging.log(LogService.LOG_ERROR, "Target classification is null");
                    }

                    else {
                        if (icons == null) {
                            icons = new ArrayList<>();
                        }
                        icons.add(new Property("icon", "icon to show on ATAK",
                                MutabilityType.ReadOnly.toString(), classification.getType().getValue().value()));

                        Logging.log(LogService.LOG_DEBUG, "Adding classification of type: " +
                                classification.getType().getValue().value());
                    }
                }
            }
        }

        _object = icons;
    }
}
