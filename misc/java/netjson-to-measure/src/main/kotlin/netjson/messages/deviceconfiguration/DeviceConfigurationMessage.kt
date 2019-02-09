package netjson.messages.deviceconfiguration

import netjson.general.NetJSONObject


/**
 * Configuration and properties of a network device
 *
 * @property general General Object that contains general information related to the network device
 * @property hardware Hardware information related to the network device
 * @property operating_system Operating system object
 * @property interfaces List of interfaces that are configured on the device
 * @property radios List of radios available on the system
 * @property routes List of static routes on the device
 * @property dns_servers List of strings denoting DNS servers configured on the device
 * @property dns_search LIst of strings denoting DNS search paths configured on the device
 */
class DeviceConfigurationMessage : NetJSONObject() {
    val general: General? = null
    val hardware: Hardware? = null
    val operating_system: OperatingSystem? = null
    val interfaces: List<Interface>? = null
    val radios: List<Radio>? = null
    val routes: List<StaticRoute>? = null
    val dns_servers: List<String>? = null
    val dns_search: List<String>? = null

    /**
     * @property hostname Hostname of the network device
     * @property timezone Must be value present in the Time Zone Database maintained by IANA
     * @property maintainer Email address of the maintainer of the network device
     * @property description Free form textual description of the network device
     * @property ula_prefix IPv6 Unique Local Address prefix applicable to every network interface
     */
    class General {
        val hostname: String? = null
        val timezone: String? = null
        val maintainer: String? = null // Email address of the maintainer of the network device
        val description: String? = null
        val ula_prefix: String? = null // IPv6 Unique Local Address prefix applicable to every network interface
    }


    /**
     * @property manufacturer Name of the manufacturer
     * @property model Name of the model
     * @property version Version of the device
     * @property cpu CPU specification
     */
    class Hardware {
        val manufacturer: String? = null
        val model: String? = null
        val version: String? = null
        val cpu: String? = null
    }

    /**
     * @property name Name of the operating system or firmware
     * @property kernel Kernel version
     * @property version Version name or number of the OS
     * @property revision Revision from which the binary was built
     * @property description Free-form textual description
     */
    class OperatingSystem {
        val name: String? = null
        val kernel: String? = null
        val version: String? = null
        val revision: String? = null
        val description: String? = null
    }

    /**
     * @property name Name of the interface (no longer than 15 characters and no whitespace)
     * @property type Type of interface
     * @property mac Mac address of interface
     * @property mtu Value of MTU (default to 1500)
     * @property autostart Indicates whether interface should be automatically started after reboot (default to true)
     * @property disabled Indicates whether the interface is disabled (defaults to false)
     * @property bridge_members List of interface names representing bridged interfaces
     * @property addresses Array of Address objects
     * @property wireless WirelessSettings object for settings
     */
    class Interface {
        val name: String = ""
        val type: InterfaceType? = null
        val mac: String? = null
        val mtu: Long? = null
        val txqueuelen: Long? = null
        val autostart: Boolean = true
        val disabled: Boolean = false
        val bridge_members: List<String>? = null
        val addresses: List<Address>? = null
        val wireless: WirelessSettings? = null

        enum class InterfaceType {
            ethernet,
            wireless,
            bridge,
            virtual,
            loopback,
            other
        }

        /**
         * @property proto 'DHCP' or 'Static'
         * @property family 'IPv4' or 'IPv6'
         * @property address Ipv4 or IPv6 address (must be present if type is static)
         * @property mask Integer representing the network mask. Number after '/' in CIDR notation (must be present if 'static')
         * @property gateway Address of the gateway
         */
        class Address {
            val proto: String = ""
            val family: String = ""
            val address: String? = null
            val mask: Long? = null
            val gateway: String? = null
        }


        /**
         * @property radio Name of one of the radios in the RadioObject
         * @property mode Wireless mode
         * @property ssid ESSID of the network, no longer than 32 char
         * @property bssid BSSID, usually used in 'adhoc' or 'wds' mode
         * @property ack_distance Distance between access point and furthest client in meters (no less than 1)
         * @property rts_threshold Frame size at which transmitter must use RTS/CTS protocol (between 0 and 2346)
         * @property frag_threshold Indicates maximum frame size (between 0 and 2346)
         * @property encryption Encryption settings
         */
        class WirelessSettings {
            val radio: String = ""
            val mode: String = ""
            val ssid: String = ""
            val bssid: String? = null
            val hidden: Boolean? = null
            val ack_distance: Int? = null
            val rts_threshold: Int? = null
            val frag_threshold: Int? = null
            val encryption: Encryption? = null

            /**
             * @property protocol Encryption protocol
             * @property key Encryption key
             * @property cipher String representing ciphers (eg: 'auto', 'ccmp', 'tkip', 'tkip+ccmp'
             * @property disabled Whether to disable encryption or not (should default to false)
             */
            class Encryption {
                val protocol: String = ""
                val key: String = ""
                val cipher: String? = null
                val disabled: Boolean? = false
            }
        }
    }

    /**
     * @property name Name of the radio device (WirelessSettings.radio should use a name specified here)
     * @property protocol Wireless protocol used ('802.11ac', '802.11n', '802.11b', etc)
     * @property channel Channel number
     * @property channel_width Channel width in Hertz
     * @property phy Name of the physical device, optional because usually autodetected
     * @property country Two digit country code in ISO 3166-1 alpha-2 format
     * @property tx_power Transmission power in dBm
     * @property disabled Whether the radio should be disabled or not (defaults to false)
     */
    class Radio {
        val name: String = ""
        val protocol: String = ""
        val channel: Int = 0
        val channel_width: Int = 0
        val phy: String? = null
        val country: String? = null
        val tx_power: Int? = null
        val disabled: Boolean? = false
    }

    /**
     * @property destination Destination address of the static route
     * @property next Next hop address for the static route
     */
    class StaticRoute {
        val device: String = ""
        val destination: String = ""
        val next: String = ""
        val cost: Int = 0
    }
}

