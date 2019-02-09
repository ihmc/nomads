package netlogger.model.timeseries;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class CountStatistic
{
    public CountStatistic (String srcApplication, String destApplication, String type, int count) {
        _count = count;
        _sourceApplication = srcApplication;
        _destApplication = destApplication;
    }

    public long getCount () {
        return _count;
    }

    public String getSourceApplication () {
        return _sourceApplication;
    }

    public String getDestApplication () {
        return _destApplication;
    }

    private String _sourceApplication;
    private String _destApplication;
    private int _count;
    private static final Logger _logger = LoggerFactory.getLogger(CountStatistic.class);
}
