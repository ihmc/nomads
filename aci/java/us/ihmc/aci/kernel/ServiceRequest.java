package us.ihmc.aci.kernel;

public class ServiceRequest
{
    public static final int REQUEST_TYPE_ACTIVATE      = 1;    // Only applicable to Java VM Container
    public static final int REQUEST_TYPE_TERMINATE     = 2;
    public static final int REQUEST_TYPE_INVOKE        = 3;
    public static final int REQUEST_TYPE_CONNECT       = 4;
    public static final int REQUEST_TYPE_DEACTIVATE    = 5;
    public static final int REQUEST_TYPE_DEPLOY        = 6;
    public static final int REQUEST_TYPE_MIGRATE       = 7;
    public static final int REQUEST_TYPE_CAPTURE_STATE = 8;
    public static final int REQUEST_TYPE_RESTORE_STATE = 9;

    public static final int TRANSPORT_TYPE_MOCKETS     = 10;
    public static final int TRANSPORT_TYPE_SOCKETS     = 11;


    /* ****************************************************************
     * DO NOT RENAME OR DELETE ANY OF THESE FIELDS
     * UNLESS YOU KNOW WHAT YOU ARE DOING.
     * ****************************************************************
     */
    public int transportType;
    public int requestType;
    public ServiceInputStream sis;
    public ServiceOutputStream sos;

    public String methodName;
    public boolean asynchronous;

    public int originalServiceRequestRef;
    /* ************************************************************ */
}
