package us.ihmc.cue.imsbridgepublisher.decorators;


import java.util.UUID;

public class InstanceIDDecorator extends LFDecorator<UUID>
{
    @Override
    public void generateObject () {
        _object = _currentObservation.getUuid();
    }
}
