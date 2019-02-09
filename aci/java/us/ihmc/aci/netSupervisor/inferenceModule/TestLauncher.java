package us.ihmc.aci.netSupervisor.inferenceModule;
import com.google.protobuf.Duration;
import us.ihmc.aci.ddam.MicroFlow;
import us.ihmc.aci.ddam.NetworkEvent;
import us.ihmc.aci.ddam.NetworkEventContainer;
import us.ihmc.aci.ddam.QoS;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

public class TestLauncher
{

    public static void main(String[] args) throws Exception
    {
        _IMM = new ClaspInferenceModuleManager();
        QoS qosMessage;
        _eventsRelated = NetworkEventContainer.newBuilder();
        System.out.println("here");
        _IMM.init("../../aci/cpp/netSupervisor/claspModule/lp_sources/");
        createLinkType("decrease", "change", "2");
        Duration d = Duration.newBuilder().setSeconds(Long.MAX_VALUE).build();
        _IMM.createEvent(NetworkEvent.newBuilder().setDescription("newLinkType").setTarget("2")
                .setValue("SATCOM").setDuration(d).build(), _eventsRelated.build());
        _eventsRelated = NetworkEventContainer.newBuilder();
        createLinkType("increase", "change", "4");
        _IMM.createEvent(NetworkEvent.newBuilder().setDescription("newLinkType").setTarget("4")
                .setValue("LAN").setDuration(d).build(), _eventsRelated.build());
        //_IMM.createInferenceTag("diagnostic");

        //testCode();

        eventReceiver();
    }

    private static void eventReceiver ()
    {
        Socket socket;
        ServerSocket serverSocket;
        String dataBuffer = "", tempString;
        NetworkEventContainer eventsBuilder;
        QoS qosMessage = null;
        try {
            serverSocket = new ServerSocket(20000);
            while (true) {

                socket = serverSocket.accept();
                //BufferedReader sockInput = new BufferedReader(new InputStreamReader(socket.getInputStream()));

                //tempString = sockInput.readLine();
                //    System.out.println(tempString);
                //    dataBuffer += tempString;
                eventsBuilder = NetworkEventContainer.parseFrom(socket.getInputStream());

                if (eventsBuilder != null) {
                    //eventsBuilder = NetworkEventContainer.newBuilder();
                    //eventsBuilder = NetworkEventContainer.parseFrom(dataBuffer.getBytes());
                    System.out.println("Event Received");
                    //NetworkEventContainer nc = eventsBuilder;
                    for (NetworkEvent e : eventsBuilder.getEventsList()) {
                        System.out.println("Event Received: " + e.getDescription() + "," + e.getTarget() + "," +
                                e.getValue() + "," + e.getDuration().getSeconds());
                    }
                    qosMessage = _IMM.startPriorityMechanism(eventsBuilder, null);
                }

                if (qosMessage != null) {
                    System.out.println("QoS message for NetProxy:");
                    for (MicroFlow microFlow: qosMessage.getMicroflowsList()) {
                        System.out.println("(" + microFlow.getId() + "," + microFlow.getIpSrc() + "," +
                                microFlow.getPortSrc() + "," + microFlow.getIpDst() + "," + microFlow.getPortDst() + "," +
                                microFlow.getProtocol() + "," + microFlow.getDSCP() + "," +
                                microFlow.getDuration().getSeconds() + ")");
                    }
                    System.out.println("Timestamp: " + qosMessage.getTimestamp());

                    Socket socketMarker = new Socket("localhost", 20001);

                    DataOutputStream outputStream = new DataOutputStream(socketMarker.getOutputStream());

                    byte[] message = qosMessage.toByteArray();
                    System.out.println("" + message.length);
                    outputStream.write(message);
                    System.out.println("QoS message sent");

                    socketMarker.close();
                }
                socket.close();
            }


        }
        catch (IOException e) {
            System.out.println("Error while creating server socket");
            return;
        }
    }

    private static void testCode ()
    {
        QoS qosMessage;
        Duration d;
        NetworkEventContainer.Builder events = NetworkEventContainer.newBuilder();
        d = Duration.newBuilder().setSeconds(15).build();
        events.addEvents(NetworkEvent.newBuilder().setDescription("CommonEvent").setTarget("skirmish")
                .setDuration(d).build());
        //d = Duration.newBuilder().setSeconds(25).build();
        //events.addEvents(NetworkEvent.newBuilder().setDescription("increase").setTarget("diagnostic")
        //        .setDuration(d).build());
        //d = Duration.newBuilder().setSeconds(Long.MAX_VALUE).build();
        //events.addEvents(NetworkEvent.newBuilder().setDescription("newLinkType").setTarget("1.1.1.1 2.2.2.2")
        //        .setValue("SATCOM").setDuration(d).build());
        //events.addEvents(NetworkEvent.newBuilder().setDescription("newLinkType").setTarget("3.3.3.3 4.4.4.4")
        //        .setValue("LAN").setDuration(d).build());
        //events.clear();
        System.out.println("here");
        qosMessage = _IMM.startPriorityMechanism(events.build(),null);
        System.out.println("here");
        if (qosMessage != null) {
            System.out.println("QoS message for NetProxy:");
            for (MicroFlow microFlow: qosMessage.getMicroflowsList()) {
                System.out.println("(" + microFlow.getId() + "," + microFlow.getIpSrc() + "," +
                        microFlow.getPortSrc() + "," + microFlow.getIpDst() + "," + microFlow.getPortDst() + "," +
                        microFlow.getProtocol() + "," + microFlow.getDSCP() + "," +
                        microFlow.getDuration().getSeconds() + ")");
            }
            System.out.println("Timestamp: " + qosMessage.getTimestamp());
        }

        events.clear();

        //d = Duration.newBuilder().setSeconds(21).build();
        //events.addEvents(NetworkEvent.newBuilder().setDescription("increase").setTarget("diagnostic")
        //        .setDuration(d).build());
        d = Duration.newBuilder().setSeconds(15).build();
        events.addEvents(NetworkEvent.newBuilder().setDescription("CommonEvent").setTarget("skirmish")
                .setDuration(d).build());
        //d = Duration.newBuilder().setSeconds(Long.MAX_VALUE).build();
        //events.addEvents(NetworkEvent.newBuilder().setDescription("newLinkType").setTarget("1.1.1.1 2.2.2.2")
        //        .setValue("LAN").setDuration(d).build());
        qosMessage = _IMM.startPriorityMechanism(events.build(),null);

        if (qosMessage != null) {
            System.out.println("QoS message for NetProxy:");
            for (MicroFlow microFlow: qosMessage.getMicroflowsList()) {
                System.out.println("(" + microFlow.getId() + "," + microFlow.getIpSrc() + "," +
                        microFlow.getPortSrc() + "," + microFlow.getIpDst() + "," + microFlow.getPortDst() + "," +
                        microFlow.getProtocol() + "," + microFlow.getDSCP() + "," +
                        microFlow.getDuration().getSeconds() + ")");
            }
            System.out.println("Timestamp: " + qosMessage.getTimestamp());
        }

        events.clear();

        //d = Duration.newBuilder().setSeconds(20).build();
        //events.addEvents(NetworkEvent.newBuilder().setDescription("increase").setTarget("diagnostic")
        //        .setDuration(d).build());
        d = Duration.newBuilder().setSeconds(Long.MAX_VALUE).build();
        events.addEvents(NetworkEvent.newBuilder().setDescription("newLinkType").setTarget("1.1.1.1 2.2.2.2")
                .setValue("LAN").setDuration(d).build());
        d = Duration.newBuilder().setSeconds(15).build();
        events.addEvents(NetworkEvent.newBuilder().setDescription("CommonEvent").setTarget("skirmish")
                .setDuration(d).build());
        qosMessage = _IMM.startPriorityMechanism(events.build(),null);

        if (qosMessage != null) {
            System.out.println("QoS message for NetProxy:");
            for (MicroFlow microFlow: qosMessage.getMicroflowsList()) {
                System.out.println("(" + microFlow.getId() + "," + microFlow.getIpSrc() + "," +
                        microFlow.getPortSrc() + "," + microFlow.getIpDst() + "," + microFlow.getPortDst() + "," +
                        microFlow.getProtocol() + "," + microFlow.getDSCP() + "," +
                        microFlow.getDuration().getSeconds() + ")");
            }
            System.out.println("Timestamp: " + qosMessage.getTimestamp());
        }

        //IMM.createInferenceTag("diagnostic");
        //IMM.createEvent(new NetworkEventWrapper("CommonEvent", "eventTest", null), events);
    }

    private static void createLinkType (String actionDiagnostic, String actionMission, String level) {
        Duration d = Duration.newBuilder().setSeconds(Long.MAX_VALUE).build();
        NetworkEvent event = NetworkEvent.newBuilder().setDescription(actionDiagnostic).setTarget("diagnostic")
                .setDuration(d).build();
        _eventsRelated.addEvents(event);
        event = NetworkEvent.newBuilder().setDescription(actionMission).setTarget("mission")
                .setDuration(d).setValue(level).build();
        _eventsRelated.addEvents(event);
    }

    private static NetworkEventContainer.Builder _eventsRelated;
    private static InferenceModuleManager _IMM;
}