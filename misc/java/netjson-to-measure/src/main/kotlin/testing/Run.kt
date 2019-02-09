package testing

import com.google.gson.GsonBuilder
import netjson.messages.deviceconfiguration.DeviceConfigurationMessage
import netjson.messages.devicemonitoring.DeviceMonitoringMessage
import netjson.messages.networkroutes.NetworkRoutesMessage

class Run {
    companion object {
        @JvmStatic
        fun main(args: Array<String>) {
            val deviceConfigurationJSON = this::class.java.getResource("/examples/DeviceConfiguration.json").readText()
            val deviceMonitoringJSON = this::class.java.getResource("/examples/DeviceMonitoring.json").readText()
            val networkGraphJSON = this::class.java.getResource("/examples/NetworkGraph.json").readText()
            val networkingCollectionJSON = this::class.java.getResource("/examples/NetworkingCollection.json").readText()
            val networkRoutesJSON = this::class.java.getResource("/examples/NetworkRoutes.json").readText()


            // Convert to the Kotlin objects
//            val networkRoutesMessage = GsonBuilder().create().fromJson(networkRoutesJSON, NetworkRoutesMessage::class.java)
//            val networkGraphMessage = GsonBuilder().create().fromJson(networkGraphJSON, NetworkGraphMessage::class.java)
//            val deviceMonitoringMessage = GsonBuilder().create().fromJson(deviceMonitoringJSON, DeviceMonitoringMessage::class.java)
            val deviceConfigurationMessage = GsonBuilder().create().fromJson(deviceConfigurationJSON, DeviceConfigurationMessage::class.java)
            print(deviceConfigurationMessage.general!!.description!!)
//            val networkCollectionMessage = GsonBuilder().create().fromJson(networkingCollectionJSON, NetworkCollectionMessage::class.java)




        }
    }
}