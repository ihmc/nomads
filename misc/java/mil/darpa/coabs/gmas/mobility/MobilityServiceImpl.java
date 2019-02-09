package mil.darpa.coabs.gmas.mobility;

import java.io.Serializable;

import mil.darpa.coabs.gmas.BasicGridServiceImpl;
import mil.darpa.coabs.gmas.GmasServiceException;
import mil.darpa.coabs.gmas.MobilityService;
import mil.darpa.coabs.gmas.util.GmamlParser;
import mil.darpa.coabs.gmas.util.URIEncoder;

import com.globalinfotek.coabsgrid.DefaultAgentRep;
import com.globalinfotek.coabsgrid.Message;
import com.globalinfotek.coabsgrid.entry.CoABSAgentDescription;

/**
 * The implementation of the BasicGridService for GMAS Mobility Services. Its
 * parent class uses the GridHelper class to communicate over the Grid. If the
 * user of this service wants to move itself or launch some other agent over the
 * Grid, there is no need for the user to have registered with the Grid. The
 * service object itself will register with the Grid in order to perform the
 * move or launch operation.
 * 
 * @author tcowin <tom.cowin@gmail.com> modifications to extend GMAS for
 *         messaging, additional platform support
 * @version $Revision$ $Date$
 * @see mil.darpa.coabs.gmas.MobilityService
 * @see mil.darpa.coabs.gmas.BasicGridService
 * @see mil.darpa.coabs.gmas.BasicGridServiceImpl
 */

abstract public class MobilityServiceImpl extends BasicGridServiceImpl
        implements MobilityService {

    public MobilityServiceImpl()
    {
        super();
    }

    /**
     * Clone (move) this agent to remote server specified. This method utilizes
     * the Grid to locate the destination server, then uses the GMAS infra-
     * structure to convert the passed in GridMobileAgent to a form where it can
     * be sent utilizing Grid Messaging, in an XML format defined by the
     * gmaml(Grid Mobile Agent Markup Language).dtd.
     * 
     * @param gridMobileAgent agent to be moved
     * @param server the destination Grid location
     * @see mil.darpa.coabs.mobility.client.BasicGridService
     */
    public void cloneAgent (GridMobileAgent gridMobileAgent, CoABSAgentDescription server)
            throws GmasMobilityException
    {
        AgentContainer agentContainer = null;
        DefaultAgentRep gridDestination = null;
        try {
            gridDestination = findServerOnGrid (server);
        }
        catch (GmasServiceException e) {
            e.printStackTrace();
        }
        GridAgentMetaData metaData = gridMobileAgent.getMetaData();
        _log.info ("Sending GMAS Agent: [" + metaData.getLaunchAgent()
                + "] to " + server.name);
        metaData.setOperation (AgentContainer.MoveOperationType.Clone);

        if (gridMobileAgent instanceof Serializable) {
            // Type II agent
            try {
                gridMobileAgent.setMetaData (metaData);
                agentContainer = new AgentContainer (metaData,
                                                     (Serializable) gridMobileAgent);
            }
            catch (Exception xcp) {
                throw new GmasMobilityException ("An error occurred when trying to serialize the GridMobileAgent for Type II mobility.");
            }
        }
        else if (gridMobileAgent instanceof SelfSerializable) {
            agentContainer = new AgentContainer (metaData,
                                                 ((SelfSerializable) gridMobileAgent).getAgentState());
        }
        else {
            throw new GmasMobilityException ("The GridMobileAgent is neither an instance of SelfSerializable nor Serializable");
        }

        mobilizeAgent (gridDestination, agentContainer);
        try {
            deregister();
        }
        catch (GmasServiceException e) {
            e.printStackTrace();
        }
    }

    /**
     * Launch operation is similar to cloneAgent, but since there is no existing
     * agent instance or state to send, operation is a bit simpler. It still
     * creates a GMAML message from the info contained in the meta data and the
     * variable state.
     * 
     * @param metaData MetaData of Agent to be launched.
     * @param variableState state of internal class variables
     * @param server destination server
     */
    public void launchAgent (GridAgentMetaData metaData,
                             GridAgentVariableState variableState,
                             CoABSAgentDescription server)
            throws GmasMobilityException
    {
        try {
            DefaultAgentRep gridDestination = findServerOnGrid (server);
            metaData.setOperation (AgentContainer.MoveOperationType.Launch);
            mobilizeAgent (gridDestination, new AgentContainer (metaData,
                                                                variableState));
            deregister();
        }
        catch (GmasServiceException e) {
            e.printStackTrace();
        }
        catch (GmasMobilityException e) {
            e.printStackTrace();
        }
    }

    /**
     * Performs a three step process to affect the move or launch of an agent
     * via Grid Messaging. This includes the formulation and transmission of a
     * launch request to the destination server, receipt of a response from the
     * dest server which indicates acceptance or denial of the move request,
     * then, if the move/launch is accepted, then an XML message containing the
     * agent(move) or agent's info(launch) is sent.
     * 
     * @param gridDestination specifies the location of the server on the Grid
     *            where the agent is headed
     * @param agentContainer holds agent or agents info needed to reconstitute
     *            the agent on the far end.
     */
    private void mobilizeAgent (DefaultAgentRep gridDestination,
                                AgentContainer agentContainer)
            throws GmasMobilityException
    {
        Message responseMsg;
        LaunchRequestResponse launchRequestResponse = null;
        String msgText = null;
        LaunchRequest launchRequest = new LaunchRequest (agentContainer.getMetaData());

        _log.info ("Sending agent launch request to: "
                + gridDestination.getName());
        try {
            _gridHelper.sendMsg (gridDestination,
                                 "GMAML",
                                 launchRequest.toExternalForm());
        }
        catch (Exception xcp) {
            xcp.printStackTrace();
            throw new GmasMobilityException ("Failed to send move request to grid location ");
        }

        do {
            try {
                _log.debug ("Waiting for response to launch request from server...");
                responseMsg = _gridHelper.receiveMsgBlocking();
            }
            catch (Exception xcp) {
                xcp.printStackTrace();
                throw new GmasMobilityException ("Failed to send launch request to grid location ");
            }
            msgText = URIEncoder.includeSpecialChars (responseMsg.getRawText());
            _log.debug ("Response to agent launch request is: " + msgText);
        } while (!msgText.startsWith ("<?xml"));

        for (Object msgObject : new GmamlParser().extractMessageObjects (msgText)) {
            if (msgObject.getClass().getName().endsWith ("LaunchRequestResponse")) {
                launchRequestResponse = (LaunchRequestResponse) msgObject;
            }
            else {
                throw new GmasMobilityException ("Not able to parse Response correctly out of rtn msg");
            }
        }
        if (launchRequestResponse.getErrorcode() != 0) {
            throw new GmasMobilityException ("Received non-zero status code from destination server");
        }
        else {
            _log.info ("Requested move accepted, sending agent to "
                    + gridDestination.getName());
            agentContainer.setLaunchApprovalToken (launchRequestResponse.getToken());
            try {
                _gridHelper.sendMsg (gridDestination,
                                     "GMAML",
                                     agentContainer.toExternalForm());
            }
            catch (Exception xcp) {
                xcp.printStackTrace();
                throw new GmasMobilityException ("Failed to send move request to grid location ");
            }
        }
    }
}
