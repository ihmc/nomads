package nats;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.stage.Stage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Launcher extends Application
{
    public static void main(String[] args) throws Exception {
        launch(args);
    }

    @Override
    public void start (Stage primaryStage) throws Exception {
        FXMLLoader newLoader = new FXMLLoader();
        newLoader.setLocation(getClass().getResource("/nats/NATSTopicsPane.fxml"));
        newLoader.load();

        NatsTopicsChooser chooser = newLoader.getController();
        chooser.parseTopicsFromPath("../../../../measure/NATSTopics.json");
        chooser.addTopicsToView();

        primaryStage.setScene(new Scene(newLoader.getRoot()));
        primaryStage.show();
    }

    private static final Logger _logger = LoggerFactory.getLogger(Launcher.class);
}
