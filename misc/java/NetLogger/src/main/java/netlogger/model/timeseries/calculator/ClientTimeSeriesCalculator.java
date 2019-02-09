package netlogger.model.timeseries.calculator;

import netlogger.model.timeseries.DefaultColumnNames;
import netlogger.model.timeseries.TimeStatistic;
import netlogger.model.timeseries.TrackingMeasureTimeSeriesItem;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ClientTimeSeriesCalculator
{
    public ClientTimeSeriesCalculator (String clientID, List<TrackingMeasureTimeSeriesItem> data) {
        _clientID = clientID;
        _data = data;

        _timeCalculator = new TimeCalculator(_data);
        _statisticCalculator = new StatisticCalculator(_data);
        _countCalculator = new CountCalculator(_data);
    }

    public Long calculateTimeSinceLastReceive () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, _clientID);
        return _timeCalculator.getTimeSinceLastItemWithFilter(filterValues);
    }

    public Long calculateTimeSinceLastSend () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, _clientID);

        return _timeCalculator.getTimeSinceLastItemWithFilter(filterValues);
    }

    public TimeStatistic calculateMinTimeBetweenReceives () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, _clientID);
        return _timeCalculator.getMinTimeBetweenItemsWithFilters(filterValues);
    }

    public TimeStatistic calculateMinTimeBetweenSends () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, _clientID);
        return _timeCalculator.getMinTimeBetweenItemsWithFilters(filterValues);
    }

    public String calculateLeastReceivedFrom () {
        return _statisticCalculator.getSourceLeastReceivedFrom(_clientID);
    }

    public String calculateLeastReceivedType () {
        return _statisticCalculator.getLeastReceivedType(_clientID);
    }

    public String calculateLeastSentTo () {
        return _statisticCalculator.getDestLeastSentTo(_clientID);
    }

    public String calculateLeastSentType () {
        return _statisticCalculator.getLeastSentType(_clientID);
    }

    public String calculateMostReceivedType () {
        return _statisticCalculator.getMostReceivedType(_clientID);
    }

    public String calculateMostSentTo () {
        return _statisticCalculator.getDestMostSentTo(_clientID);
    }

    public String calculateMostSentType () {
        return _statisticCalculator.getMostSentType(_clientID);
    }

    /**
     * Generates a map that uses a "source_client_id" with the passed in value to filter the search results. A sent tracked
     * measure is stored as the source client id being the one that sent the data.
     *
     * @return Count of the total sent by the client
     */
    public int calculateTotalSent () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, _clientID);
        return _countCalculator.calculateTotalWithFilters(filterValues);
    }

    /**
     * Generates a map that uses a "dest_client_id" with the passed in value to filter the search results. A received tracked
     * measure is stored as the dest client id being the one that received the data.
     *
     * @return count of the total received by the client
     */
    public int calculateTotalReceived () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, _clientID);
        return _countCalculator.calculateTotalWithFilters(filterValues);
    }

    /**
     * Generates a map that uses the dest client ID string with the passed in value to filter the search results. The
     * time calculator then finds the longest time between indices.
     *
     * @return Statistic for the time between receives
     */
    public TimeStatistic calculateLongestTimeBetweenReceives () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, _clientID);
        return _timeCalculator.getMaxTimeBetweenItemsWithFilters(filterValues);
    }

    /**
     * Generates a map that uses the source client ID string with the passed in value to filter the search results. The
     * time calculator then finds the longest time between indices.
     *
     * @return Statistic for the time between sends
     */
    public TimeStatistic calculateLongestTimeBetweenSends () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, _clientID);
        return _timeCalculator.getMaxTimeBetweenItemsWithFilters(filterValues);
    }

    /**
     * Generates a map that uses the source client ID string with the passed in value to filter the search results. The
     * time calculator then finds the shortest time between indices.
     *
     * @return Statistic for the time between sends
     */
    public TimeStatistic calculateSmallestTimeBetweenSends () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.SOURCE_CLIENT_ID_STRING, _clientID);
        return _timeCalculator.getMinTimeBetweenItemsWithFilters(filterValues);
    }

    /**
     * Generates a map that uses the source client ID string with the passed in value to filter the search results. The
     * time calculator then finds the shortest time between indices.
     *
     * @return Statistic for the time between sends
     */
    public TimeStatistic calculateSmallestTimeBetweenReceives () {
        Map<String, String> filterValues = new HashMap<>();
        filterValues.put(DefaultColumnNames.DEST_CLIENT_ID_STRING, _clientID);
        return _timeCalculator.getMinTimeBetweenItemsWithFilters(filterValues);
    }

    public String calculateMostReceivedFrom () {
        return _statisticCalculator.getSourceMostReceivedFrom(_clientID);
    }


    private TimeCalculator _timeCalculator;
    private StatisticCalculator _statisticCalculator;
    private CountCalculator _countCalculator;
    private final String _clientID;
    private final List<TrackingMeasureTimeSeriesItem> _data;
    private static final Logger _logger = LoggerFactory.getLogger(ClientTimeSeriesCalculator.class);
}
