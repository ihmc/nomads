package us.ihmc.android.aci.dspro.wear

/**
 * NotificationType.java
 *
 * Enum `NotificationType` handles notification types.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
enum class NotificationType(private val _code: Int) {

    NEW_NEIGHBOR(0),
    DEAD_NEIGHBOR(1),
    POSITION_UPDATED(2),
    NETWORK_STATUS_GOOD(3),
    NETWORK_STATUS_BAD(4);

    fun code(): Int {
        return _code
    }

    companion object {

        fun fromCode(code: Int): NotificationType {
            return when (code) {
                0 -> NEW_NEIGHBOR
                1 -> DEAD_NEIGHBOR
                2 -> POSITION_UPDATED
                3 -> NETWORK_STATUS_GOOD
                4 -> NETWORK_STATUS_BAD
                else -> throw NumberFormatException("Unrecognized code " + code)
            }
        }
    }
}
