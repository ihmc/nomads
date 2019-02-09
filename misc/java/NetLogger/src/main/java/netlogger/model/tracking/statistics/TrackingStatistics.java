package netlogger.model.tracking.statistics;

import netlogger.util.DateUtil;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TrackingStatistics
{
    public TrackingStatistics (GeneralStatistics generalStatistics, ApplicationStatistics applicationStatistics, TypeStatistics typeStatistics) {
        _generalStatistics = generalStatistics;

        _applicationStatistics = applicationStatistics;
        _typeStatistics = typeStatistics;
    }

    @Override
    public String toString () {
        StringBuilder builder = new StringBuilder();

        appendGeneralStatistics(builder);

        appendApplicationStatistics(builder);

        appendTypeStatistics(builder);

        return builder.toString();
    }

    private void appendGeneralStatistics (StringBuilder builder) {
        builder.append("Total sent: ").append(_generalStatistics.getTotalSent());
        builder.append("\nTotal received: ").append(_generalStatistics.getTotalReceived());
        builder.append("\nLast sent: ");
        if (_generalStatistics.getLastSentTime().equals("null")) {
            builder.append("---");
        }
        else {
            long lastSentTime = System.currentTimeMillis() - Long.valueOf(_generalStatistics.getLastSentTime());
            String formatTime = DateUtil.parseDate(lastSentTime);

            builder.append(_generalStatistics.getLastSentTime())
                    .append(" (").append(formatTime).append(" )");
        }

        builder.append("\nLast received: ");
        if (_generalStatistics.getLastReceivedTime().equals("null")) {
            builder.append("---");
        }
        else {
            long timeSinceReceiving = System.currentTimeMillis() - Long.valueOf(_generalStatistics.getLastReceivedTime());
            String formatTime = DateUtil.parseDate(timeSinceReceiving);

            builder.append(_generalStatistics.getLastReceivedTime())
                    .append(" (").append(formatTime).append(" )");
        }

        builder.append("\nMost data received from: ").append(_generalStatistics.getMostReceivedFromClient());
        builder.append("\nMost data sent to: ").append(_generalStatistics.getMostSentToClient());
        builder.append("\nLeast data received from: ").append(_generalStatistics.getLeastReceivedFromClient());
        builder.append("\nLeast data sent to: ").append(_generalStatistics.getLeastSentToClient());

        builder.append("\nMost received type: ").append(_generalStatistics.getMostReceivedType());
        builder.append("\nMost sent type: ").append(_generalStatistics.getMostSentTrackedType());
        builder.append("\nLeast received type: ").append(_generalStatistics.getLeastReceivedType());
        builder.append("\nLeast sent type: ").append(_generalStatistics.getLeastSentTrackedType());
    }

    private void appendApplicationStatistics (StringBuilder builder) {
        if (_applicationStatistics == null) {
            return;
        }

        builder.append("\n\n-----------------");
        builder.append("\nTotal sent: ").append(_applicationStatistics.getTotalSent());
        builder.append("\nTotal received: ").append(_applicationStatistics.getTotalReceived());
        builder.append("\nLast sent: ")
                .append(_applicationStatistics.getLastSentTime())
                .append(" (").append(System.currentTimeMillis() - _applicationStatistics.getLastSentTime()).append("ms)");

        builder.append("\nLast received: ")
                .append(_applicationStatistics.getLastReceivedTime())
                .append(" (").append(System.currentTimeMillis() - _applicationStatistics.getLastReceivedTime()).append("ms)");

        builder.append("\nMost received type: ").append(_applicationStatistics.getMostReceivedType());
        builder.append("\nMost sent type: ").append(_applicationStatistics.getMostSentTrackedType());
        builder.append("\nLeast received type: ").append(_applicationStatistics.getLeastReceivedType());
        builder.append("\nLeast sent type: ").append(_applicationStatistics.getLeastSentTrackedType());
    }

    private void appendTypeStatistics (StringBuilder builder) {
        if (_typeStatistics == null) {
            return;
        }

        builder.append("\n\n-----------------");
        builder.append("\n").append(_typeStatistics.getType()).append(" sent: ").append(_typeStatistics.getTotalSent());
        builder.append("\n").append(_typeStatistics.getType()).append(" received: ").append(_typeStatistics.getTotalReceived());

        builder.append("\n").append(_typeStatistics.getType()).append(" last sent: ")
                .append(_typeStatistics.getLastSentTime())
                .append(" (").append(System.currentTimeMillis() - _typeStatistics.getLastSentTime()).append("ms)");

        builder.append("\n").append(_typeStatistics.getType()).append(" last sent: ")
                .append(_typeStatistics.getLastReceivedTime())
                .append(" (").append(System.currentTimeMillis() - _typeStatistics.getLastReceivedTime()).append("ms)");
    }


    private GeneralStatistics _generalStatistics;
    private ApplicationStatistics _applicationStatistics;
    private TypeStatistics _typeStatistics;

    private static final Logger _logger = LoggerFactory.getLogger(TrackingStatistics.class);
}
