package netlogger.model.timeseries.calculator;

import netlogger.model.timeseries.DefaultColumnNames;
import netlogger.model.timeseries.TrackingMeasureTimeSeriesItem;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import java.util.stream.Collectors;

public class StatisticCalculator
{
    public StatisticCalculator (List<TrackingMeasureTimeSeriesItem> data) {
        _data = data;
    }


    private Map<String, List<TrackingMeasureTimeSeriesItem>> splitSeriesIntoMapByColumn (String columnName) {
        // Split the data into a map based on type after filtering data by being sent
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapByType = new HashMap<>();
        List<TrackingMeasureTimeSeriesItem> copy = new ArrayList<>(_data);

        for (TrackingMeasureTimeSeriesItem item : copy) {
            String dataType = item.getColumnValue(columnName);
            List<TrackingMeasureTimeSeriesItem> itemsForType = mapByType.computeIfAbsent(dataType, k -> new ArrayList<>());

            itemsForType.add(item);
        }

        // This isn't valid. Remove it. Can happen if Federation doesn't have the type of data yet.
        mapByType.remove("null");

        return mapByType;
    }

    private Map<String, List<TrackingMeasureTimeSeriesItem>> filterListsOfMap (Map<String, List<TrackingMeasureTimeSeriesItem>> mapToFilter,
                                                                               Map<String, String> filterByMap) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByType = new HashMap<>(mapToFilter);

        for (String column : filterByMap.keySet()) {
            String columnValue = filterByMap.get(column);

            Map<String, List<TrackingMeasureTimeSeriesItem>> tempMap = new HashMap<>(filteredMapByType);

            for (String type : tempMap.keySet()) {
                List<TrackingMeasureTimeSeriesItem> filteredList = tempMap.get(type)
                        .stream()
                        .filter(item -> item.getColumnValue(column).equals(columnValue))
                        .collect(Collectors.toList());

                filteredMapByType.put(type, filteredList);
            }
        }

        return filteredMapByType;
    }

    private String getLeast (Map<String, List<TrackingMeasureTimeSeriesItem>> map) {
        AtomicInteger smallestCount = new AtomicInteger(Integer.MAX_VALUE);
        AtomicReference<String> type = new AtomicReference<>();

        map.forEach((s, trackingMeasureTimeSeriesItems) -> {
            if (smallestCount.get() > trackingMeasureTimeSeriesItems.size()) {
                smallestCount.set(trackingMeasureTimeSeriesItems.size());
                type.set(s);
            }
        });

        return type.get();
    }

    private String getMost (Map<String, List<TrackingMeasureTimeSeriesItem>> map) {

        AtomicInteger largestCount = new AtomicInteger(0);
        AtomicReference<String> type = new AtomicReference<>();

        map.forEach((s, trackingMeasureTimeSeriesItems) -> {
            if (largestCount.get() < trackingMeasureTimeSeriesItems.size()) {
                largestCount.set(trackingMeasureTimeSeriesItems.size());
                type.set(s);
            }
        });

        return type.get();
    }


    public String getLeastSentType (String src) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapByType = splitSeriesIntoMapByColumn(DefaultColumnNames.DATA_TYPE_STRING);

        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, src);

        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByType = filterListsOfMap(mapByType, filterValues);

        return getLeast(filteredMapByType);
    }

    public String getMostSentType (String src) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapByType = splitSeriesIntoMapByColumn(DefaultColumnNames.DATA_TYPE_STRING);

        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, src);

        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByType = filterListsOfMap(mapByType, filterValues);

        return getMost(filteredMapByType);
    }

    public String getLeastReceivedType (String dest) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapByType = splitSeriesIntoMapByColumn(DefaultColumnNames.DATA_TYPE_STRING);

        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, dest);

        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByType = filterListsOfMap(mapByType, filterValues);

        return getLeast(filteredMapByType);
    }

    public String getMostReceivedType (String dest) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapByType = splitSeriesIntoMapByColumn(DefaultColumnNames.DATA_TYPE_STRING);

        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, dest);

        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByType = filterListsOfMap(mapByType, filterValues);

        return getMost(filteredMapByType);
    }

    public String getSourceLeastReceivedFrom (String dest) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapBySource = splitSeriesIntoMapByColumn(DefaultColumnNames.SOURCE_CLIENT_ID_STRING);

        // Remove ourselves from the map because a client cannot send to itself
        mapBySource.remove(dest);

        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, dest);

        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByDest = filterListsOfMap(mapBySource, filterValues);

        return getLeast(filteredMapByDest);
    }

    public String getSourceMostReceivedFrom (String dest) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapBySource = splitSeriesIntoMapByColumn(DefaultColumnNames.SOURCE_CLIENT_ID_STRING);


        // Remove ourselves from the map because a client cannot send to itself
        mapBySource.remove(dest);

        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, dest);

        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByDest = filterListsOfMap(mapBySource, filterValues);

        return getMost(filteredMapByDest);
    }

    public String getDestLeastSentTo (String src) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapByDest = splitSeriesIntoMapByColumn(DefaultColumnNames.DEST_CLIENT_ID_STRING);

        // Remove ourselves from the map because a client cannot send to itself
        mapByDest.remove(src);

        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, src);

        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByDest = filterListsOfMap(mapByDest, filterValues);

        return getLeast(filteredMapByDest);
    }

    public String getDestMostSentTo (String src) {
        Map<String, List<TrackingMeasureTimeSeriesItem>> mapByDest = splitSeriesIntoMapByColumn(DefaultColumnNames.DEST_CLIENT_ID_STRING);

        // Remove ourselves from the map because a client cannot send to itself
        mapByDest.remove(src);

        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, src);

        Map<String, List<TrackingMeasureTimeSeriesItem>> filteredMapByDest = filterListsOfMap(mapByDest, filterValues);

        return getMost(filteredMapByDest);
    }

    private List<TrackingMeasureTimeSeriesItem> _data;
    private static final Logger _logger = LoggerFactory.getLogger(StatisticCalculator.class);
}
