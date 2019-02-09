package us.ihmc.cue.imsbridgepublisher.decorators;


public class DataNameDecorator extends LFDecorator<String>
{

    @Override
    public void generateObject () {
        _object = _currentObservation.getAssetName();
    }
}
