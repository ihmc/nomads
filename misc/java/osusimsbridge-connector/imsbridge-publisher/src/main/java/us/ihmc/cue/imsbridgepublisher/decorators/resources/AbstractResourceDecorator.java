package us.ihmc.cue.imsbridgepublisher.decorators.resources;

import us.ihmc.cue.imsbridgepublisher.decorators.LFDecorator;
import us.ihmc.linguafranca.Resource;

import java.util.List;

public abstract class AbstractResourceDecorator extends LFDecorator<Resource>
{
    @Override
    public abstract void generateObject ();
}
