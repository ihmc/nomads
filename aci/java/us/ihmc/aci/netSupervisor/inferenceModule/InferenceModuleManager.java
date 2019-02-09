package us.ihmc.aci.netSupervisor.inferenceModule;

import us.ihmc.aci.ddam.NetworkEvent;
import us.ihmc.aci.ddam.NetworkEventContainer;
import us.ihmc.aci.ddam.QoS;
import us.ihmc.aci.netSupervisor.traffic.WorldStateSummary;

import java.util.List;

/**
 * InferenceModuleManager.java
 * <p/>
 * Interface <code>InferenceModuleManager</code> defines an API for different Inference Modules.
 *
 * @author Emanuele Tagliaferro (etagliaferro@ihmc.us)
 */

public interface InferenceModuleManager
{

    /**
     * Initializes the module
     */
    public void init(String lpSolverPath);

    /**
     * Populates the traffic entries to analyze and pass to a different module
     */
    public QoS startPriorityMechanism(NetworkEventContainer inputEvents, WorldStateSummary worldStateSummary);

    /**
     * Creates the structure to manage the events for a tag
     */
    public void createInferenceTag (String tagName, QoS qosTrafficDescription);

    /**
     * Creates a structure for a composite event
     */
    public void createEvent (NetworkEvent event, NetworkEventContainer events);

    /**
     * Update the system priority classification with some new input events
     *
     * @param events
     */
    public NetworkEventContainer updatePriorityClassification (NetworkEventContainer events);

}
