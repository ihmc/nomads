package netlogger.model;

import javafx.fxml.FXMLLoader;
import netlogger.Launcher;

import java.io.IOException;

public class ViewFactory
{
    public static FXMLLoader create (String fxmlPath) {
        FXMLLoader loader = new FXMLLoader(Launcher.class.getResource(fxmlPath));
        try {
            loader.load();
        } catch (IOException e) {

        }

        return loader;
    }
}
