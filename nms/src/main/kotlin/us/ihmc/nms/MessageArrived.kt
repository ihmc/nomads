package us.ihmc.nms

/**
 * Created by gbenincasa on 10/3/17.
 */
data class MessageArrived (val incomingIface: String, val srcIPAddr: String,
                           val msgType: Byte, val msgId: Short,
                           val hopCount: Byte, val ui8TTL: Byte, val isUnicast: Boolean,
                           val metadata: ByteArray, val data:ByteArray, val timestamp: Long,
                           val groupMsgCount: Long, val unicastMsgCount: Long)