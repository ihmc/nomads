package netlogger;

import javafx.application.Application;
import javafx.stage.Stage;
import netlogger.logger.LoggerInitializerImpl;
import netlogger.model.NetLoggerFactory;
import netlogger.util.ConfigFileManager;
import netlogger.util.params.CommandLineParser;
import netlogger.util.params.CommandLineValues;

public class Launcher extends Application
{
    @Override
    public void start (Stage primaryStage) throws Exception {
        Object[] params = getParameters().getRaw().toArray();
        String[] obj = new String[params.length];
        for (int i = 0; i < params.length; i++) {
            obj[i] = (String) params[i];
        }

        CommandLineParser parser = new CommandLineParser();
        CommandLineValues clv = parser.parse(obj);

        LoggerInitializerImpl initializer = new LoggerInitializerImpl();
        initializer.initialize(clv.fileDebugLevel, clv.consoleDebugLevel, clv.logHome, "NetLogger");

        ConfigFileManager configFileManager = new ConfigFileManager();
        configFileManager.init(clv.configFile);

        NetLogger netLogger = NetLoggerFactory.create();
        netLogger.createSettings();
        netLogger.setConfigManager(configFileManager);
        netLogger.initializeAsStandalone();
        netLogger.startThreads();

    }


    public static void main (String[] args) {
        launch(args);
    }
}
