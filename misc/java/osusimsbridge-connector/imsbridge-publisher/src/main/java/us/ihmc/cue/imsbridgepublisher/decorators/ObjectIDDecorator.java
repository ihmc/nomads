package us.ihmc.cue.imsbridgepublisher.decorators;


public class ObjectIDDecorator extends LFDecorator<String>
{

    @Override
    public void generateObject () {
        if (_special){
            _object = _currentObservation.getAssetName() + "-" + _currentObservation.getUuid();
        }
        else {
            _object = _currentObservation.getAssetName();
        }
    }

    public void setSpecial(boolean val){
        _special = val;
    }

    private boolean _special;
}
