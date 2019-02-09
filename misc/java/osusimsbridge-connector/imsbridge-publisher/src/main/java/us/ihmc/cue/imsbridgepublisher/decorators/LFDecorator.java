package us.ihmc.cue.imsbridgepublisher.decorators;


import mil.dod.th.core.observation.types.Observation;
import us.ihmc.linguafranca.Message;

public abstract class LFDecorator<T>
{
    public void setObservation(Observation observation){
        _currentObservation = observation;
    }

    public abstract void generateObject();

    public T getObject(){
        return _object;
    }

    @Override
    public String toString(){
        return getClass().getSimpleName();
    }

    protected Observation _currentObservation;
    protected T _object = null;
}
