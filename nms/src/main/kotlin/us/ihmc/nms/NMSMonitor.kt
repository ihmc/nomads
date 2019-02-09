package us.ihmc.nms

import org.slf4j.LoggerFactory
import us.ihmc.util.StringUtil

/**
 * Created by gbenincasa on 10/3/17.
 */

class Monitor(private val host: String="127.0.0.1", private val port: Int = NMSProxy.DEFAULT_PORT) {

    private val logger = LoggerFactory.getLogger(Monitor::class.simpleName)
    private val proxy = NMSProxy(host, port)

    fun monitor() {
        val desc = "NMSProxy Server: $host:$port"
        try {
            proxy.init()
            logger.info("Connected to " + desc)
            proxy.registerNetworkMessageServiceListener ('d'.toByte(), { cback: MessageArrived ->
                logger.info(StringBuilder(if (cback.isUnicast) "uni" else "multi")
                        .append("cast message Arrived from ${cback.srcIPAddr}").toString())
            })
            logger.info("Registered Monitor")
        }
        catch(ex: Exception) {
            logger.warn("Error while connecting to " + desc + ": " + StringUtil.getStackTraceAsString(ex))
        }
    }
}

fun main(args : Array<String>) {
    var port = NMSProxy.DEFAULT_PORT
    var host = "127.0.0.1"
    args.forEachIndexed { i, a ->
        when (a) {
            "-s", "--server" -> host = args[i+1]
            "-p", "--port" -> port = args[i+1].toInt()
        }
    }
    Monitor(host, port).monitor()
}
