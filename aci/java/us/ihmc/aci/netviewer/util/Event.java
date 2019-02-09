package us.ihmc.aci.netviewer.util;

import us.ihmc.aci.nodemon.data.Data;
import us.ihmc.aci.nodemon.data.NodeMonDataType;
import us.ihmc.aci.nodemon.data.node.Node;

/**
 * Wrapper for the class <code>Node</code> representing elements added to the callbacks queue
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Event
{
    /**
     * Constructor used when a new node is added
     * @param node contains all the node info
     */
    public Event (Node node)
    {
        _node = node;
        _type = EventType.newNode;
        _nodeId = node.getId();
        _nmdType = null;
        _bUpdatedData = null;
        _oUpdatedData = null;
    }

    /**
     * Constructor used when updates about an already inserted node arrive
     * @param nodeId node id
     * @param nmdType node monitor update type
     * @param updatedData byte array representation of a given type of data update
     */
    public Event (String nodeId, NodeMonDataType nmdType, byte[] updatedData)
    {
        _node = null;
        _nodeId = nodeId;
        _type = EventType.updatedNode;
        _nmdType = nmdType;
        _bUpdatedData = updatedData;
        _oUpdatedData = null;
    }

    /**
     * Constructor used when updates about an already inserted node arrive
     * @param nodeId node id
     * @param nmdType node monitor update type
     * @param updatedData data update
     */
    public Event (String nodeId, NodeMonDataType nmdType, Data updatedData)
    {
        _node = null;
        _nodeId = nodeId;
        _type = EventType.updatedNode;
        _nmdType = nmdType;
        _bUpdatedData = null;
        _oUpdatedData = updatedData;
    }

    /**
     * Constructor used when a node dies
     * @param nodeId node id
     */
    public Event (String nodeId)
    {
        _node = null;
        this._nodeId = nodeId;
        _type = EventType.deadNode;
        _nmdType = null;
        _bUpdatedData = null;
        _oUpdatedData = null;
    }

    /**
     * Gets the node instance
     * @return the node instance
     */
    public Node getNode()
    {
        return _node;
    }

    /**
     * Gets the node id
     * @return the node id
     */
    public String getNodeId()
    {
        return _nodeId;
    }

    /**
     * Gets the event type
     * @return the event type
     */
    public EventType getType()
    {
        return _type;
    }

    /**
     * Gets the node monitor data type
     * @return the node monitor data type
     */
    public NodeMonDataType getNmdType()
    {
        return _nmdType;
    }

    /**
     * Gets the byte array representing the updated data
     * @return the byte array representing the updated data
     */
    public byte[] getBUpdatedData()
    {
        return _bUpdatedData;
    }

    /**
     * Gets the updated data
     * @return the updated data
     */
    public Data getOUpdatedData()
    {
        return _oUpdatedData;
    }


    private final Node _node;
    private final String _nodeId;
    private final EventType _type;
    private final NodeMonDataType _nmdType;
    private final byte[] _bUpdatedData;
    private final Data _oUpdatedData;
}
