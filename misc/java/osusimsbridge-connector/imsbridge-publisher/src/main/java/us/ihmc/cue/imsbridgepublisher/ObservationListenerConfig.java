package us.ihmc.cue.imsbridgepublisher;
import aQute.bnd.annotation.metatype.Meta.AD;
import aQute.bnd.annotation.metatype.Meta.OCD;

@OCD(name = "ObservationListener") // <- tells BND this interface provides ConfigurationAdmin data
public interface ObservationListenerConfig
{
	@AD(required=false, deflt = "127.0.0.1", description = "IP of the Federation instance")	// <- tells BND this is a configuration attribute definition, and provides a default value
	String host();	
	@AD(required=false, deflt = "4567", description = "Port of the Federation instance")	// <- tells BND this is a configuration attribute definition, and provides a default value
	int port();
	@AD(required=false, deflt = "true")	// <- tells BND this is a configuration attribute definition, and provides a default value
	boolean Run();
}
