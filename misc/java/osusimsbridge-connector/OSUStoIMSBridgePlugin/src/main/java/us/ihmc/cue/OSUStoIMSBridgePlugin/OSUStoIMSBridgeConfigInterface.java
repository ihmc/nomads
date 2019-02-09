package us.ihmc.cue.OSUStoIMSBridgePlugin;
import aQute.bnd.annotation.metatype.Meta.AD;
import aQute.bnd.annotation.metatype.Meta.OCD;

@OCD(name = "OSUStoIMSBridge plug-in") // <- tells BND this interface provides ConfigurationAdmin data
public interface OSUStoIMSBridgeConfigInterface
{
	@AD(required=false, deflt = "192.168.104.22", description = "IP of the Federation instance")	// <- tells BND this is a configuration attribute definition, and provides a default value
	String host();	
	@AD(required=false, deflt = "4567", description = "Port of the Federation instance")	// <- tells BND this is a configuration attribute definition, and provides a default value
	int port();
	@AD(required=false, deflt = "true")	// <- tells BND this is a configuration attribute definition, and provides a default value
	boolean Run();
}
