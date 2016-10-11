package us.ihmc.aci.disServiceProProxy;

import java.util.logging.Logger;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.StringUtil;

/**
 *
 * @author Giacomo Benincasa    gbenincasa@ihmc.us
 */

public class PeerNodeContext
{
    public PeerNodeContext (String nodeID, String teamID, String missionID, String role, String matchmakingFilter,
                            int currInformationVersion, int classifierVersion, int currPathVersion, int currWayPointVersion,
                            int currWayPointInPath, int usefulDistance, boolean isPeerActive)
    {
        _nodeID = nodeID;
        _teamID = teamID;
        _missionID = missionID;
        _role = role;
        _currInformationVersion = currInformationVersion;
        _classifierVersion = classifierVersion;
        _currPathVersion = currPathVersion;
        _currWayPointVersion = currWayPointVersion;
        _matchmakingFilter = matchmakingFilter;
        _currWayPointInPath = currWayPointInPath;      // Current position of the node along the path.
        _usefulDistance = usefulDistance;
        _isPeerActive = isPeerActive;
    }

    public static PeerNodeContext read (CommHelper commHelper)
    {
        String nodeID = "";
        String teamID = "";
        String missionID = "";
        String role = "";
        boolean isPeerActive = false;
        int usefulDistance = -1;
        int currWayPointInPath = -1;
        String matchmakingFilter = "";
        int currInformationVersion = -1;
        int currPathVersion = -1;
        int currWayPointVersion = -1;
        int classifierVersion = -1;

        try {
            int len;
            nodeID = ((len = commHelper.read32()) > 0) ? new String (commHelper.receiveBlob(len)) : null;
            teamID = ((len = commHelper.read32()) > 0) ? new String (commHelper.receiveBlob(len)) : null;
            missionID = ((len = commHelper.read32()) > 0) ? new String (commHelper.receiveBlob(len)) : null;
            role = ((len = commHelper.read32()) > 0) ? new String (commHelper.receiveBlob(len)) : null;
            isPeerActive = (commHelper.read8() == 1 ? true : false);
            usefulDistance = commHelper.read32();
            currWayPointInPath = commHelper.read32();
            matchmakingFilter = ((len = commHelper.read32()) > 0) ? new String (commHelper.receiveBlob(len)) : null;
            currInformationVersion = commHelper.read16();
            currPathVersion = commHelper.read16();
            currWayPointVersion = commHelper.read16();
            classifierVersion = commHelper.read16();
        }
        catch (Exception e) {
            _LOGGER.severe (StringUtil.getStackTraceAsString (e));
        }

        return new PeerNodeContext (nodeID, teamID, missionID, role,  matchmakingFilter, currInformationVersion,
                                    classifierVersion, currPathVersion, currWayPointVersion,
                                    currWayPointInPath, usefulDistance, isPeerActive);
    }

    //Variables
    public String _nodeID;
    public String _teamID;
    public String _missionID;
    public String _role;
    public int _currInformationVersion;
    public int _classifierVersion;
    public int _currPathVersion;
    public String _matchmakingFilter;
    public int _currWayPointVersion;
    public int _currWayPointInPath;
    public int _usefulDistance;
    public boolean _isPeerActive;

    private static final Logger _LOGGER = Logger.getLogger (PeerNodeContext.class.getName());
}
