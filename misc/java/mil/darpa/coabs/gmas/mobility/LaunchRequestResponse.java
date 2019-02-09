package mil.darpa.coabs.gmas.mobility;

import java.io.Serializable;
import java.util.Enumeration;
import java.util.Hashtable;

import mil.darpa.coabs.gmas.GmamlMessageHandlingService;
import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;

import org.w3c.dom.Element;
import org.w3c.dom.Node;

/**
 * Represents the response of the server to a launchreq of a Grid Mobile agent.
 * In addition, provides conversion methods to and from GMAML.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version $Revision$ $Date$
 * @see GmasMobilityServiceImpl
 * @see GmamlMessageHandlingService
 */
public class LaunchRequestResponse implements Serializable {

    /**
     * 
     */
    public LaunchRequestResponse()
    {
        _errorcode = new Integer (0);
        _token = "";
        _comment = "";
        _vars = null;
    }

    public LaunchRequestResponse (int ec, String tk, String co, Hashtable v)
    {
        _errorcode = new Integer (ec);
        _token = tk;
        _comment = co;
        _vars = v;
    }

    public LaunchRequestResponse (Node gmanode)
    {
        try {
            _token = ((Element) gmanode).getAttribute ("token");
            _errorcode = new Integer (((Element) gmanode).getAttribute ("errorcode"));
            _comment = ((Element) gmanode).getAttribute ("comment");
        }
        catch (Exception e) {
            _log.error ("Invalid LaunchRequestResponse:"
                    + this.toExternalForm());
            e.printStackTrace();
        }
    }

    /** _errorcode retrieval method. */
    public int getErrorcode()
    {
        return _errorcode.intValue();
    }

    /** _token retrieval method. */
    public String getToken()
    {
        return _token;
    }

    /** _comment retrieval method. */
    public String getComment()
    {
        return _comment;
    }

    /** var hashtable retrieval method. */
    public Hashtable getVars()
    {
        return _vars;
    }

    /** _errorcode set method. */
    public void setErrorcode (int ec)
    {
        _errorcode = new Integer (ec);
    }

    /** _token set method. */
    public void setToken (String tk)
    {
        _token = tk;
    }

    public void setComment (String co)
    {
        _comment = co;
    }

    public void setVars (Hashtable v)
    {
        _vars = v;
    }

    /**
     * Convert the LaunchRequestResponse to GMAML form.
     * 
     * @return String containing the equivalent GMAML form.
     */
    public String toExternalForm()
    {
        StringBuffer sb = new StringBuffer();

        sb.append ("<?xml version=\"1.0\" ?>\n");
        try {
            sb.append ("<!DOCTYPE gmas SYSTEM \""
                    + AgentContainer.getGridCodebaseURL() + "gmaml.dtd\">\n");
        }
        catch (GmasMobilityException e1) {
            e1.printStackTrace();
        }
        sb.append ("<gmas>\n");
        sb.append ("<response errorcode=\"" + _errorcode + "\" token=\""
                + _token + "\" comment=\"" + _comment + "\">\n");
        sb.append ("</response>\n");
        sb.append ("</gmas>\n");
        return sb.toString();
    }

    /**
     * Display the object state. This method dumps the object state to the log.
     * It is provided for debugging purposes.
     */
    public void dump()
    {
        Enumeration enumeration;
        Object object;
        String key;

        _log.info ("Response code: " + _errorcode + " token: " + _token);
        _log.info ("comment: " + _comment);
        if (_vars != null && _vars.isEmpty() == false) {
            enumeration = _vars.keys();
            while (enumeration.hasMoreElements()) {
                key = (String) enumeration.nextElement();
                object = _vars.get (key);
                _log.info ("Variable: name=\"" + key + "\" type=\""
                        + object.getClass().getName() + "\" value=\""
                        + object.toString() + "\"\n");
            }
        }
    }

    /** _errorcode/response code returned from the server. */
    protected Integer _errorcode;
    /** one-time use _token(UUID?) returned from the server for entry of agent. */
    protected String _token;
    /** textual _comment returned from the server. */
    protected String _comment;
    /**
     * Vector of variables(name, type, value) potentially included in response
     * to agent. These would typically be resource constraints that the server
     * is informing the agent of.
     */
    protected Hashtable _vars;
    static final Logger _log = LogInitializer.getLogger (LaunchRequestResponse.class.getName());
    static final long serialVersionUID = 1L;
}
