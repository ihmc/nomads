package node;

import us.ihmc.ddam.databroker.proto.Measure;

public interface MeasureGenerator
{
    Measure generateMeasure();
}
