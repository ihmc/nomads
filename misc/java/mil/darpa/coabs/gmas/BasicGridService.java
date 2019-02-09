package mil.darpa.coabs.gmas;

/**
 * BasicGridService.java abstracted/refactored from original
 * GridMobilityServiceInterface
 * <p>
 * Copyright 2000 Arne Grimstrup, Department of Computer Science, Dartmouth
 * College Use, modification and redistribution subject to the terms of the
 * standard GNU public license at http://www.gnu.org/copyleft/gpl.html.
 * <p>
 * Defines the interface for the most basic CoABS Grid operations that will be
 * implemented by all GMAS implementors.
 * 
 * @author Arne Grimstrup
 * @author Greg Hill
 * @author Tom Cowin <tom.cowin@gmail.com>
 */

import java.util.Vector;

import com.globalinfotek.coabsgrid.DefaultAgentRep;
import com.globalinfotek.coabsgrid.entry.CoABSAgentDescription;

public interface BasicGridService {

    /**
     * Register the object described by the provided description with the CoABS
     * Grid.
     * 
     * @param mydesc description of entity registering; name is typically the
     *            salient feature
     * @return
     * @throws GmasServiceException
     */
    public String register (CoABSAgentDescription mydesc)
            throws GmasServiceException;

    /**
     * Deregister from the Grid, removing the advertisement of your presence.
     * 
     * @throws GmasServiceException
     */
    public void deregister()
            throws GmasServiceException;

    /**
     * Update your registration on the grid providing new or different info in
     * the included description.
     * 
     * @param newdesc new info about you
     * @throws GmasServiceException
     */
    public void updateRegistration (CoABSAgentDescription newdesc)
            throws GmasServiceException;

    /**
     * Attempt to locate the target entity on the Grid.
     * 
     * @param target entity you are attempting to locate; name is most salient
     * @return vector of agentRep objects representing the entities found that
     *         match the provided description
     * @throws GmasServiceException
     */
    public Vector<DefaultAgentRep> lookup (CoABSAgentDescription target)
            throws GmasServiceException;
}
