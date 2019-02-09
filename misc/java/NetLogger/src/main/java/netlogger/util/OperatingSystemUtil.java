package netlogger.util;

public class OperatingSystemUtil
{
    public static OperatingSystemType getOS () {
        OperatingSystemType type = OperatingSystemType.Unrecognized;

        if (isWindows()) {
            type = OperatingSystemType.Windows;
        }
        else if (isMac()) {
            type = OperatingSystemType.Mac;
        }
        else if (isUnix()) {
            type = OperatingSystemType.Unix;
        }
        else if (isSolaris()) {
            type = OperatingSystemType.Solaris;
        }

        return type;
    }

    private static boolean isWindows () {
        return (_operatingSystem.contains("win"));
    }

    private static boolean isMac () {

        return (_operatingSystem.contains("mac"));

    }

    private static boolean isUnix () {

        return (_operatingSystem.contains("nix") || _operatingSystem.contains("nux") || _operatingSystem.indexOf("aix") > 0);

    }

    private static boolean isSolaris () {

        return (_operatingSystem.contains("sunos"));

    }

    private static final String _operatingSystem = System.getProperty("os.name").toLowerCase();
}
