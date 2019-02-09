package us.ihmc.aci.netviewer.scenarios;

import us.ihmc.aci.netviewer.util.Event;
import us.ihmc.aci.nodemon.data.Node;

import java.util.Collection;
import java.util.Objects;

/**
 * Container for scenario events
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
class ScenarioEvent
{
    /**
     * Constructor
     * @param event generic event
     * @param sleep sleep time before notifying the listeners about the current event
     */
    ScenarioEvent (Event event, long sleep)
    {
        _event = Objects.requireNonNull (event, "The " + Event.class.getSimpleName() + " object cannot be null");
        _sleep = sleep;
    }

    /**
     * Gets the sleep time the thread needs to wait before notifying the listeners about the current event
     * @return the sleep time the thread needs to wait before notifying the listeners about the current event
     */
    long getSleepTime()
    {
        return _sleep;
    }

    /**
     * Gets the current event
     * @return the current event
     */
    Event getEvent()
    {
        return _event;
    }


    private final long _sleep;
    private final Event _event;
}
