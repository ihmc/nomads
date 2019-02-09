package us.ihmc.cue.imsbridgepublisher.decorators;


public class TimestampDecorator extends LFDecorator<Long>
{
    @Override
    public void generateObject () {
        _object = _currentObservation.getCreatedTimestamp();
    }
}
