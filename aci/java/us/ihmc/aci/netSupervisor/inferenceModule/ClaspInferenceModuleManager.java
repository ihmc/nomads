package us.ihmc.aci.netSupervisor.inferenceModule;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.TimeUtil;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.netSupervisor.traffic.WorldStateSummary;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * ClaspInferenceModuleManager.java
 * <p/>
 * Class <code>ClaspInferenceModuleManager</code> defines an API to the clasp module.
 *
 * @author Emanuele Tagliaferro (etagliaferro@ihmc.us)
 */

public class ClaspInferenceModuleManager implements InferenceModuleManager
{
    @Override
    public void init (String lpSolverPath)
    {
        _solverLauncher = new ClaspSolverLauncher();
        _solverLauncher.initSolver(lpSolverPath);

        _activeEvents = new ArrayList<>();
        _localFilters = new ArrayList<>();

        TagList tagList = readJsonTagListFile("../../aci/conf/netsupervisor/");

        if (tagList != null) {
            if (tagList.getQosTagsList() != null) {
                for (TaggedQoS taggedQoS : tagList.getQosTagsList()) {
                    createInferenceTag(taggedQoS.getTag(), taggedQoS.getQos());
                }
            }
        }

    }

    @Override
    public QoS startPriorityMechanism (NetworkEventContainer inputEvents, WorldStateSummary worldStateSummary)
    {
        List<ClaspTraffic> trafficPriorityResults;
        ClaspEvent tempClaspEvent;

        _localFilters.clear();
        for (NetworkEvent tempEvent : inputEvents.getEventsList()) {
            tempClaspEvent = new ClaspEvent(tempEvent);
            _activeEvents.add(tempClaspEvent);
            if (tempEvent.getDescription().equals("newLinkType")) {
                System.out.println("new link type description");
                //_solverLauncher.updateLinkTypeGroup(tempEvent.getTarget(), tempEvent.getValue());
                _localFilters.addAll(NetworkInformationFilter.addEventFilters(tempEvent.getTarget(),
                        tempClaspEvent.getEventName()));
            }
        }

        _networkContext = _solverLauncher.findEventsTargets(_activeEvents);

        for (ClaspGroup group: _networkContext) {
            System.out.println(group.print());
        }

        trafficPriorityResults = updateTrafficClassification(worldStateSummary);

        System.out.println("end");

        _activeEvents.clear();

        return getQoSMessage(trafficPriorityResults);
    }

    public void createInferenceTag (String tagName, QoS qosTrafficDescription)
    {
        List<ClaspTraffic> trafficEntries = new ArrayList<>();

        if (qosTrafficDescription != null) {
            if (qosTrafficDescription.getMicroflowsList() != null) {
                for (MicroFlow microFlow: qosTrafficDescription.getMicroflowsList()) {
                    ClaspTraffic claspTraffic = new ClaspTraffic(microFlow);
                    System.out.println(claspTraffic.print());
                    trafficEntries.add(claspTraffic);
                }
            }
        }

        if(!_solverLauncher.createClaspTag(tagName, trafficEntries)) {
            System.out.println("Error creating the new tag");
        }
    }

    public void createEvent (NetworkEvent event, NetworkEventContainer events)
    {
        List<ClaspEvent> claspEvents = new ArrayList<>();
        ClaspEvent newClaspEvent;

        if (events != null) {
            for (NetworkEvent tempEvent : events.getEventsList()) {
                if (tempEvent != null) {
                    claspEvents.add(new ClaspEvent(tempEvent));
                }
            }
        }

        newClaspEvent = new ClaspEvent(event);
        if (!_solverLauncher.createEvent(newClaspEvent, claspEvents)) {
            System.out.println("Error creating the new composite event");
        }
    }

    private List<ClaspTraffic> updateTrafficClassification (WorldStateSummary worldStateSummary)
    {
        List<ClaspTraffic> filteredTraffic, trafficPriorityResults;
        List<ClaspGroup> networkContextToFilter;

        if (_networkContext != null && _networkContext.size() != 0) {
            networkContextToFilter = new ArrayList<>();
            for (ClaspGroup claspGroup : _networkContext) {
                System.out.println("Main group: " + claspGroup.getMainGroup());
                System.out.println("Sub-group: " + claspGroup.getSubGroup());

                if (!claspGroup.getMainGroup().equals(_tagString)) {
                    networkContextToFilter.add(claspGroup);
                }
            }

            if (networkContextToFilter.size() != 0) {
                _localFilters.addAll(NetworkInformationFilter.hostInformationFilter(networkContextToFilter,
                        worldStateSummary));
                filteredTraffic = NetworkInformationFilter.trafficFilter(networkContextToFilter, worldStateSummary);
            }
            else {
                filteredTraffic = new ArrayList<>();
            }

            trafficPriorityResults = _solverLauncher.calculateNewPriorityClassification(filteredTraffic, _localFilters);

            for (ClaspTraffic trafficEntry : trafficPriorityResults) {
                System.out.println("From " + trafficEntry.getSourceHostIp() + ":" + trafficEntry.getSourceHostPort());
                System.out.println("To " + trafficEntry.getDestinationHostIp() + ":" +
                        trafficEntry.getDestinationHostPort());
                System.out.println("Priority value = " + trafficEntry.getPriorityValue() + "\n");
            }

            return trafficPriorityResults;
        }

        return null;
    }

    public NetworkEventContainer updatePriorityClassification (NetworkEventContainer events)
    {
        List<ClaspAction> solverActions;
        List<ClaspGroup> networkContext;
        NetworkEventContainer.Builder outgoingActions;

        List<ClaspEvent> inputClaspEvents = new ArrayList<>();

        for (NetworkEvent tempEvent : events.getEventsList()) {
            inputClaspEvents.add(new ClaspEvent(tempEvent));
        }

        networkContext = _solverLauncher.findEventsTargets(inputClaspEvents);
        solverActions = _solverLauncher.getAllCurrentActions();

        outgoingActions = NetworkEventContainer.newBuilder();

        for (ClaspAction action : solverActions) {
            for (ClaspGroup group : networkContext) {
                if (group.getGroupId() == action.getGroupId()) {
                    //outgoingActions.add(actionAndGroupToNetworkEvent(action, group));
                }
            }
        }

        return outgoingActions.build();
    }

    private TagList readJsonTagListFile (String filePath)
    {
        BufferedReader br;
        try {
            FileReader fr = new FileReader(filePath + _configFile);
            br = new BufferedReader(fr);
        }
        catch(Exception e) {
            System.out.println("The file doesn't exist.");
            return null;
        }
        String output = "";
        String newline = "";
        try {
            newline = br.readLine();
            while(newline != null) {
                output = output + newline;
                newline = br.readLine();
            }
        }
        catch (IOException e) {
            System.out.println("End of file");
        }

        if(!output.equals("")) {
            try {
                TagList.Builder tagList = TagList.newBuilder();
                JsonFormat.parser().merge(output, tagList);
                return tagList.build();
            }
            catch (InvalidProtocolBufferException e) {
                System.out.println("Error during the parsing from Json...");
                e.printStackTrace();
            }
        }
        else {
            System.out.println("Error reading json file.");
        }

        return null;
    }

    private QoS getQoSMessage (List<ClaspTraffic> claspTrafficList)
    {
        QoS.Builder qosBuilder = QoS.newBuilder();
        MicroFlow microflow;
        int i = 0;

        if (claspTrafficList != null) {
            for (ClaspTraffic claspTraffic: claspTrafficList) {
                microflow = claspTraffic.getMicroFlowFromClaspTraffic();
                if (microflow != null) {
                    qosBuilder.addMicroflows(microflow);
                    i++;
                }
            }
            qosBuilder.setTimestamp(TimeUtil.getCurrentTime());
        }

        return qosBuilder.build();
    }

    private List<ClaspEvent> _activeEvents;
    private SolverLauncher _solverLauncher;
    private List<ClaspGroup> _networkContext;
    private List<ClaspHostDescription> _localFilters;
    private final String _tagString = "tag";
    private final String _configFile = "qosConfigFile.json";
}
