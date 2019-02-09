package netlogger.util.params;

import org.kohsuke.args4j.Option;

public class CommandLineValues
{
    @Option(name = "-consoleLogLevel", usage = "Sets the netlogger.chqosinitializer level for the console, possible values are: ALL, TRACE, DEBUG, INFO, WARN, ERROR")
    public String consoleDebugLevel = "INFO";

    @Option(name = "-fileLogLevel", usage = "Sets the netlogger.chqosinitializer level for the file, possible values are: ALL, TRACE, DEBUG, INFO, WARN, ERROR")
    public String fileDebugLevel = "INFO";

    @Option(name = "-logHome", usage = "Sets the location of the log folder")
    public String logHome = "log";

    @Option(name = "-conf", usage = "Sets the config file location")
    public String configFile = "conf/netlogger.properties";

}
