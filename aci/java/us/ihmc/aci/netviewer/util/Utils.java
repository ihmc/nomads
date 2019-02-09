package us.ihmc.aci.netviewer.util;

/**
 * Class exposing utility methods
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Utils
{
    /**
     * Builds an id based on a combination of the two strings passed as input in alphabetical order
     * @param s1 first string
     * @param s2 second string
     * @return the id created
     */
    public static String buildId (String s1, String s2)
    {
        if ((s1 == null) || (s2 == null)) {
            return s1 + "-" + s2;
        }

        if (s1.compareTo (s2) <= 0) {
            return s1 + "-" + s2;
        }
        else {
            return s2 + "-" + s1;
        }
    }
}
