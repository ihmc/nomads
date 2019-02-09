package us.ihmc.aci.netSupervisor.inferenceModule;

import us.ihmc.aci.netSupervisor.traffic.WorldStateSummary;

import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

/**
 * NetworkInformationFilter.java
 * <p/>
 * Static Class <code>NetworkInformationFilter</code> used as a utility to filter different network object.
 *
 * @author Emanuele Tagliaferro (etagliaferro@ihmc.us)
 */

public class NetworkInformationFilter
{

    /**
     * Looking on the targets of the events, filters the traffic on the World State Summary
     *
     * @return false in case of error
     */
    public static List<ClaspTraffic> trafficFilter (List<ClaspGroup> targetGroups, WorldStateSummary worldStateSummary)
    {
        boolean addAll = false;

        if (_filteredTrafficArray == null) {
            _filteredTrafficArray = new ArrayList<>();
        }
        else {
            _filteredTrafficArray.clear();
        }
        for (ClaspGroup group: targetGroups) {
            switch (group.getMainGroup()) {
                case "link":
                    addLinkTrafficEntries(targetGroups, group);
                    break;
                default:
                    addAll = true;
            }
        }

        if (addAll) {
            addAllTrafficEntries(worldStateSummary);
        }

        /*
        for (int i = 1; i < 10; i++) {
            filteredTrafficArray.add(new ClaspTraffic("128.49.235.105", i, "128.49.235.106", i+1, "UDP", 2));
        }
        for (int i = 1; i < 10; i++) {
            filteredTrafficArray.add(new ClaspTraffic("128.49.235.105", i, "128.49.235.249", i+1, "UDP", 2));
        }

        for (int i = 1; i < 10; i++) {
            filteredTrafficArray.add(new ClaspTraffic("128.49.235.103", i, "128.49.235.211", i+1, "UDP", 2));
        }
*/
        return _filteredTrafficArray;
    }

    /**
     * Looking on the targets of the events, filters the hosts information on the World State Summary
     *
     * @return false in case of error
     */
    public static List<ClaspHostDescription> hostInformationFilter (List<ClaspGroup> targetGroups,
                                                                    WorldStateSummary worldStateSummary)
    {
        List<ClaspHostDescription> filteredHostDescriptionArray = new ArrayList<>();

        filteredHostDescriptionArray.add(new ClaspHostDescription("128.49.235.100", "internal", ""));
        filteredHostDescriptionArray.add(new ClaspHostDescription("128.49.235.249", "internal", ""));
        filteredHostDescriptionArray.add(new ClaspHostDescription("128.49.235.103", "diagnosticType", ""));
        filteredHostDescriptionArray.add(new ClaspHostDescription("128.49.235.104", "transport", ""));
        filteredHostDescriptionArray.add(new ClaspHostDescription("128.49.235.120", "nodemonitor", ""));

        return filteredHostDescriptionArray;
    }

    public static List<ClaspHostDescription> addEventFilters (String targetsRecord, String eventId) {

        List<ClaspHostDescription> filteredHostDescriptionArray = new ArrayList<>();
        String tempString;

        if (!targetsRecord.equals("")) {
            StringTokenizer stringTokenizer = new StringTokenizer(targetsRecord);

            while (stringTokenizer.hasMoreTokens()) {
                tempString = stringTokenizer.nextToken();
                filteredHostDescriptionArray.add(new ClaspHostDescription(tempString, "", eventId));
            }
        }

        return filteredHostDescriptionArray;
    }

    private static void addAllTrafficEntries (WorldStateSummary worldStateSummary)
    {
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.100", "23", "128.49.235.249", "7999", "udp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.100", "23", "128.49.235.250", "24", "tcp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.100", "100", "128.49.235.250", "101", "tcp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.109", "23", "128.49.235.250", "24", "tcp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.103", "23", "128.49.235.104", "24", "tcp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.103", "23", "128.49.235.106", "24", "tcp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.103", "23", "128.49.235.108", "24", "tcp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.105", "23", "128.49.235.249", "120", "udp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.120", "5000", "128.49.235.106", "101", "tcp", 0));
        _filteredTrafficArray.add(new ClaspTraffic(0,"128.49.235.105", "23", "128.49.235.120", "5000", "tcp", 0));
    }

    private static void addLinkTrafficEntries (List<ClaspGroup> targetGroups, ClaspGroup currentGroup)
    {
        for (ClaspGroup linkGroup: targetGroups) {
            if (linkGroup.getMainGroup().equals("link") && currentGroup.getGroupId() == linkGroup.getGroupId() &&
                    !linkGroup.getGroupName().equals(currentGroup.getGroupName())) {
                _filteredTrafficArray.add(new ClaspTraffic(0, currentGroup.getGroupName(), "*",
                        linkGroup.getGroupName(), "10000-15000", "tcp", 0));
                //_filteredTrafficArray.add(new ClaspTraffic(0, currentGroup.getGroupName(), "*",
                //        linkGroup.getGroupName(), "*", "tcp", 0));
            }
        }
    }

    private static List<ClaspTraffic> _filteredTrafficArray;
}
