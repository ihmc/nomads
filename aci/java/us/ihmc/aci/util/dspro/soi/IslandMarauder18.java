package us.ihmc.aci.util.dspro.soi;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by gbeni on 4/5/2018.
 */
public class IslandMarauder18 {

    private static final Map<String, String> _dsproIdToURN;
    static {
        Map<String, String> m = new HashMap<>();

        // By long name
        m.put(toKey("PLT CMDR, PLT HQ, 1 PLT, E CO, 2 BN, 3 MAR, 3 MARDIV"), "2390781");
        m.put(toKey("1 SQD, 1 PLT, E CO, 2 BN, 3 MAR, 3 MARDIV"), "2390784");
        m.put(toKey("2 SQD, 1 PLT, E CO, 2 BN, 3 MAR, 3 MARDIV"), "2390802");

        // By short name
        m.put(toKey("E Co 1st Plt Cmdr"), "2390781");
        m.put(toKey("E Co 1st Sqd LDR 1st Plt"), "2390785");
        m.put(toKey("E Co 2nd Sqd LDR 1st Plt"), "2390802");

        // By long name
        m.put(toKey("3SQD, 3 PLT C 2BN 3MAR"), "2390590");
        m.put(toKey("1FIRETEAM, 3 SQD 3PLT C 1BN 3MAR"), "2390592");
        m.put(toKey("2FIRETEAM, 3 SQD 3PLT C 1BN 3MAR"), "2390597");
        m.put(toKey("3FIRETEAM, 3 SQD 3PLT C 1BN 3MAR"), "2390602");

        // By short name
        m.put(toKey("SOI Handheld 1"), "2390590");
        m.put(toKey("SOI Handheld 2"), "2390592");
        m.put(toKey("ATAK Handheld 3"), "2390597");
        m.put(toKey("ATAK Handheld 4"), "2390602");

        m.put(toKey("SOI H1"), "2390590");
        m.put(toKey("SOI H2"), "2390592");
        m.put(toKey("ATAK1"), "2390597");
        m.put(toKey("ATAK2"), "2390602");

        _dsproIdToURN = Collections.unmodifiableMap(m);
    }

    private static String toKey (String s) {
        return s.replaceAll("\\s+","").toLowerCase();
    }

    static String toURN (String name) {
        final String key = toKey(name);
        if (!_dsproIdToURN.containsKey(key)) {
            return "0000000";
        }
        return _dsproIdToURN.get(key);
    }


}
