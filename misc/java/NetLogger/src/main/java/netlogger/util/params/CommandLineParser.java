package netlogger.util.params;

import org.kohsuke.args4j.CmdLineException;
import org.kohsuke.args4j.CmdLineParser;

public class CommandLineParser
{
    public CommandLineValues parse (String[] args) {
        CommandLineValues clvs = new CommandLineValues();
        CmdLineParser parser = new CmdLineParser(clvs);
        try {
            parser.parseArgument(args);
            return clvs;
        } catch (CmdLineException e) {
            // handling of wrong arguments
            System.err.println(e.getMessage());
            parser.printUsage(System.err);
            return null;
        }
    }
}
