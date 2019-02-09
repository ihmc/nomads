package us.ihmc.aci.netviewer.scenarios.json;

import java.util.ArrayList;
import java.util.List;

/**
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Local
{
    private String id;
    private String name;
    private List<String> neighbors = new ArrayList<>();
    private List<String> ips = new ArrayList<>();

    /**
     *
     * @return
     * The id
     */
    public String getId()
    {
        return id;
    }

    /**
     *
     * @param id
     * The id
     */
    public void setId (String id)
    {
        this.id = id;
    }

    /**
     *
     * @return
     * The name
     */
    public String getName()
    {
        return name;
    }

    /**
     *
     * @param name
     * The name
     */
    public void setName (String name)
    {
        this.name = name;
    }

    /**
     *
     * @return
     * The neighbors
     */
    public List<String> getNeighbors()
    {
        return neighbors;
    }

    /**
     *
     * @param neighbors
     * The neighbors
     */
    public void setNeighbors (List<String> neighbors)
    {
        this.neighbors = neighbors;
    }

    /**
     *
     * @return
     * The ips
     */
    public List<String> getIps()
    {
        return ips;
    }

    /**
     *
     * @param ips
     * The ips
     */
    public void setIps (List<String> ips)
    {
        this.ips = ips;
    }
}
