package us.ihmc.cue.imsbridgepublisher.decorators.resources;


import mil.dod.th.core.observation.types.Observation;
import us.ihmc.cue.imsbridgepublisher.decorators.LFDecorator;
import us.ihmc.linguafranca.Resource;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class ResourceListDecorator extends LFDecorator<List<Resource>>
{
    public ResourceListDecorator() {
        _imageDecorator = new ImageDecorator();
        _observationDecorator = new ObservationDecorator();
        _tweetDecorator = new TweetDecorator();

        _object = new ArrayList<>();
    }

    @Override
    public void generateObject () {
        _imageDecorator.generateObject();
        _observationDecorator.generateObject();
        _tweetDecorator.generateObject();

        _object.addAll(Arrays.asList(_imageDecorator.getObject(), _observationDecorator.getObject(), _tweetDecorator.getObject()));
        _object.removeAll(Collections.singleton(null));
    }

    @Override
    public void setObservation(Observation observation){
        _imageDecorator.setObservation(observation);
        _observationDecorator.setObservation(observation);
        _tweetDecorator.setObservation(observation);
    }


    private final ImageDecorator _imageDecorator;
    private final ObservationDecorator _observationDecorator;
    private final TweetDecorator _tweetDecorator;
}
