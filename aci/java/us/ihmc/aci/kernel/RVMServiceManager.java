package us.ihmc.aci.kernel;


import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import us.ihmc.aci.grpMgrOld.GroupManagerProxy;
import us.ihmc.aci.service.Service;
import us.ihmc.algos.Statistics;
import com.ibm.JikesRVM.*;

public class RVMServiceManager extends ServiceManager
{
    private static final boolean DEBUG = true;
    static final int NUM_WORKER_THREADS = 10;
    static final boolean BENCHMARK = false;

    /**
     * The single instance of this class
     */
    static RVMServiceManager _instance;

    /**
     * Global table of all instatiated services in this VM
     */
    Hashtable _servicesHT;

    /**
     * Array of worker threads in charge of handling service requests coming from the ACI Kernel
     */
    RVMServiceRequestHandler[] _jsmThreads;

    /**
     * The single event handler in this VM
     */
    RVMServiceEventHandler _eventHandler;

    /**
     * The unique statistics object to collect times
     */
    ServiceManagerStatistics _statsObject;

    /**
     *
     */
    GroupManagerProxyRVMImpl _gmp;

    /**
     * This inner class is used by all the worker threads to store their
     * statistics. Being accessed by multiple threads, its fields can only
     * be accessed through synchronized methods.
     *
     * @author raffaele
     */
    public class ServiceManagerStatistics
    {
        public static final int PARSING = 0;
        public static final int INVOKE = 1;
        public static final int ENCODE = 2;
        public static final int FIND_METHOD = 3;
        public static final int CAPTURE = 4;
        public static final int RESTORE = 5;

        // ---- used for benchmarking stuff ----
        private boolean _firstInvocation;
        private Statistics _parsingStats, _invokeStats, _encodeStats, _findMethodStats, _captureStats, _restoreStats;
        // -------------------------------------

        ServiceManagerStatistics()
        {
            _firstInvocation = true;
            if (BENCHMARK) {
                _parsingStats = new Statistics();
                _invokeStats = new Statistics();
                _encodeStats = new Statistics();
                _findMethodStats = new Statistics();
                _captureStats = new Statistics();
                _restoreStats = new Statistics();
            }
        }

        public synchronized void updateStats(int whichStat, double value)
        {
            switch (whichStat) {
                case CAPTURE:
                    _captureStats.update (value);
                    break;
                case RESTORE:
                    _restoreStats.update(value);
                        break;
                case PARSING:
                    _parsingStats.update(value);
                    break;
                case INVOKE:
                    _invokeStats.update(value);
                    break;
                case ENCODE:
                    _encodeStats.update(value);
                    break;
                case FIND_METHOD:
                    _findMethodStats.update(value);
                    break;
            }
        }

        public synchronized double getAverage(int whichStat)
        {
            switch(whichStat) {
                case CAPTURE:
                    return _captureStats.getAverage();
                case RESTORE:
                    return _restoreStats.getAverage();
                case PARSING:
                    return _parsingStats.getAverage();
                case INVOKE:
                    return _invokeStats.getAverage();
                case ENCODE:
                    return _encodeStats.getAverage();
                case FIND_METHOD:
                    return _findMethodStats.getAverage();
                default:
                    return -1;
            }
        }

        public synchronized double getStDev(int whichStat) {
            switch(whichStat) {
                case CAPTURE:
                    return _captureStats.getStDev();
                case RESTORE:
                    return _restoreStats.getStDev();
                case PARSING:
                    return _parsingStats.getStDev();
                case INVOKE:
                    return _invokeStats.getStDev();
                case ENCODE:
                    return _encodeStats.getStDev();
                case FIND_METHOD:
                    return _findMethodStats.getStDev();
                default:
                    return -1;
            }
        }

        public synchronized boolean getFirstInvocation()
        {
            return _firstInvocation;
        }

        public synchronized void setFirstInvocation(boolean value)
        {
            _firstInvocation = value;
        }
    }

    void run() {

        // TODO: possible global try/catch

        /* 1. Create the two readers so that the init() method in the kernel can return successfully
         * as soon as possible.
         * N.B. the RVMServiceRequestReader is a singleton class, with a private constructor. */
        RVMServiceRequestReader requestReader = RVMServiceRequestReader.getInstance(0);

        RVMServiceEventReader eventReader = new RVMServiceEventReader(3);

        _gmp = new GroupManagerProxyRVMImpl(4);

        /* 1.a Create the ServiceManagerStatistics object if benchmark is enabled */
        if(BENCHMARK)
            _statsObject = new ServiceManagerStatistics();
        else
            _statsObject = null;

        /* 2. Instantiate the array of worker threads, which will concurrently extract ServiceRequests
         * posted by the ACI Kernel */
        _jsmThreads = new RVMServiceRequestHandler[NUM_WORKER_THREADS];
        for(int i=0; i<NUM_WORKER_THREADS; i++) {
            _jsmThreads[i] = new RVMServiceRequestHandler(requestReader, _instance);
            _jsmThreads[i].start();
        }

        /* 3. Create the single event handler thread, which is in charge of extracting ServiceEvents and
         * taking actions in response to them */
        _eventHandler = new RVMServiceEventHandler(eventReader, _gmp);
        _eventHandler.start();

        /* 4. Create the global hash table of instantiated services */
        _servicesHT = new Hashtable();

        log ("main(): RVMServiceManager correctly initialized. ");
    }

    static RVMServiceManager getInstance()
    {
        if (_instance == null) {
            _instance = new RVMServiceManager();
        }
        return _instance;
    }

    public static void main(String[] args) {

        // First of all, configure the Mobile JikesRVM environment as ACIK
        try {
            MobileJikesRVM.configureJikesRVM(MobileJikesRVM.ACIK);
        } catch (UnsupportedJikesRVMEnvironment e) {
            e.printStackTrace();
        }

        _instance = getInstance();
        _instance.run();
    }

    private static void log(String msg) {
        if (DEBUG) {
            System.out.println("[RVMServiceManager] " + msg);
        }
    }

    public void deregisterServiceAttribute(Service service, String attrKey, String attrVal) {
        String serviceUUID = this.getServiceInstanceUUID(service);
        if (serviceUUID == null) {
            log("deregisterServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                    + service.getClass().toString());
            return;
        }

        _gmp.deregisterServiceAttribute(serviceUUID, attrKey, attrVal);
    }

    public GroupManagerProxy getGroupManagerProxy() {
        return     _gmp;
    }

    public String getServiceInstanceUUID(Object serviceInstance) {

        Enumeration en = _servicesHT.keys();
        while (en.hasMoreElements()) {
            String uuid = (String) en.nextElement();
            if (_servicesHT.get(uuid) == serviceInstance) {
                return uuid;
            }
        }

        return null;
    }

    public boolean hasServiceAttribute(Service service, String attrKey, String attrVal) {

        String serviceUUID = this.getServiceInstanceUUID(service);
        if (serviceUUID == null) {
            log("hasServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                    + service.getClass().toString());
            return false;
        }

        return _gmp.hasServiceAttribute(serviceUUID, attrKey, attrVal);
    }

    public Vector getNetworkTopology() {
        throw new UnsupportedOperationException("not implemented");
    }

    public void registerServiceAttribute(Service service, String attrKey, String attrVal) {
        String serviceUUID = this.getServiceInstanceUUID(service);
        if (serviceUUID == null) {
            log("registerServiceAttribute:: WARNING. ServiceInstanceUUID not found for service "
                    + service.getClass().toString());
            return;
        }

        _gmp.registerServiceAttribute(serviceUUID, attrKey, attrVal);
    }

    public String startPeerSearch(Service service, byte[] param, long TTL) {
        String serviceUUID = this.getServiceInstanceUUID(service);
        return _gmp.startPeerSearch(serviceUUID, param, TTL);
    }

    public String startPersistentPeerSearch(Service service, byte[] param) {
        String serviceUUID = this.getServiceInstanceUUID(service);
        return _gmp.startPeerSearch(serviceUUID, param, -1);
    }

    public void stopPersistentPeerSearch(Service service, String searchUUID) {
        String serviceUUID = this.getServiceInstanceUUID(service);
        _gmp.stopPeerSearch(serviceUUID, searchUUID);
    }

    public LocalCoordinator getLocalCoordinatorRef()
    {
        // TODO Auto-generated method stub
        return null;
    }

    public void initLocalCoordinator (String className, String localNodeUUID, String centralCoordName)
        throws Exception
    {
        // TODO Auto-generated method stub
    }

    public String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout)
    {
        // TODO Auto-generated method stub
        return null;
    }

    public String findAServiceInstance (String serviceName, String attrKey, String attrValue, long timeout, boolean force)
    {
        // TODO Auto-generated method stub
        return null;
    }

    public Vector findServiceInstances (String serviceName, String attrKey, String attrValue, long timeout, int maxInstanceNum)
    {
        return null;
    }

    public void initLocalCoordinator(String localCoordClassName)
        throws Exception
    {
        // TODO Auto-generated method stub
    }
}
