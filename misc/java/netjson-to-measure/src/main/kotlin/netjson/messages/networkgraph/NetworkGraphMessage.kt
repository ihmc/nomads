package netjson.messages.networkgraph

import netjson.NetJSONTypes
import netjson.general.NetJSONObject

/**
 * A list of nodes and links known by a node
 *
 * @property protocol Name of the routing protocol implementation ("static" when static routes)
 * @property version Version of routing protocol (may be null when static route)
 * @property metric Name of routing metric (may be null when static route)
 * @property nodes List of Nodes
 * @property links List of Links
 * @property revision String indicating revision from which the routing protocol daemon binary was built
 * @property topology_id Arbitrary identifier of the topology
 * @property router_id Arbitrary identifier of the router on which the protocol is running
 * @property label Human readable label for the topology
 */
class NetworkGraphMessage : NetJSONObject() {
    var protocol: String = ""
    var version: String = ""
    var metric: String = ""
    var nodes: @JvmSuppressWildcards List<Node>? = null
    var links: @JvmSuppressWildcards List<Link>? = null
    var revision: String? = ""
    var topology_id: String? = ""
    var router_id: String? = ""
    var label: String? = ""

    /**
     * @property id Arbitrary identifier of the node (eg: ipv4, ipv6, mac address)
     * @property label Human readable label of the node
     * @property local_addresses Array of IP addresses
     * @property properties Hashmap (key,varue) of properties that define this node
     */
    class Node {
        var id: String = ""
        var label: String = ""
        var local_addresses: List<String>? = null
        var properties: HashMap<String, String>? = null
    }


    /**
     * @property source Id of the source node
     * @property target Id of the target node
     * @property cost varue of routing metric indicating the outgoing cost to reach the destination (Infinity and NaN are not allowed)
     * @property cost_text Human readable representation of cost
     * @property properties Hashmap (key, value) of properties that define this link
     */
    class Link {
        var source: String = ""
        var target: String = ""
        var cost: String = ""
        var cost_text: String = ""
        var properties: HashMap<String, String>? = null
    }

}
