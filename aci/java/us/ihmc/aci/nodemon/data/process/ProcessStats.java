package us.ihmc.aci.nodemon.data.process;

import com.google.gson.annotations.SerializedName;

import java.util.HashMap;
import java.util.Map;

/**
 * PROCESS_STATS.java
 */
public class ProcessStats
{

    public ProcessStats ()
    {
        _processes = new HashMap<>();
    }

    /**
     * Gets the process matching with the given process id.
     *
     * @param processId the process id
     * @return the process with the given process id, null if not found
     */
    public Process getProcess (String processId)
    {
        return _processes.get(processId);
    }

    /**
     * Sets the <code>PROCESS</code>, an instance of a program running (generally using the network) in order to collect
     * statistics about it.
     *
     * @param processId the id of the <code>PROCESS</code>
     * @param process   the instance of the <code>PROCESS</code>
     */
    public void setProcess (String processId, Process process)
    {

        _processes.put(processId, process);
    }

    /**
     * Removes a <code>PROCESS</code> from this collection.
     *
     * @param processId the id of the process to be removed.
     */
    public void removeProcess (String processId)
    {
        _processes.remove(processId);
    }

    /**
     * Gets the collection of Processes associated with this <code>NODE</code>.
     *
     * @return a <code>Collection</code> of <code>PROCESS</code> instances
     */
    public Map<String, Process> get ()
    {
        return _processes;
    }

    public void put (Map<String, Process> processes)
    {
        _processes = processes;
    }

    @SerializedName("processes")
    private Map<String, Process> _processes;
}
