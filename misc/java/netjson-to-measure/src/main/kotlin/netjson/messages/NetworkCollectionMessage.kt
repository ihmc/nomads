package netjson.messages

import netjson.general.NetJSONObject

class NetworkCollectionMessage : NetJSONObject() {
    override val type: String = ""
    val collection: List<NetJSONObject>? = null
}