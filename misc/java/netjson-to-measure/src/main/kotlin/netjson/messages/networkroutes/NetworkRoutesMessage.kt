package netjson.messages.networkroutes

import netjson.NetJSONTypes
import netjson.general.NetJSONObject

/**
 * A list of routes of a dynamic routing protocol or statically configured on the device
 *
 * @property protocol Name of routing protocol implementation (may be "static" when representing static routes)
 * @property version Version of routing protocol implementation (may be null when representing static routes)
 * @property metric Name of main routing metric may be null when representing static routes
 * @property routes List of route objects
 * @property revision Revision from which routing protocol daemon binary was built
 * @property router_id Router on which protocol is running
 * @property topology_id Topology
 */
class NetworkRoutesMessage  : NetJSONObject() {
    var protocol: String = "" // May be "static" when representing static routes
    var version: String = "" // May be null when representing static routes
    var metric: String = "" // Name of main routing metric may be null when representing static routes
    var routes: List<Route>? = null // List of route objects
    var revision: String? = "" // Revision from which routing protocol daemon binary was built
    var router_id: String? = "" // Router on which protocol is running
    var topology_id: String? = "" // Topology

    class Route {
        var destination: String = "" // Ip prefix or mac that matches destination
        var next: String = ""// Ip or mac of next hop
        var device: String? = "" // Interface traffic goes to (may be omitted if static route)
        var cost: Int = 0 // Value of routing metric (may be omitted when static routes (Infinity and NaN not allowed))
        var cost_text: String? = ""
        var source: String? = "" // Source address of the route
    }
}