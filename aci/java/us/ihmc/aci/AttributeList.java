package us.ihmc.aci;

/**
 * AttributeList
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @author Christoher Eagle (ceagle@ihmc.us)
 * @author Florian Bertele (fbertele@ihmc.us)
 * @version $Revision$
 *          Created on May 17, 2004 at 4:24:12 PM
 *          $Date$ \
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class AttributeList
{
    /**
    * Specifies the name of a node (FGInfoVertex) in the fginfo. The name of a node is not necessarily
    * the same as the ID of a node (NODE_ID). IDs are required to be unique. By default, the name
    * of a Node matches it's ID, until it is explicitly changed. In general, the name of the node is
    * what should be displayed by visualization routing, as oposed to the IDs of the nodes.
    * Note that the ID of a node (or any other fginfo component) is NOT an attribute. IDs are unique
    * and are part of the fginfo component.
    */
    public static final String NODE_NAME = "node_name";

    /**
    * Specifies the type of the node in the fginfo. Currently the known types are:
    * ENVIRONMENT or AGENT
    */
    public static final String NODE_TYPE = "node_type";

    /**
    * Notifies the node to become a comms relay at location XxY, XxY is specified as a string
    */
    public static final String NODE_BECOME_RELAY_AT= "node.become.relay.at";

    /**
    * This property can assume ANY string property. If it exists, the node can be moved,
    * otherwise it is static. Note: If the node is static, please make sure to REMOVE this
    * property. Setting it to FALSE will not help.
    */
    public static final String NODE_MOBLIE= "node_mobile";

    /**
    * Specifies what type of model (or object) to be loaded in 3-D system.  The types that are currently
    * supported are: sphere, hovercraft, c130, tank, mine and f18. Types are specified exactly as in the list,
    * all in lowercase. Default type is a sphere (i.e ENVIRONMENT_TYPE = null).  The default sphere is blue,
    * with a radius of 1 meter and initial position at the origin.  If node has x,y,z position sphere
    * will be located at nodal position.
    */
    public static final String ENVIRONMENT_TYPE = "environment_type";

    /**
    *Specified the URI of the environemnt in the for (tcp://host:port) as a string.
    * This URI is used to send AGENTS to the environment, not to send FlexFeedMessages
    */
    public static final String ENVIRONMENT_URI = "environment_uri";

    /**
    *Specifies a list of COMM URI's handled by the environemnt for FlexFeed messaging
    * In general, each communication provider supported by the environment will create it's own URI
    * and will report it as part of this list. The Schema of the URI defines the name of the
    * flexfeed commProvider (tcp://...., is a FlexFeed TCP comm provider, while udp://... is
    * a udp comm provider). The order in which the URI's are reported is important and specified
    * the order - preference - for receivind data/control data, however, reliability characteristics
    * are also taken into account when choosing a provider.
    */
    public static final String ENVIRONMENT_FFCOMM_URILIST = "environment_ffcomm_urilist";

    /**
    * The color of the node. This can be used by the viewer
    */
    public static final String NODE_COLOR = "node_color";

    /**
    * Specifies the domain (or semi-column separated list of domains) that a node is part of.
    */
    public static final String NODE_DOMAIN = "node_domain";

    /**
    * X-coordinate location of a node. The default assumption is that this coordinate is part of a 3-D system
    * with origin at (0,0,0). The system specify distances in meters, in primitive DOUBLE (Object) values that
    * can be negative or positive. Any further constraints in scale and range must be aggreed upon (and verified by)
    * each set of applications.  The x-z plane is the base or ground plane; where x values can be thought of
    * as being east or west values from the z axis.  Values to west are positive and to east are negative.
    */
    public static final String NODE_XPOS = "node_xpos";

    /**
    * Target X-coordinate location of a node. This is just an auxiliar property that specifies the designated
    * target location for that node. It can be used by a moving robot.
    * The default assumption is that this coordinate is part of a 3-D system
    * with origin at (0,0,0). The system specify distances in meters, in primitive DOUBLE (Object) values that
    * can be negative or positive. Any further constraints in scale and range must be aggreed upon (and verified by)
    * each set of applications.  The x-z plane is the base or ground plane; where x values can be thought of
    * as being east or west values from the z axis.  Values to west are positive and to east are negative.
    */
    public static final String NODE_TARGET_XPOS = "node_target_xpos";

    /**
    * Target Y-coordinate location of a node. This is just an auxiliar property that specifies the designated
    * target location for that node. It can be used by a moving robot.
    * The default assumption is that this coordinate is part of a 3-D system
    * with origin at (0,0,0). The system specify distances in meters, in primitive DOUBLE (Object) values that
    * can be negative or positive. Any further constraints in scale and range must be aggreed upon (and verified by)
    * each set of applications.  The x-z plane is the base or ground plane; where x values can be thought of
    * as being east or west values from the z axis.  Values to west are positive and to east are negative.
    */
    public static final String NODE_TARGET_YPOS = "node_target_ypos";

    /**
    * Y-coordinate location of a node. The default assumption is that this coordinate is part of a 3-D system
    * with origin at (0,0,0). The system specify distances in meters, in primitive DOUBLE (Object) values that
    * can be negative or positive. Any further constraints in scale and range must be aggreed upon (and verified by)
    * each set of applications.  The y axis is altitude or height; positive is above ground and negative values
    * are below ground.
    */
    public static final String NODE_YPOS = "node_ypos";

    /**
     * The speed of the node (m/s)
     */
    public static final String NODE_SPEED = "node_speed";

    /**
    * Z-coordinate location of a node. The default assumption is that this coordinate is part of a 3-D system
    * with origin at (0,0,0). The system specify distances in meters, in primitive DOUBLE (Object) values that
    * can be negative or positive. Any further constraints in scale and range must be aggreed upon (and verified by)
    * each set of applications.  The x-z plane is the base or ground plane; where z values can be thought of
    * as being north or south values from the x axis.  Values to north are positive and to south are negative.
    */
    public static final String NODE_ZPOS = "node_zpos";

    /**
     * latitude of the node, as a double.
     */
    public static final String NODE_GPS_LATITUDE = "node_gps_latitude";

    /**
     * longitude of the node, as a double
     */
    public static final String NODE_GPS_LONGITUDE = "node_gps_longitude";

    /**
     * altitude of the node, as a double
     */
    public static final String NODE_GPS_ALTITUDE = "node_gps_altitude";

    /**
     * altitude of the node, as a int
     */
    public static final String EDGE_GPS_PRECISION = "node_gps_precision";

    /**
    * 3-D system user can change viewing position dynamically.
    *
    * NORMALIZED VIEW means: A node/model with heading = 0 would appear as head-on to a user with initialized
    * GUI; therefore, north points out of screen and south into screen.
    *
    * Heading is the node's direction in a 3-D system.  Heading is limited 0 to 360, and these limits are
    * enforced automatically in class Model.  Headings specify direction given in primitive INTEGER (Object)
    * values denoting degrees. Heading changes occur in clockwise direction.
    *
    * NORMALIZED VIEW:
    *    0 is north, 90 is east, 180 is south and 270 is west.
    */
    public static final String NODE_HEADING = "node_heading";

    /**
    * 3-D system user can change viewing position dynamically.
    *
    * NORMALIZED VIEW means: A node/model with heading = 0 would appear as head-on to a user with initialized
    * GUI; therefore, north points out of screen and south into screen.
    *
    * Pitch changes in a loop from 0 to 360.  Boundary limits are enforce automatically in class Model.  Pitch
    * is given in primitive INTEGER (Object) values denoting degrees. The term "nose" used in following
    * explanations means node/model front end.
    *
    * NORMALIZED VIEW:
    *    1 to 89 is nose above horizon.
    *    91 to 180 nose above horizon but node/model is inverted.
    *    181 to 269 nose below horizon but node/plane is inverted.
    *    271 to 359 nose below horizon.
    *
    *    0 (up-right) and 180 (inverted) both with nose on horizon.
    *    90 is vertical with nose pointed up, where 270 is vertical with nose pointed down.
    */
    public static final String NODE_PITCH = "node_pitch";

    /**
    * 3-D system user can change viewing position dynamically.
    *
    * NORMALIZED VIEW means: A node/model with heading = 0 would appear as head-on to a user with initialized
    * GUI; therefore, north points out of screen and south into screen.
    *
    * Roll is primitive INTEGER (Object) values that denote degrees.  The roll boundary limits are 0 to 360 and
    * are automatically enforced in class Model.  Roll changes in right wing roll direction (i.e. right wing dips
    * below the horizon).
    *
    * NORMALIZED VIEW:
    *   0 would appear as normal node/model view.
    *   90 would appear as a 90 degree right roll position.
    *   180 would appear as node/model upside down (inverted).
    *   270 would appears as a 90 degree left roll position.
    */
    public static final String NODE_ROLL = "node_roll";

    /**
    * Specifies the URI (jave.net.URI) that should be use when sending messages to this node.
    * The URI is a inserted as a String (NOT AN URI OBJECT). The String is the the in the form:
    * 'tcp://202.202.202.202:3333".
    */
    public static final String NODE_URI = "node_uri";


    /**
     * The IP address of the node (often in Adhoc mode this is the identifier of the node)
     */
    public static final String NODE_ADHOC_IP = "node_adhoc_ip";

    /**
    * NODE_AVAILABILITY is java.lang.Double ranging from 0 to 1 which describes
    * what percentage of node_resources is currently available and not occupied by other streams.
    * This attribute is assigned only to nodes whoes node_type = ENVIRONMENT (physical machines)
    */
    public static final String NODE_AVAILABILITY = "node_availability";

    /**
    * NODE_RELAY_COST is a positive java.lang.Double that describes the costs per bps (one unit of the measure of
    * the data rate) of forwarding a data stream
    * through the node relative to the reference machine (the tablet PCs). The tablet PCs have a node_relay_cost of 1,
    * a higher value like 1.1 means higher costs on the current machine
    */
    public static final String NODE_RELAY_COST = "node_relay_cost";

    /**
    * NODE_SENSOR_COST is a positive java.lang.Double that describes the sensor costs per bps (one unit of the measure of the
    * data rate) of acquiring a data stream from a sensor relative to the reference machine (the tablet PCs). The tablet PCs have
    * a node_sensor_cost of 1, a higher value like 1.1 means higher costs on the current machine.
    * This attribute is set for all ENVIRONMENT nodes
    * that can provide a stream.
    */
    public static final String NODE_SENSOR_COST = "node_sensor_cost";

    /**
    * NODE_SENSOR_FPS is a positive java.lang.Integer that describes the frames
    * being sent/requested per second. This attribute is set for all AGENT nodes
    * that can provide a stream.
    */
    public static final String NODE_SENSOR_FPS = "node_sensor_fps";

    /**
    * NODE_SENSOR_RESOLUTION is a String formatted "%1x%2", where %1 and %2 are
    * positive integers describing the x- respectively the y-axis resolution.
    * This attribute is set for all AGENT nodes that can provide a stream.
    */
    public static final String NODE_SENSOR_RESOLUTION = "node_sensor_resolution";

    /**
    * NODE_SENSOR_COMPRESSION is a positive java.lang.Double between 1 and 0
    * describing the compression rate of the provided data. A value of 0.6 means that
    * the sent data was compressed to 60% of its original size.
    * This attribute is set for all AGENT nodes that can provide a stream.
    */
    public static final String NODE_SENSOR_COMPRESSION = "node_sensor_compression";

    /**
    * NODE_SENSOR_COLOR_DEPTH is a positive java.lang.Integer, describing the
    * color depth of the picture material provided in bits.
    * This attribute is set for all AGENT nodes that can provide a stream.
    */
    public static final String NODE_SENSOR_COLOR_DEPTH = "node_sensor_color_depth";

    /**
     * NODE_SENSOR_TIMELAG is a positive java.lang.Integer that describes MAXIMUM CAPABILITY
     * of this node (assumed to be a video sensor) in delaying the stream (in milliseconds).
    */
    public static final String NODE_SENSOR_TIMELAG = "node_sensor_timelag";

   /**
    * NODE_TRANSFORM_COST is a positive java.lang.Double that describes the costs of transforming a data stream into another
    * data stream. The actual transformation cost is calculated out of this factor, NODE_RESOURCES, NODE_AVAILABILITY and the
    * input and output data rates. The relation of (1 - NODE_AVAILABILITY) and NODE_TRANSFORM_COSTis intended to
    * be superlinear in order to prefer finding paths over not used nodes rather than using the last resources of
    * already used nodes. NODE_TRANSFORM_COST is a relative cost factor to the reference machine, the tablet PCs.
    * The tablet PCs have a node_transform_cost of 1, a value of 1.1 indicates higher transformation costs.
    **/
    public static final String NODE_TRANSFORM_COST = "node_transform_cost";

    /**
    * Specifies the type of the edge in the fginfo. Currently the known types are:
    * REACHABILITY    - exists between two vertices that can reach each other
    * MEMBERSHIP   - between a vertex type (AGENT) and a vertex type (ENVIRONMENT) to indicate membership
    *  - the source of a MEMBERSHIP edge is the member (like an AGENT), the target is the ENVIRONMENT node
    * STREAM_REQUEST - A stream request between two (or multiple) vertices type (NODE)
    * DATA_STREAM - Actual streams flowing between verices (NODES and ENVIRONMENTS) in the fginfo
    * MOCKET_CONNECTION - A mocket connection
    *
    */
    public static final String EDGE_TYPE = "edge_type";

    /**
    * Specifies the state of a mocket connection edge
    * HEALTHY - the mocket connection represented by the edge with type (MOCKET_CONNECTION) is healthy
    * BROKEN - the mocket connection represented by the edge with type (MOCKET_CONNECTION) is broken
    */
    public static final String MOCKET_CONNECTION_STATE = "mocket_connection_state";

    /**
    * Specifies the state of a node as far as FGraph server is concerned.
    * Ther are tow possible String values for this variable "ACTIVE", "UNKNOWN"
    * Active nodes are those that (through some means such as heart-beat, updates, etc.)
    * maintain their state with the server. If there are reasons to believe that the
    * information at the server is outdated or untrusted, the state of the node is
    * changed to UNKNONWN.
    */
    public static final String NODE_STATUS = "node_status";

    /**
    * EDGE_CAPACITY is a postitve java.lang.Double representing the connection speed of an
    * edge in Mbits/s.
    */
    public static final String EDGE_CAPACITY = "edge_capacity";
    
    /**
     * TODO :: doc.
     */
    public static final String EDGE_CAPACITY_FACTOR = "edge_capacity_factor";

    /**
     * TODO :: doc.
     */
    public static final String EDGE_MAXIMUM_CAPACITY = "edge_maximum_capacity";    

    /**
     * Metrics from BBN - Must be identified and discussed
     * ALL STRINGS
     */
    public static final String EDGE_BBN_COST = "edge_bbn_cost";

    public static final String EDGE_BBN_QUALITY= "edge_bbn_quality";

    public static final String EDGE_BBN_DB = "edge_bbn_db";

    public static final String EDGE_BBN_UTILIZATION = "edge_bbn_utilization";

    /**
    * EDGE_CURRENT_COST is a postitve java.lang.Double representing bandwidth use
    * of edge in Mbits/s.
    */
    public static final String EDGE_CURRENT_COST = "edge_current_cost";

    /**
    * EDGE_AVAILABILITY is a java.lang.Double ranging from 0 to 1 which
    * describes what percentage of edge_resources is currently available and
    * not occupied by other streams.
    */
    public static final String EDGE_AVAILABILITY = "edge_availability";

    /**
    * EDGE_STREAM_DOWNGRADE_COUNTER is a positive java.lang.Integer that is for
    * internal use in the Coordinator (ulm algorithm) only. It is applied STREAM_REQUESTs of
    * the FGraphLocal. Initially it is set to 0 and increases with each downgrading of the
    * stream by 1.
    */
    public static final String EDGE_STREAM_DOWNGRADE_COUNTER = "edge_stream_downgrade_counter";

    /**
    * EDGE_STREAM_FPS is a positive java.lang.Double that describes the frames
    * being sent/requested per second. This attribute is set for the edge_types
    * STREAM_REQUEST and DATA_STREAM.
    */
    public static final String EDGE_STREAM_FPS = "edge_stream_fps";

    /**
    * EDGE_STREAM_RESOLUTION is a String formatted "%1x%2", where %1 and %2 are
    * positive integers describing the x- respectively the y-axis resolution.
    * Exists in a DATA_STREAM edge;
    */
    public static final String EDGE_STREAM_RESOLUTION = "edge_stream_resolution";

    /**
    * EDGE_STREAM_COMPRESSION is a positive java.lang.Double between 1 and 0
    * describing the compression rate of the sent data. A value of 0.6 means that
    * the sent data was compressed to 60% of its original size.
    * Exists in a DATA_STREAM edge;
    */
    public static final String EDGE_STREAM_COMPRESSION = "edge_stream_compression";

    /**
     * EDGE_STREAM_TIMELAG is a positive java.lang.Integer describing the delay
     * (timelag) requested. A value of 30 means that this request must be provided
     * with a MINIMUM (not maximum) of 30 milliseconds delay. This variable is NOT use
     * for quality of service but instead for policy enforcement. The idea is that by
     * enforcing a delay in the stream you can reduce, for instance, its security classification.
     * Exists in a DATA_STREAM edge;
     */
    public static final String EDGE_STREAM_TIMELAG = "edge_stream_timelag";

    /**
    * EDGE_STREAM_COLOR_DEPTH is a positive java.lang.Integer, describing the
    * color depth of the picture material sent over the network in bits.
    * Exists in a DATA_STREAM edge;
    */
    public static final String EDGE_STREAM_COLOR_DEPTH = "edge_stream_color_depth";


    /**
    * EDGE_STREAM_SRC_AGENT_NAME is an attribute that exist in a DATA_STREAM
    * edge. It specified the name of the agent (sourceAgent) that will provide the stream
    * defined by the edge.
    */
    public static final String EDGE_STREAM_SOURCE_AGENT = "edge_stream_src_agent";

    /**
    * EDGE_STREAM_SRC_AGENT_CLASS is an attribute that exist in a DATA_STREAM
    * edge. It specifies the name of class that should be instantiated as source for the data-straem
    * Note that FlexFeed WILL alwyas attempt to instantiate this calss with the name given by
    * the attribute EDGE_STREAM_SOURCE_AGENT_NAME. When the stream refers to an agent that should not
    * be instantiated (such as an actual video source), then only the name should be provided.
    */
    //public static final String EDGE_STREAM_SOURCE_AGENT_CLASS = "edge_stream_src_agent";

    /**
    * EDGE_STREAM_TARGET_AGENT_NAME is an attribute that exist in a DATA_STREAM
    * edge. It specified the name of the agent (targetAgent) that will receive the stream
    * defined by the edge.
    * <florian, please add the new format here....>
    */
    public static final String EDGE_STREAM_TARGET_AGENT = "edge_stream_target_agent";

    /**
    * EDGE_STREAM_SRC_TARGET_CLASS is an attribute that exist in a DATA_STREAM
    * edge. It specifies the name of class that should be instantiated as target for the data-straem
    * Note that FlexFeed WILL alwyas attempt to instantiate this calss with the name given by
    * the attribute EDGE_STREAM_SOURCE_TARGET_NAME. When the stream refers to an agent that should not
    * be instantiated (such as an actual video sink), then only the name should be provided.
    * It also takes indices such as EDGE_STREAM_TARGET_AGENT_CLASS0=us.ihmc.flexfeed.transformationAgent,
    * EDGE_STREAM_TARGET_AGENT_CLASS1=us.ihmc.flexfeed.RelayAgent, etc.
    */
    //public static final String EDGE_STREAM_TARGET_AGENT_CLASS = "edge_stream_target_agent;

    /**
    * EDGE_STREAM_SOURCE_ENV is an attribute that exist in a DATA_STREAM
    * edge. It specified the UUID of the source environment (the environment where the source agent
    * is running)
    */
    public static final String EDGE_STREAM_SOURCE_ENV = "edge_stream_source_env";

    /**
    * EDGE_STREAM_TARGET_ENV is an attribute that exist in a DATA_STREAM
    * edge. It specified the UUID of the source environment (the environment where the source agent
    * is running)
    */
    public static final String EDGE_STREAM_TARGET_ENV = "edge_stream_target_env";

    /**
    * EDGE_STREAM_REQUEST_ID Specified the ID of the request that resulted int the
     * deployment of this DATA_STREAM edge.
    */
    public static final String EDGE_STREAM_REQUEST_ID = "edge_stream_request_id";


    /**
     * The name of th agent requesting the stream (String)
     */
    public static final String EDGE_STREAM_REQUESTOR = "edge_sgtream_requestor";


    /**
    * EDGE_STREAM_SENSOR_ID ia a String, describing the
    * vertexID of the sensor where this stream is emitted.
    * This attribute is held by all DATA_STREAM edges.
    */
    public static final String EDGE_STREAM_SENSOR_ID = "edge_stream_sensor_id";


    //public static final String COORDINATOR_NAME = "COORDINATOR";
    
    /**
     * Specifies edge weight. The specification is in primitive INTEGER(Object)
     * Used by FFD simulator 
     */
    public static final String EDGE_INT_WEIGHT = "edge_int_weight";
    
    /**
     * Specifies the state of the simulation. Currently, the known types are:
     * READY - A new simulation can start
     * IN_PROGRESS - The simulation is in progress
     * DONE - The simulation has been completed. Waiting for a new fginfo...
     * The specification is in primitive String(Object)
     * Used by FFD simulator
     */
    public static final String SIMULATION_STATE = "simulation_state";
    

}
