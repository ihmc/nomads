package us.ihmc.aci.netSupervisor.utilities;

/**
 * Created by Roberto on 3/7/2016.
 */
import org.apache.log4j.Logger;

public class Utilities {
    public static double getFactorial(int n) {
        if( n <= 1 )     // base case
            return 1;
        else
            return n * getFactorial( n - 1 );
    }

    public static int startTimeCounter() {
        for(int index = 0; index < 100; index++)
        {
            if(_startTime[index] == 0) {
                _startTime[index] = System.currentTimeMillis();
                return index;
            }
        }
        return -1;
    }

    public static void getElapsedTime(int n) {
        log.info("World state summary populated, time elapsed: " + (System.currentTimeMillis() - _startTime[n]) + "ms");
        _startTime[n] = 0;
    }

    private static double[] _startTime = new double[100];
    private static final Logger log = Logger.getLogger(Utilities.class);
}

