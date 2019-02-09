package netlogger.logger;

import ch.qos.logback.classic.Level;
import ch.qos.logback.classic.LoggerContext;
import ch.qos.logback.classic.encoder.PatternLayoutEncoder;
import ch.qos.logback.classic.spi.ILoggingEvent;
import ch.qos.logback.core.ConsoleAppender;
import ch.qos.logback.core.FileAppender;
import ch.qos.logback.core.util.StatusPrinter;
import chqosinitializer.ContextManager;
import chqosinitializer.DefaultLogFilter;
import chqosinitializer.RepeatDebounceFilter;
import netlogger.util.params.CommandLineValues;
import org.slf4j.LoggerFactory;

import java.text.SimpleDateFormat;
import java.util.Date;

public class LoggerInitializerImpl
{
    public LoggerInitializerImpl () {
        _contextManager = new ContextManager();
    }

    public void initialize (String fileLogLevel, String consoleLogLevel, String logHome, String name) {

        _applicationName = name;

        Level fileLevel = getLevelFromString(fileLogLevel);
        Level consoleLevel = getLevelFromString(consoleLogLevel);

        createConsoleAppender(consoleLevel);
        createLogAppender(fileLevel, logHome);
        _contextManager.print();
    }

    private Level getLevelFromString (final String level) {
        return Level.valueOf(level);
    }

    /**
     * Get the default console appender and add some additional features to it
     *
     * @param level Lowest level for filtering logs
     */
    private void createConsoleAppender (final Level level) {
        // Get the default console appender
        ConsoleAppender<ILoggingEvent> consoleAppender = _contextManager.getConsoleAppender();
        PatternLayoutEncoder encoder = _contextManager.getDefaultPatternLayout();
        RepeatDebounceFilter<ILoggingEvent> logFilter = new RepeatDebounceFilter<>();
        logFilter.setLevel(level);
        logFilter.setTimeBetweenPrints(2000);
        logFilter.start();
        encoder.start();

        consoleAppender.addFilter(logFilter);
        consoleAppender.setEncoder(encoder);
    }

    /**
     * Create a netlogger.logger file appender
     *
     * @param level   Lowest level for filtering logs
     * @param logHome Location of the netlogger.log/ folder
     */
    private void createLogAppender (final Level level, final String logHome) {
        // Create a file appender
        FileAppender<ILoggingEvent> fileAppender = _contextManager.getFileAppender("FILE_LOG");
        PatternLayoutEncoder encoder = _contextManager.getDefaultPatternLayout();
        DefaultLogFilter<ILoggingEvent> logFilter = new DefaultLogFilter<>();

        // Add a date pattern for the log file
        String now = new SimpleDateFormat("MM_dd_yyyy__HH_mm_ss").format(new Date());
        String fileName = logHome +
                "/" + _applicationName + "-" + now + ".log";
        fileAppender.setFile(fileName);

        logFilter.setLevel(level);
        logFilter.start();
        fileAppender.addFilter(logFilter);

        encoder.start();
        fileAppender.setEncoder(encoder);

        fileAppender.start();
    }

    private String _applicationName = "";
    private final ContextManager _contextManager;
}