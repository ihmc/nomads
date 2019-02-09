package netjson.messages.devicemonitoring

import netjson.general.NetJSONObject

/**
 * Information that indicates the behavior of a device that changes over time
 *
 * @property general Contains monitoring data that is related to the whole device
 * @property resources Describes hardware resources being used by the system
 * @property interfaces List of interfaces that represent monitoring data related to network interfaces
 */
class DeviceMonitoringMessage  : NetJSONObject() {
    val general: General? = null
    val resources: Resources? = null
    val interfaces: List<Interface>? = null

    /**
     * @property local_time The device's local time
     * @property uptime Time since boot (in seconds)
     */
    class General {
        val local_time: Long? = null // Device local time
        val uptime: Long? = null // Device local time
    }

    /**
     * @property load List of 3 numeric values representing load average values in
     *      1) the last minute 2) the last 5 minutes and 3) the last 15 minutes
     * @property memory Object describing Ram usage
     * @property swap Object describing swap memory usage
     * @property connections Object describing connection state
     * @property processes Object describing running processes
     * @property cpu Describes CPU usage
     * @property flash Describes flash usage
     * @property storage Describes storage usage
     */
    class Resources {
        val load: List<Float>? = null
        val memory: Memory? = null
        val swap: Swap? = null
        val connections: Connections? = null
        val processes: Processes? = null
        val cpu: CPU? = null
        val flash: Flash? = null
        val storage: Storage? = null


        class Memory {
            val total: Int? = null
            val free: Int? = null
            val buffered: Int? = null
            val cache: Int? = null
        }

        class Swap {
            val total: Int? = null
            val free: Int? = null
        }

        class Connections {
            val ipv4: IPConnection? = null
            val ipv6: IPConnection? = null

            /**
             * @property tcp Number of TCP connections
             * @property udp Number of UDP connections
             */
            class IPConnection {
                val tcp: Int = 0
                val udp: Int = 0
            }
        }

        /**
         * Properties are recommended to be included but not required
         * @property running Recommended to be filled but not required
         */
        class Processes {
            val running:  Int? = null
            val sleeping: Int? = null
            val blocked:  Int? = null
            val zombie:   Int? = null
            val stopped:  Int? = null
            val paging:   Int? = null
        }

        /**
         * @property frequency Measured in MHz
         */
        class CPU {
            val frequency: Int? = null
            val user: Int? = null
            val system: Int? = null
            val nice: Int? = null
            val idle: Int? = null
            val iowait: Int? = null
            val irq: Int? = null
            val softirq: Int? = null
        }

        // In bytes
        class Flash {
            val total: Int? = null
            val free: Int? = null
        }

        class Storage {
            val total: Int? = null
            val free: Int? = null
        }
    }

    class Interface {
        val name: String? = ""
        val uptime: Long? = null
        val statistics: Statistics? = null

        /**
         * Statistics about the interface
         */
        class Statistics {
            val collisions:           Long? = null
            val rx_frame_errors:      Long? = null
            val tx_compressed:        Long? = null
            val multicast:            Long? = null
            val rx_length_errors:     Long? = null
            val tx_dropped:           Long? = null
            val rx_bytes:             Long? = null
            val rx_missed_errors:     Long? = null
            val tx_errors:            Long? = null
            val rx_compressed:        Long? = null
            val rx_over_errors:       Long? = null
            val tx_fifo_errors:       Long? = null
            val rx_crc_errors:        Long? = null
            val rx_packets:           Long? = null
            val tx_heartbeat_errors:  Long? = null
            val rx_dropped:           Long? = null
            val tx_aborted_errors:    Long? = null
            val tx_packets:           Long? = null
            val rx_errors:            Long? = null
            val tx_bytes:             Long? = null
            val tx_window_errors:     Long? = null
            val rx_fifo_errors:       Long? = null
            val tx_carrier_errors:    Long? = null
        }
    }
}

