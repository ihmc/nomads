package mil.darpa.coabs.gmas;

import java.util.Vector;

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;
import mil.darpa.coabs.gmas.util.GridHelper;

import com.globalinfotek.coabsgrid.DefaultAgentRep;
import com.globalinfotek.coabsgrid.entry.CoABSAgentDescription;

/**
 * The Abstract implementation of the BasicGridService. Provides the primitive
 * grid service operations to allow registration and lookup. Uses the GridHelper
 * class to communicate over the Grid. If the user of subclasses of this 
 * service needs to utilize more advanced services such as messaging or 
 * mobility, there is no need for the user to have explicitly registered with 
 * the Grid. The service object itself will register with the Grid in order to 
 * perform the operation.
 * <p>
 * The implementor will have to provide an impl of (@link #getLocalGridHelper())
 * for MAS specific differentiation.
 * 
 * @author tcowin <tom.cowin@gmail.com> modifications to extend GMAS for
 *         messaging, additional platform support
 * @version $Revision$ $Date$
 * @see mil.darpa.coabs.gmas.util.GridHelper
 * @see mil.darpa.coabs.gmas.BasicGridService
 */

abstract public class BasicGridServiceImpl implements BasicGridService {

    /**
     * 
     */
    public BasicGridServiceImpl()
    {
    }

    /**
     * Register the given object represented by the CoABS Agent Description with
     * the CoABS Grid.
     * 
     * @see com.globalinfotek.coabsgrid.entry.CoABSAgentDescription
     * @throws GmasServiceException
     */
    public String register (CoABSAgentDescription mydesc)
            throws GmasServiceException
    {
        _coabsAgentDescription = mydesc;
        registerWithGrid();
        return "success";
    }

    /**
     * Deregister this object from the Grid.
     * 
     * @throws GmasServiceException
     */
    public void deregister()
            throws GmasServiceException
    {
        if (_gridHelper == null) {
            // already deregistered.
            return;
        }

        try {
            _gridHelper.deregister();
            _gridHelper = null;
        }
        catch (Exception xcp) {
            xcp.printStackTrace();
            throw new GmasServiceException ("failed to deregister from grid");
        }
    }

    /**
     * Updates the registration with the Grid with a new CoABS Agent
     * Description.
     * 
     * @param newdesc describes some new or different agent capabilities
     * @throws GmasServiceException
     */
    public void updateRegistration (CoABSAgentDescription newdesc)
            throws GmasServiceException
    {
        _gridHelper.updateRegistration (newdesc);
    }

    /**
     * Attempt to locate the service or agent described by the target on the
     * Grid. If found, a vector of AgentRep's will be returned. Otherwise, a
     * null will be returned.
     * 
     * @see com.globalinfotek.coabsgrid.DefaultAgentRep
     * @see com.globalinfotek.coabsgrid.entry.CoABSAgentDescription
     * @throws GmasServiceException
     */
    public Vector<DefaultAgentRep> lookup (CoABSAgentDescription target)
            throws GmasServiceException
    {
        DefaultAgentRep destinationAgentRep = null;
        try {
            destinationAgentRep = _gridHelper.findAgentByName (target.name);
            _log.debug ("found: " + destinationAgentRep.getName());
        }
        catch (Exception xcp) {
            _log.error ("Exception while attempting to find agent on grid: "
                    + target.name);
            return null;
        }
        if (destinationAgentRep == null) {
            return null;
        }
        Vector<DefaultAgentRep> returnVector = new Vector<DefaultAgentRep>();
        returnVector.addElement (destinationAgentRep);
        return returnVector;
    }

    /**
     * Returns gridHelper object customized to local environment, with appropriate
     * name, description and other settings relevant to local agent system impl.
     * 
     * @return gridHelper that provides access to basic CoABS Grid services
     * @throws GmasServiceException
     */
    abstract protected GridHelper getLocalGridHelper()
            throws GmasServiceException;

    /**
     * Registers with the CoABS grid and attempts to find destination server on
     * the Grid.
     * 
     * @param server destination server in form of an array of Jini Entries
     * @throws GmasServiceException
     */
    protected DefaultAgentRep findServerOnGrid (CoABSAgentDescription server)
            throws GmasServiceException
    {
        try {
            registerWithGrid();
        }
        catch (Exception xcp) {
            throw new GmasServiceException ("Failed to register with the Grid.");
        }

        DefaultAgentRep destinationAgentRep = null;
        try {
            Vector<DefaultAgentRep> v = lookup (server);
            destinationAgentRep = v.firstElement();
        }
        catch (Exception xcp) {
            throw new GmasServiceException ("Failed to find grid location ");
        }
        return destinationAgentRep;
    }

    /**
     * Registers the Mobility Service with the Grid. For purposes of moving or
     * launching an agent, or exchanging messages, the service itself must first
     * register with the Grid so that it can send a GridMsg request to a
     * launcher registered with the Grid.
     * 
     * @throws GmasServiceException
     */
    protected void registerWithGrid()
            throws GmasServiceException
    {
        if (_gridHelper != null) {
            // already registered.
            return;
        }
        if (_coabsAgentDescription != null) {
            _gridHelper = new GridHelper (_coabsAgentDescription.name,
                                          _coabsAgentDescription.description,
                                          _coabsAgentDescription.acls,
                                          _coabsAgentDescription.architecture,
                                          _coabsAgentDescription.contentLanguages,
                                          _coabsAgentDescription.displayIconURL,
                                          _coabsAgentDescription.documentationURL,
                                          _coabsAgentDescription.ontologies,
                                          _coabsAgentDescription.organization);
        }
        else {
            _gridHelper = getLocalGridHelper();
        }
    }

    protected Logger _log = LogInitializer.getLogger (this.getClass().getName());
    protected GridHelper _gridHelper = null;
    private CoABSAgentDescription _coabsAgentDescription = null;
}
