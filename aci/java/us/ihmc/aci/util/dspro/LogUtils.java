package us.ihmc.aci.util.dspro;

import org.slf4j.LoggerFactory;

public class LogUtils {

    public static org.slf4j.Logger getLogger(Class c)
    {
        String name = c.getSimpleName();
        if (name.length() > 23) {
            name = name.substring(0, 23);
        }
        return LoggerFactory.getLogger(name);
    }

}
