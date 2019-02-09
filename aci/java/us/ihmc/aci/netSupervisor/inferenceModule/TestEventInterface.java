package us.ihmc.aci.netSupervisor.inferenceModule;

import com.google.protobuf.Duration;
import us.ihmc.aci.ddam.NetworkEvent;
import us.ihmc.aci.ddam.NetworkEventContainer;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.InputStreamReader;
import java.io.InterruptedIOException;
import java.net.Socket;
import java.util.Objects;

/**
 * Created by nomads on 2/9/17.
 */
public class TestEventInterface {
    public static void main (String[] args) throws Exception {
        NetworkEventContainer.Builder eventContainer = NetworkEventContainer.newBuilder();
        Socket socket = new Socket("localhost", 20000);
        DataOutputStream outputStream = new DataOutputStream(socket.getOutputStream());
        byte[] message;
        NetworkEvent.Builder event;
        Duration.Builder d = Duration.newBuilder();
        String otherEvents = "y";
        String tempString;
        long duration = 0;
        BufferedReader input = new BufferedReader(new InputStreamReader(System.in));
        System.out.println("----------- Event interface-----------");
        try {
            //while (true) {
                while (otherEvents.equals("y")) {
                    event = NetworkEvent.newBuilder();
                    System.out.println("Type the new event parameters:");
                    System.out.print("- event description: ");
                    tempString = input.readLine();
                    if (tempString != null && !tempString.equals("")) {
                        event.setDescription(tempString);
                    }
                    System.out.print("- event target: ");
                    tempString = input.readLine();
                    if (tempString != null && !tempString.equals("")) {
                        event.setTarget(tempString);
                    }
                    System.out.print("- event value: ");
                    tempString = input.readLine();
                    if (tempString != null && !tempString.equals("")) {
                        event.setValue(tempString);
                    }
                    System.out.print("- event duration (seconds): ");
                    String durationString = input.readLine();
                    if (durationString.equals("")) {
                        duration = Long.MAX_VALUE;
                    } else {
                        try {
                            duration = Long.parseLong(durationString);
                        } catch (Exception e) {
                            System.out.println("Wrong input");
                        }
                    }
                    event.setDuration(d.setSeconds(duration).build());
                    eventContainer.addEvents(event.build());
                    System.out.println("Other events (y/n): ");
                    otherEvents = input.readLine();
                }
                Objects.requireNonNull(eventContainer.build(), "Container to be serialized can't be null");
                message = eventContainer.build().toByteArray();
                outputStream.write(message);
                System.out.println("Event Sent");
                otherEvents = "y";
            //}
        }
        catch (InterruptedIOException e) {
            socket.close();
        }
    }

}
