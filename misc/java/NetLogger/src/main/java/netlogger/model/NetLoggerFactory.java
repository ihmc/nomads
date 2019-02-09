package netlogger.model;

import javafx.fxml.FXMLLoader;
import netlogger.NetLogger;

public class NetLoggerFactory
{
    public static NetLogger create () {
        NetLogger netLogger;
        FXMLLoader loader = ViewFactory.create("/fxml/MainView.fxml");
        netLogger = loader.getController();
        return netLogger;
    }
}
