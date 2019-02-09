package us.ihmc.cue.imsbridgepublisher.decorators;


import us.ihmc.linguafranca.Location;

public class LocationDecorator extends LFDecorator<Location>
{
    @Override
    public void generateObject () {
        double lat = -1000;
        double lon = -1000;
        double alt = -6000000;
        
        // If there's coordinates, set the location for the linguafranca
        if (_currentObservation.isSetAssetLocation()){
            if(_currentObservation.getAssetLocation().isSetLatitude()) {
                lat = _currentObservation.getAssetLocation().getLatitude().getValue();
            }
            if (_currentObservation.getAssetLocation().isSetLongitude()){
                lon = _currentObservation.getAssetLocation().getLongitude().getValue();
            }
            if (_currentObservation.getAssetLocation().isSetAltitude()){
                alt = _currentObservation.getAssetLocation().getAltitude().getValue();
            }
        }

        // Check if the detection has coordinates
        if (_currentObservation.isSetDetection()){
            if (_currentObservation.getDetection().isSetTargetLocation()) {
                if (_currentObservation.getDetection().getTargetLocation().isSetLatitude()) {
                    lat = _currentObservation.getDetection().getTargetLocation().getLatitude().getValue();
                }
                if (_currentObservation.getDetection().getTargetLocation().isSetLongitude()) {
                    lon = _currentObservation.getDetection().getTargetLocation().getLongitude().getValue();
                }
                if (_currentObservation.getDetection().getTargetLocation().isSetAltitude()) {
                    alt = _currentObservation.getDetection().getTargetLocation().getAltitude().getValue();
                }
            }
        }

       _object = new Location(lat, lon, alt, null);
    }
}
