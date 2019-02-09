package us.ihmc.android.aci.dspro.wear

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.graphics.Color
import android.os.Build
import android.support.v4.app.NotificationCompat
import android.support.v4.app.NotificationManagerCompat

import android.app.PendingIntent
import android.content.Intent

import us.ihmc.android.aci.dspro.DSProActivity
import us.ihmc.android.aci.dspro.R

/**
 * Notifier.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
object Notifier {

    private const val NOTIFICATION_CHANNEL_ID = "us.ihmc.android.aci.dspro.NOTIFICATION_CHANNEL"

    private fun prebuild(context: Context): NotificationCompat.Builder {
        val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        //String NOTIFICATION_CHANNEL_ID = context.getString(R.string.notification_channel_dspro);


        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) { // Since API Level 26, this is needed
            val notificationChannel = NotificationChannel(NOTIFICATION_CHANNEL_ID, "DSPro Notifications", NotificationManager.IMPORTANCE_HIGH)

            // Configure the notification channel.
            notificationChannel.description = "A channel for DSPro related notifications."
            notificationChannel.enableLights(true)
            notificationChannel.lightColor = Color.RED
            notificationChannel.vibrationPattern = longArrayOf(0, 1000, 500, 1000)
            notificationChannel.enableVibration(true)
            notificationManager.createNotificationChannel(notificationChannel)
        }

        return NotificationCompat.Builder(context, NOTIFICATION_CHANNEL_ID)
    }


    /**
     * Notifies the user of the specific event passed as id. Check NotificationType for the list
     * of supported notifications.
     *
     * @param context: the current Context
     * @param id: the id of the NotificationType
     * @param title: a title for the notification
     * @param text: a text for the notification
     */
    fun notify(context: Context, id: Int, title: String, text: String) {
        // Build intent for notification content
        val viewIntent = Intent(context, DSProActivity::class.java)
        val viewPendingIntent = PendingIntent.getActivity(context, 0, viewIntent, 0)

        val builder = prebuild(context)



        builder.setAutoCancel(true)
                .setDefaults(Notification.DEFAULT_ALL)
                .setWhen(System.currentTimeMillis())
                .setTicker(title) //se if variable ticker is needed
                .setContentTitle(title)
                .setContentText(text)
                .setContentInfo("Info")
                .setContentIntent(viewPendingIntent)

        when (id) {
            NotificationType.NEW_NEIGHBOR.code(),
            NotificationType.DEAD_NEIGHBOR.code(),
            NotificationType.POSITION_UPDATED.code() -> builder.setSmallIcon(R.drawable.ihmc_logo_96_transparent)

            NotificationType.NETWORK_STATUS_GOOD.code() -> builder.setSmallIcon(R.drawable.network_cable_good_48)
            NotificationType.NETWORK_STATUS_BAD.code() -> builder.setSmallIcon(R.drawable.network_cable_bad_48)
        }


//        val notificationBuilder = NotificationCompat.Builder(context)
//                .setSmallIcon(R.drawable.ic_notification_overlay)
//                .setContentTitle(title)
//                .setContentText(text)
//                .setContentIntent(viewPendingIntent)

        // Get an instance of the NotificationManager service
        val notificationManager = NotificationManagerCompat.from(context)
        // Build the notification and issues it with notification manager.
        notificationManager.notify(id, builder.build())
    }
}
