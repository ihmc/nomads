package us.ihmc.aci.netviewer.scenarios;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.nodemon.data.NodeMonDataType;
import us.ihmc.aci.nodemon.msg.IncomingMessage;
import us.ihmc.aci.nodemon.msg.MessageType;
import us.ihmc.aci.nodemon.msg.OutgoingMessage;
import us.ihmc.aci.nodemon.scheduler.ProxyScheduler;
import us.ihmc.aci.nodemon.scheduler.Recipient;

import java.util.Collection;
import java.util.Objects;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;

/**
 * Plays a scenario identified by a list of scenario update events
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class ScenarioPlayer implements Runnable
{
    /**
     * Constructor
     * @param proxyScheduler node monitor proxy scheduler instance
     */
    ScenarioPlayer (ProxyScheduler proxyScheduler)
    {
        _scenarioEventsQueue = new LinkedBlockingDeque<>();
        _proxyScheduler = Objects.requireNonNull (proxyScheduler, "The proxy scheduler instance cannot be null");
    }

    /**
     * Says whether the player was started at least once
     * @return true if the player was started at least once
     */
    boolean hasStarted()
    {
        return _hasStarted;
    }

    /**
     * Starts the thread to play a specific scenario
     * @param events update events
     */
    void start (Collection<ScenarioEvent> events)
    {
        log.info ("Received request to start the scenario");

        _scenarioEventsQueue.clear();
        _scenarioEventsQueue.addAll (events);

        _terminate = false;
        (new Thread (this, "SampleScenarioThread")).start();
        _hasStarted = true;
        play();
    }

    /**
     * Stops the thread to play a specific scenario
     */
    void stop()
    {
        _terminate = true;
        log.info ("Received request to stop the scenario");
    }

    /**
     * Plays a specific scenario
     */
    void play()
    {
        _paused = false;
        log.info ("Received request to play the scenario");
    }

    /**
     * Pause a specific scenario
     */
    void pause()
    {
        _paused = true;
        log.info ("Received request to pause the scenario");
    }

    @Override
    public void run()
    {
        log.info ("Scenario player started");
        while (!_terminate) {
            if (_paused) {
                log.info ("The scenario player is paused");
                try {
                    Thread.sleep (2000);
                    continue;
                }
                catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            ScenarioEvent scenarioEvent = null;
            try {
                scenarioEvent = _scenarioEventsQueue.poll ((long) 3, TimeUnit.SECONDS);
            }
            catch (InterruptedException e) {
                e.printStackTrace();
                continue;
            }

            if (scenarioEvent == null) {
                continue;
            }

            log.info ("Polled scenario event from the queue");

            try {
                long sleep = scenarioEvent.getSleepTime();
                log.info ("Sleeping for " + sleep + " ms");
                Thread.sleep (sleep);
            }
            catch (InterruptedException e) {
                e.printStackTrace();
            }

            log.info ("Processing scenario event");

            OutgoingMessage oMsg;
            IncomingMessage iMsg;

            switch (scenarioEvent.getEvent().getType()) {
                case newNode:
                    oMsg = new OutgoingMessage (MessageType.NewNode, Recipient.Node,
                            scenarioEvent.getEvent().getNodeId(), NodeMonDataType.Node,
                            scenarioEvent.getEvent().getNode());
                    if (!_terminate) {  // in case the scenario has been stopped
                        _proxyScheduler.addOutgoingMessage (oMsg);
                    }
                    break;
                case deadNode:
                    oMsg = new OutgoingMessage(MessageType.DeadNode, Recipient.Node, scenarioEvent.getEvent().getNodeId(),
                            NodeMonDataType.Node, null);
                    if (!_terminate) {  // in case the scenario has been stopped
                        _proxyScheduler.addOutgoingMessage (oMsg);
                    }
                    break;
                case updatedNode:
                    iMsg = new IncomingMessage (MessageType.UpdateData, Recipient.Node, scenarioEvent.getEvent().getNodeId(),
                            NodeMonDataType.NodeStats, scenarioEvent.getEvent().getBUpdatedData());
                    if (!_terminate) {  // in case the scenario has been stopped
                        _proxyScheduler.addIncomingMessage (iMsg);
                    }
                    break;
                default:
                    log.error ("Wrong event type " + scenarioEvent.getEvent().getType());
                    continue;
            }
        }

        _hasStarted = false;
        log.info ("Scenario player stopped");
    }


    private boolean _terminate = false;
    private boolean _paused = true;
    private boolean _hasStarted = false;
    private final BlockingQueue<ScenarioEvent> _scenarioEventsQueue;
    private final ProxyScheduler _proxyScheduler;

    private static final Logger log = LoggerFactory.getLogger (ScenarioPlayer.class);
}
