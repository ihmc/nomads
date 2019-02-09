package netlogger.controller.tracking;

import javafx.application.Platform;
import javafx.scene.control.Tooltip;
import netlogger.model.timeseries.calculator.ClientTimeSeriesCalculator;
import netlogger.model.tracking.statistics.GeneralStatistics;
import netlogger.model.tracking.statistics.TrackingStatistics;
import netlogger.model.tracking.statistics.builders.GeneralStatisticBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ClientStatisticsManager extends StatisticsManager implements Runnable
{
    public ClientStatisticsManager (String clientID) {
        _clientID = clientID;
    }

    @Override
    public void run () {
        while (!_terminationRequested) {
            if (!_canRun) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                continue;
            }

            long start = System.currentTimeMillis();
            GeneralStatistics newStat = calculateGeneralStatistics();
            long end = System.currentTimeMillis();

            long dur = end - start;

            if (dur > 1000) {
                _logger.error("Calculation for {} took {}", _clientID, dur);
            }

            TrackingStatistics statistics = new TrackingStatistics(newStat, null, null);
            String text = statistics.toString();

            Platform.runLater(() -> {
                _toolTip.setText(text);
            });

            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void setTooltip (Tooltip tooltip) {
        _toolTip = tooltip;
    }

    public void requestTermination () {
        _terminationRequested = true;
    }

    public void setCanRun (boolean canRun) {
        _canRun = canRun;
    }


    public GeneralStatistics calculateGeneralStatistics () {
        ClientTimeSeriesCalculator calculator = new ClientTimeSeriesCalculator(_clientID, _series.getItems());

        int totalSent = calculator.calculateTotalSent();
        int totalReceived = calculator.calculateTotalReceived();

        Long lastReceivedTime = calculator.calculateTimeSinceLastReceive();
        Long lastSentTime = calculator.calculateTimeSinceLastSend();

        String leastReceivedFrom = calculator.calculateLeastReceivedFrom();
        String mostReceivedFrom = calculator.calculateMostReceivedFrom();
        String mostSentTo = calculator.calculateMostSentTo();
        String leastSentTo = calculator.calculateLeastSentTo();

        String leastReceivedType = calculator.calculateLeastReceivedType();
        String mostReceivedType = calculator.calculateMostReceivedType();
        String mostSentType = calculator.calculateMostSentType();
        String leastSentType = calculator.calculateLeastSentType();

        return new GeneralStatisticBuilder()
                .setTotalSent(totalSent)
                .setTotalReceived(totalReceived)
                .setLastReceived(String.valueOf(lastReceivedTime))
                .setLastSent(String.valueOf(lastSentTime))
                .setLeastReceivedFromClient(leastReceivedFrom)
                .setMostReceivedFromClient(mostReceivedFrom)
                .setMostSentToClient(mostSentTo)
                .setLeastSentToClient(leastSentTo)
                .setLeastReceivedType(leastReceivedType)
                .setMostReceivedType(mostReceivedType)
                .setMostSentType(mostSentType)
                .setLeastSentType(leastSentType)
                .createGeneralStatistic();
    }

    private String _clientID;
    private Tooltip _toolTip;
    private boolean _canRun;
    private boolean _terminationRequested;
    private static final Logger _logger = LoggerFactory.getLogger(ClientStatisticsManager.class);
}
