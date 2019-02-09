package netlogger.model;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import measure.proto.Measure;

public class MeasureIn
{
    public MeasureIn (Measure measure, String natsTopic) {
        _natsTopic = natsTopic;
        _measure = measure;
    }

    public String getNatsTopic () {
        return _natsTopic;
    }

    public Measure getMeasure () {
        return _measure;
    }

    private Measure _measure;
    private String _natsTopic;

    private static final Logger _logger = LoggerFactory.getLogger(MeasureIn.class);
}
