package netlogger.model.messages;

import netlogger.controller.NATSTopicsManager;
import netlogger.controller.SubjectCounter;
import netlogger.util.StylingManager;
import netlogger.controller.elasticsearch.ElasticsearchController;
import netlogger.controller.tracking.TrackingController;
import netlogger.model.*;
import netlogger.model.tracking.TrackingMeasure;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import measure.proto.LogTracking;
import measure.proto.Subject;

import java.util.function.Consumer;

/**
 * Controls what should happen to a Measure that's received
 */
public class MeasureHandler
{
    public MeasureHandler () {
    }

    public void setMeasureFilter (MeasureFilter measureFilter) {
        _measureFilter = measureFilter;
    }

    public void setStylingManager (StylingManager stylingManager) {
        _stylingManager = stylingManager;
    }

    public void setSubjectCounter(SubjectCounter counter){
        _subjectCounter = counter;
    }


    /**
     * Set the consumer where all incoming measures should be put
     * @param consumer Consumer for the measures
     */
    public void setMeasureConsumer (Consumer<StyledString> consumer) {
        _measureConsumer = consumer;
    }

    public void setTopicsManager (NATSTopicsManager topicsManager) {
        _topicsManager = topicsManager;
    }

    public void setESController (ElasticsearchController controller) {
        _elasticsearchManager = controller;
    }

    public void setTrackingController (TrackingController trackingController) {
        _trackingController = trackingController;
    }


    public void handleNewMeasure (MeasureIn measure) {
        _subjectCounter.subjectReceived(measure.getMeasure().getSubject().toString());

        if (measure.getMeasure().getSubject().equals(Subject.data_tracking)) {
            handleTrackingMeasure(measure);
        }

        if (_elasticsearchManager != null) {
            _elasticsearchManager.offerMeasure(measure);
        }


        StringMeasure stringMeasure = StringMeasureFactory.createStringMeasure(measure.getMeasure(), measure.getNatsTopic());
        StyledString styledString = convertMeasure(stringMeasure);
        _topicsManager.handleTopic(measure.getNatsTopic());
        if (checkMeasure(stringMeasure)) {
            _measureConsumer.accept(styledString);
        }

    }

    private void handleTrackingMeasure (MeasureIn measure) {
        TrackingMeasure trackingMeasure = new TrackingMeasure(measure.getMeasure().getStringsMap().get(LogTracking.Str.src_application_name.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.source_module.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.dest_application_name.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.dest_module.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.prev_data_id.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.curr_data_id.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.checksum.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.type.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.source_client_id.toString()),
                measure.getMeasure().getStringsMap().get(LogTracking.Str.dest_client_id.toString()),
                measure.getMeasure().getTimestamp());

        if (_trackingController != null) {
            _trackingController.handleDataTrackingMeasure(trackingMeasure);
        }
        else{
            _logger.error("Tracking controller is null?");
        }
    }


    private boolean checkMeasure (StringMeasure measure) {
        return _measureFilter.search(measure) && _measureFilter.checkTopic(measure);
    }


    private StyledString convertMeasure (StringMeasure stringMeasure) {
        StyledString styledMeasure = new StyledString(stringMeasure);
        _stylingManager.setValues(styledMeasure);

        return styledMeasure;
    }


    private NATSTopicsManager _topicsManager;
    private Consumer<StyledString> _measureConsumer;
    private ElasticsearchController _elasticsearchManager;
    private TrackingController _trackingController;
    private MeasureFilter _measureFilter;
    private StylingManager _stylingManager;

    private SubjectCounter _subjectCounter;

    private static final Logger _logger = LoggerFactory.getLogger(MeasureHandler.class);

}