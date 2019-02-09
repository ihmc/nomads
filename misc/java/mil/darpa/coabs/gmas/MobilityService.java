package mil.darpa.coabs.gmas;

/**
 * MobilityService 
 * <p>
 * Copyright 2000 Arne Grimstrup, Department of
 * Computer Science, Dartmouth College Use, modification and redistribution
 * subject to the terms of the standard GNU public license at
 * http://www.gnu.org/copyleft/gpl.html.
 * <p>
 * Defines the GMAS Mobility services interface that will be implemented by all
 * GMAS implementors.
 * 
 * @author Arne Grimstrup
 * @author Greg Hill
 * @author Tom Cowin <tom.cowin@gmail.com>
 */
import mil.darpa.coabs.gmas.mobility.GmasMobilityException;
import mil.darpa.coabs.gmas.mobility.GridAgentMetaData;
import mil.darpa.coabs.gmas.mobility.GridAgentVariableState;
import mil.darpa.coabs.gmas.mobility.GridMobileAgent;

import com.globalinfotek.coabsgrid.entry.CoABSAgentDescription;

public interface MobilityService {

    /**
     * Make a copy of the specified agent from the current system to the
     * specified location. Will result in the invocation of the agent's
     * getMetaData() method. May also result in the invocation of the agent's
     * getAgentState() method (this will be the case for Type I mobility).
     * 
     * @param gma the mobile agent
     * @param location agent description of Grid visible server entity to send
     *                 agent to
     * @exception GmasMobilityException in case of an error in creating the
     *                clone
     */
    public void cloneAgent (GridMobileAgent gma, CoABSAgentDescription location)
            throws GmasMobilityException;

    /**
     * Launch a new instance of the specified agent to the specified location.
     * 
     * @param gamd metadata for the agent to be launched
     * @param gavs the initial state to be passed to the agent via the
     *            setInitialState() method
     * @param location agent description of Grid visible server entity to send
     *                 agent to
     * @exception GmasMobilityException in case of an error during the launch
     */
    public void launchAgent (GridAgentMetaData gamd,
                             GridAgentVariableState gavs,
                             CoABSAgentDescription location) throws GmasMobilityException;

}
