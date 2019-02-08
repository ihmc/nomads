package us.ihmc.aci.disServiceProProxy;

/**
 * User: mbreedy
 * Date: Sep 21, 2010
 * Time: 8:58:50 AM
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 */

public interface MatchmakingLogListener
{
    public void informationMatched (String localNodeID, String peerNodeID,
                                    String matchedObjectID, String matchedObjectName,
                                    String[] rankDescriptors,
                                    float[] partialRanks, float[] weights, String comment,
                                    String operation);

    public void informationSkipped (String localNodeID, String peerNodeID,
                                    String skippedObjectID, String skippedObjectName,
                                    String[] rankDescriptors,
                                    float[] partialRanks, float[] weights, String comment,
                                    String operation);

    public enum  RankDescriptor {
        COORDINATES ("Coordinates"),
        TIME ("Time"),
        EXPIRATION ("Expiration"),
        IMPORTANCE ("Importance"),
        SOURCE_RELIABILITY ("Source_Reliability"),
        TARGET ("Target"),
        PREDICTION ("Prediction");

        RankDescriptor (String descr)
        {
            _descr = descr;
        }

        @Override
        public String toString()
        {
            return _descr;
        }

        private final String _descr;
    }
}
