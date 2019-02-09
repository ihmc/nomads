package us.ihmc.aci.kernel;

public class JavaServiceRequest extends ServiceRequest implements Runnable
{
    public JavaServiceRequest ()
    {
        this (null);
    }

    public JavaServiceRequest (JavaServiceManager jsm)
    {
        _jsm = jsm;
    }

    public void run()
    {
        System.out.println("JavaServiceRequest ::::: run()");

        if (_jsm != null) {
            _jsm.processRequest (this);
        }
    }

    private JavaServiceManager _jsm = null;

    String classPathForService;  // Used when activating a new service
    String classNameForService;  // Used when activating a new service
    String UUID;

    String requestorNodeUUID;	// Node initiating the service request

    // Fields used by the capture/restore support of JikesRVM
    byte[] stateBuffer;
    boolean isTryAgain;
    long timeOut;
}
