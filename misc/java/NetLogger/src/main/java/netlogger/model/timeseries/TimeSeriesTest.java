package netlogger.model.timeseries;

import netlogger.controller.tracking.ClientStatisticsManager;
import netlogger.model.tracking.statistics.GeneralStatistics;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
import java.util.concurrent.ThreadLocalRandom;

public class TimeSeriesTest
{
    public static void main (String[] args) {
        TrackingMeasureTimeSeries timeSeries = new TrackingMeasureTimeSeries();
        ClientStatisticsManager clientStatisticsManager = new ClientStatisticsManager("FigGateway");
        clientStatisticsManager.setSeries(timeSeries);

        applications = new HashMap<>();

        List<String> fedClientIds = new ArrayList<>(Arrays.asList("FigGateway", "ManatimGateway"));

        applications.put("Federation", fedClientIds);

        long time = System.currentTimeMillis();

        TrackingMeasureTimeSeriesItem item1 = createReceivedType(System.currentTimeMillis(), "ManatimGateway", "Track");
        TrackingMeasureTimeSeriesItem item2 = createReceivedType(System.currentTimeMillis(), "ManatimGateway", "Track");
        TrackingMeasureTimeSeriesItem item3 = createReceivedType(System.currentTimeMillis(), "ManatimGateway", "Track");
        TrackingMeasureTimeSeriesItem item4 = createReceivedType(System.currentTimeMillis(), "ManatimGateway", "COT");
        TrackingMeasureTimeSeriesItem item5 = createReceivedType(System.currentTimeMillis(), "ManatimGateway", "COT");
        TrackingMeasureTimeSeriesItem item6 = createSentType(System.currentTimeMillis(), "ManatimGateway", "MissionPackage");
        TrackingMeasureTimeSeriesItem item7 = createSentType(System.currentTimeMillis(), "ManatimGateway", "MissionPackage");

        TrackingMeasureTimeSeriesItem item8 = createSentType(System.currentTimeMillis(), "AFSOC", "MissionPackage");
        TrackingMeasureTimeSeriesItem item9 = createSentType(System.currentTimeMillis(), "AFSOC", "Track");
        TrackingMeasureTimeSeriesItem item10 = createSentType(System.currentTimeMillis(), "AFSOC", "Track");
        TrackingMeasureTimeSeriesItem item11 = createReceivedType(System.currentTimeMillis(), "AFSOC", "Track");

        timeSeries.add(item1);
        timeSeries.add(item2);
        timeSeries.add(item3);
        timeSeries.add(item4);
        timeSeries.add(item5);
        timeSeries.add(item6);
        timeSeries.add(item7);

        timeSeries.add(item8);
        timeSeries.add(item9);
        timeSeries.add(item10);
        timeSeries.add(item11);

        GeneralStatistics stats = clientStatisticsManager.calculateGeneralStatistics();
    }

    private static TrackingMeasureTimeSeriesItem createRandomItem (long currTime) {
        HashMap<String, String> map = new HashMap<>();

        String randomSourceApplication = (String) applications.keySet().toArray()[ThreadLocalRandom.current().nextInt(0, applications.size())];
        List<String> idsForSourceApp = applications.get(randomSourceApplication);
        String randomSourceID = idsForSourceApp.get(ThreadLocalRandom.current().nextInt(0, idsForSourceApp.size()));

        String randomDestApplication = (String) applications.keySet().toArray()[ThreadLocalRandom.current().nextInt(0, applications.size())];
        List<String> idsForDestApp = applications.get(randomSourceApplication);
        String randomDestID = idsForDestApp.get(ThreadLocalRandom.current().nextInt(0, idsForDestApp.size()));

        map.put(DefaultColumnNames.SOURCE_APPLICATION_STRING, randomSourceApplication);
        map.put(DefaultColumnNames.SOURCE_MODULE_STRING, randomSourceApplication);
        map.put(DefaultColumnNames.DEST_APPLICATION_STRING, randomDestApplication);
        map.put(DefaultColumnNames.DEST_MODULE_STRING, randomDestApplication);
        map.put(DefaultColumnNames.PREV_DATA_ID_STRING, "1");
        map.put(DefaultColumnNames.CURR_DATA_ID_STRING, "2");
        map.put(DefaultColumnNames.CHECKSUM_STRING, "f");
        map.put(DefaultColumnNames.DATA_TYPE_STRING, types[ThreadLocalRandom.current().nextInt(0, types.length)]);
        map.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, randomSourceID);
        map.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, randomDestID);

        return new TrackingMeasureTimeSeriesItem(map, currTime);
    }

    private static TrackingMeasureTimeSeriesItem createReceivedType (long currTime, String sender, String type) {
        HashMap<String, String> map = new HashMap<>();

        map.put(DefaultColumnNames.SOURCE_APPLICATION_STRING, "Federation");
        map.put(DefaultColumnNames.SOURCE_MODULE_STRING, "Federation");
        map.put(DefaultColumnNames.DEST_APPLICATION_STRING, "Federation");
        map.put(DefaultColumnNames.DEST_MODULE_STRING, "Federation");
        map.put(DefaultColumnNames.PREV_DATA_ID_STRING, "1");
        map.put(DefaultColumnNames.CURR_DATA_ID_STRING, "2");
        map.put(DefaultColumnNames.CHECKSUM_STRING, "f");
        map.put(DefaultColumnNames.DATA_TYPE_STRING, type);
        map.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, sender);
        map.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, "FigGateway");

        return new TrackingMeasureTimeSeriesItem(map, currTime);
    }

    private static TrackingMeasureTimeSeriesItem createSentType (long currTime, String receiver, String type) {
        HashMap<String, String> map = new HashMap<>();

        map.put(DefaultColumnNames.SOURCE_APPLICATION_STRING, "Federation");
        map.put(DefaultColumnNames.SOURCE_MODULE_STRING, "Federation");
        map.put(DefaultColumnNames.DEST_APPLICATION_STRING, "Federation");
        map.put(DefaultColumnNames.DEST_MODULE_STRING, "Federation");
        map.put(DefaultColumnNames.PREV_DATA_ID_STRING, "1");
        map.put(DefaultColumnNames.CURR_DATA_ID_STRING, "2");
        map.put(DefaultColumnNames.CHECKSUM_STRING, "f");
        map.put(DefaultColumnNames.DATA_TYPE_STRING, type);
        map.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, "FigGateway");
        map.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, receiver);

        return new TrackingMeasureTimeSeriesItem(map, currTime);
    }


    private static Map<String, List<String>> applications;
    private static String[] types = {"Track", "MissionPackage"};

    private static final Logger _logger = LoggerFactory.getLogger(TimeSeriesTest.class);
}
