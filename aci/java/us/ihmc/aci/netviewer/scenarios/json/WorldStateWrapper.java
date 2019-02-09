package us.ihmc.aci.netviewer.scenarios.json;

import java.util.ArrayList;
import java.util.List;

public class WorldStateWrapper
{
    private Local local;
    private List<Remote> remote = new ArrayList<>();

    /**
     *
     * @return
     * The local
     */
    public Local getLocal()
    {
        return local;
    }

    /**
     *
     * @param local
     * The local
     */
    public void setLocal (Local local)
    {
        this.local = local;
    }

    /**
     *
     * @return
     * The remote
     */
    public List<Remote> getRemote()
    {
        return remote;
    }

    /**
     *
     * @param remote
     * The remote
     */
    public void setRemote (List<Remote> remote)
    {
        this.remote = remote;
    }
}
