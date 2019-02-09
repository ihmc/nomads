package us.ihmc.aci.netSupervisor.inferenceModule;

import us.ihmc.aci.ddam.NetworkEvent;

import java.util.*;

public class ClaspSolverLauncher implements SolverLauncher
{
    static
    {
        System.loadLibrary("claspsolverjni");
    }

    private native ClaspAction[] initAction (ClaspEvent[] eventString);
    private native ClaspGroup[] getCurrentActionTargets ();
    private native ClaspTraffic[] computeNewPriorityStatus (ClaspTraffic[] inputTraffic,
                                                ClaspHostDescription[] inputHostDescriptionArray);
    private native boolean initClaspModule (String lpSourcesPath);
    private native int createGroup (ClaspGroup group);
    private native boolean createAction (ClaspAction action);
    private native boolean createTraffic (ClaspTraffic[] trafficEntries, int groupId);
    private native boolean createEvent (ClaspEvent event, ClaspEvent[] associatedEvents);
    private native boolean updateGroupDescription (ClaspGroup group);

    public boolean initSolver(String lpSourcesPath)
    {
        if (!initClaspModule(lpSourcesPath)) {
            System.out.println("Error initializing the clasp solver.");
            return false;
        }

        if (_targetsGroups == null) {
            _targetsGroups = new ArrayList<>();
        }

        if (_updatedTraffic == null) {
            _updatedTraffic = new ArrayList<>();
        }

        if (_inferenceAction == null) {
            _inferenceAction = new ArrayList<>();
        }

        if (_linkTypeGroups == null) {
            _linkTypeGroups = new HashMap<>();
        }

        return true;
    }

    public List<ClaspGroup> findEventsTargets(List<ClaspEvent> events)
    {
        ClaspEvent[] eventArray = new ClaspEvent[events.size()];
        eventArray = events.toArray(eventArray);
        long startTime = System.currentTimeMillis();
        ClaspAction[] actionsArray = initAction(eventArray);
        ClaspGroup[] targetsGroupsArray = getCurrentActionTargets();
        long endTime = System.currentTimeMillis();

        long duration = (endTime - startTime);
        System.out.println("Method initAction duration: " + duration);
        _targetsGroups.clear();
        _inferenceAction.clear();

        for(int i = 0; i <  targetsGroupsArray.length; i++){
            System.out.println("Group id : " +  targetsGroupsArray[i].getGroupId());
            System.out.println("Group description : " +  targetsGroupsArray[i].getGroupName());
            System.out.println("Sub-group : " +  targetsGroupsArray[i].getSubGroup());
            System.out.println("Main group : " +  targetsGroupsArray[i].getMainGroup());
            _targetsGroups.add(targetsGroupsArray[i]);
        }

        for(int i = 0; i <  actionsArray.length; i++){
            System.out.println("Action : " + actionsArray[i].getActionName());
            System.out.print(actionsArray[i].getTypeOfAction());
            System.out.print(" of " + actionsArray[i].getPriorityValue());
            System.out.println(" on " + actionsArray[i].getGroupId());
            _inferenceAction.add(actionsArray[i]);
        }

        return _targetsGroups;
    }

    public List<ClaspTraffic> calculateNewPriorityClassification (List<ClaspTraffic> inputTraffic,
                                                                  List<ClaspHostDescription> inputHostsDescription)
    {
        ClaspTraffic[] inputTrafficArray = new ClaspTraffic[inputTraffic.size()];
        inputTrafficArray = inputTraffic.toArray(inputTrafficArray);

        ClaspHostDescription[] inputHostDescriptionArray = new ClaspHostDescription[inputHostsDescription.size()];
        inputHostDescriptionArray = inputHostsDescription.toArray(inputHostDescriptionArray);

        long startTime = System.currentTimeMillis();
        ClaspTraffic[] updatedTrafficArray = computeNewPriorityStatus(inputTrafficArray, inputHostDescriptionArray);
        long endTime = System.currentTimeMillis();

        long duration = (endTime - startTime);
        System.out.println("Method computeNewPriorityStatus duration: " + duration);

        _updatedTraffic.clear();

        for (int i = 0; i < updatedTrafficArray.length; i++) {
            _updatedTraffic.add(updatedTrafficArray[i]);
        }

        return _updatedTraffic;
    }

    public boolean createClaspTag (String tagName, List<ClaspTraffic> relatedTraffic)
    {
        ClaspGroup eventGroup;
        ClaspTraffic[] trafficArray;

        int groupId;

        eventGroup = new ClaspGroup();
        eventGroup.setGroupName("all");
        eventGroup.setSubGroup(tagName);
        eventGroup.setMainGroup("tag");

        groupId = createGroup(eventGroup);
        System.out.println("Id group:" + groupId);
        if (groupId < 0) {
            System.out.println("Error creating group " + eventGroup.print());
            return false;
        }

        eventGroup.setGroupId(groupId);
        _linkTypeGroups.put(tagName,eventGroup);

        if (!createAllActions(tagName, groupId)) {
            System.out.println("Error creating actions for the tag" + tagName);
            return false;
        }

        if (relatedTraffic != null) {
            trafficArray = new ClaspTraffic[relatedTraffic.size()];
            trafficArray = relatedTraffic.toArray(trafficArray);
            if (!createTraffic(trafficArray, groupId)) {
                System.out.println("Error creating the related traffic for the tag " + tagName);
                return false;
            }
        }

        return true;
    }

    public boolean createEvent (ClaspEvent event, List<ClaspEvent> relatedEvents)
    {
        ClaspEvent[] relatedEventArray = new ClaspEvent[relatedEvents.size()];
        int i = 0;

        for (ClaspEvent claspEvent : relatedEvents) {
            System.out.println(claspEvent.getEventName());

            relatedEventArray[i] = new ClaspEvent();
            relatedEventArray[i].setEventName(claspEvent.getEventName());
            relatedEventArray[i].setEventValue(claspEvent.getEventValue());
            relatedEventArray[i].setDuration(claspEvent.getDuration());
            i++;
        }
        if (event != null) {
            if (!createEvent(event, relatedEventArray)) {
                System.out.println("Error creating the composite event " + event.getEventName());
            }
        }
        return true;
    }

    public boolean updateLinkTypeGroup (String ipList, String linkType)
    {
        String tempString;
        ClaspGroup group = _linkTypeGroups.get(linkType);
        ClaspGroup diagnosticGroup = _linkTypeGroups.get("diagnostic");
        if (group != null && diagnosticGroup != null) {
            StringTokenizer stringTokenizer = new StringTokenizer(ipList);

            while (stringTokenizer.hasMoreTokens()) {
                tempString = stringTokenizer.nextToken();
                group.setGroupName(tempString);
                System.out.println("new description:" + group.getGroupName());
                if (!updateGroupDescription(group)) {
                    System.out.println("Error creating group description " + group.print());
                    return false;
                }
                diagnosticGroup.setGroupName(tempString);
                if (!updateGroupDescription(diagnosticGroup)) {
                    System.out.println("Error creating group description " + diagnosticGroup.print());
                    return false;
                }
            }
        }

        return true;
    }

    public List<ClaspAction> getAllCurrentActions ()
    {
        if (_inferenceAction == null) {
            _inferenceAction = new ArrayList<>();
            _inferenceAction.clear();
        }
        return _inferenceAction;
    }

    public boolean clearAll() {
        if (_targetsGroups != null) {
            _targetsGroups.clear();
        }
        if(_updatedTraffic != null) {
            _updatedTraffic.clear();
        }
        if(_inferenceAction != null) {
            _inferenceAction.clear();
        }
        return true;
    }

    private boolean createAllActions (String tagName, int groupId)
    {
        ClaspAction eventAction = new ClaspAction();
        eventAction.setDefaultForType(1, tagName, groupId);

        if (!createAction(eventAction)) {
            System.out.println("Error creating increase action");
            return false;
        }

        eventAction.setDefaultForType(2, tagName, groupId);

        if (!createAction(eventAction)) {
            System.out.println("Error creating decrease action");
            return false;
        }

        eventAction.setDefaultForType(3, tagName, groupId);

        if (!createAction(eventAction)) {
            System.out.println("Error creating change action");
            return false;
        }
        return true;
    }

    private List<ClaspTraffic> _updatedTraffic;
    private List<ClaspGroup> _targetsGroups;
    private List<ClaspAction> _inferenceAction;
    private Map<String,ClaspGroup> _linkTypeGroups;
}