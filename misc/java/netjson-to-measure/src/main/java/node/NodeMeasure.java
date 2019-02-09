package node;

import org.jeasy.rules.api.Rule;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.ddam.databroker.proto.Measure;

import java.util.ArrayList;
import java.util.List;

public final class NodeMeasure implements MeasureGenerator
{
    public NodeMeasure()
    {
        _rules = new ArrayList<>();
    }

    @Override
    public Measure generateMeasure () {
        return null;
    }

    private List<Rule> _rules;
    private static final Logger _logger = LoggerFactory.getLogger(NodeMeasure.class);
}
