package us.ihmc.aci.nodemon.util;

/**
 * Strings.java
 * <p/>
 * Class <code>Strings</code> contains constant static String objects useful throughout the <code>NodeMon</code>
 * context.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Strings
{
    public interface Files
    {
        String GROUP_MANAGER_AUTO_CONFIG = "auto.grpMgr.properties";
        String LOG4J_CONFIG = "nodemon.log4j.properties";
    }

    public interface Paths
    {
        String LOG = "log";
        String CONF = "conf";
    }

    public interface Formats
    {
        String DATE = "yyyy.MM.dd.HH.mm.ss";
    }

    public interface Symbols
    {
        String GROUP_MANAGER_PREFIX = "aci.groupmanager";
        String GROUP_MANAGER_UDP_MULTICAST_NORELAY = "UDP_MULTICAST_NORELAY";
    }

    public interface Messages
    {
        String AUTO_CONFIG_WARNING = "THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT. ANY EDIT WILL BE LOST!";
    }
}
