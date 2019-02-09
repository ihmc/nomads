package netjson

import com.google.gson.Gson
import com.google.gson.GsonBuilder
import netjson.NetJSONTypes.Companion.DEVICE_CONFIGURATION
import netjson.NetJSONTypes.Companion.DEVICE_MONITORING
import netjson.NetJSONTypes.Companion.NETWORK_COLLECTION
import netjson.NetJSONTypes.Companion.NETWORK_GRAPH
import netjson.NetJSONTypes.Companion.NETWORK_ROUTES
import netjson.general.NetJSONObject
import netjson.messages.NetworkCollectionMessage
import netjson.messages.deviceconfiguration.DeviceConfigurationMessage
import netjson.messages.devicemonitoring.DeviceMonitoringMessage
import netjson.messages.networkgraph.NetworkGraphMessage
import netjson.messages.networkroutes.NetworkRoutesMessage

class Formatter{

    companion object Factory : JSONFactory<NetJSONObject>{
        /**
         * Convert from string into NetJSONObject
         */
        override fun fromJSON(jsonText: String): NetJSONObject? {
            println("Before executing fromJSON()")
            val generic = GsonBuilder().create().fromJson<NetworkGraphMessage>(jsonText, NetworkGraphMessage::class.java)


//            val specific = when (generic.type) {
//                NETWORK_COLLECTION ->
//                    gson.fromJson<NetworkCollectionMessage>(jsonText, NetworkCollectionMessage::class.java)
//
//                DEVICE_CONFIGURATION ->
//                    gson.fromJson<DeviceConfigurationMessage>(jsonText, DeviceConfigurationMessage::class.java)
//
//                DEVICE_MONITORING ->
//                    gson.fromJson<DeviceMonitoringMessage>(jsonText, DeviceMonitoringMessage::class.java)
//
//                NETWORK_ROUTES ->
//                    gson.fromJson<NetworkRoutesMessage>(jsonText, NetworkRoutesMessage::class.java)
//
//                NETWORK_GRAPH ->
//                    gson.fromJson<NetworkGraphMessage>(jsonText, NetworkGraphMessage::class.java)
//                else -> {
//                    null
//                }
//            }

            println("After executing fromJSON()")

            return generic
        }

        override fun toJSON(m: NetJSONObject): String {
            return ""
        }



    }
}