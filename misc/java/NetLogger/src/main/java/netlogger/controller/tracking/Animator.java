package netlogger.controller.tracking;

import ch.qos.logback.classic.LoggerContext;
import ch.qos.logback.core.util.StatusPrinter;
import com.google.protobuf.Timestamp;
import javafx.animation.PathTransition;
import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.stage.Stage;
import netlogger.model.tracking.TrackingMeasure;
import netlogger.model.ViewFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;

public class Animator extends Application
{
    static {
        System.setProperty("logback.configurationFile", "logback.xml");
    }

    public Animator () {

    }

    public static void main (String[] args) {
        LoggerContext lc = (LoggerContext) LoggerFactory.getILoggerFactory();
        StatusPrinter.print(lc);

        launch(args);
    }

    @Override
    public void start (Stage primaryStage) throws Exception {
        FXMLLoader loader = ViewFactory.create("/fxml/LogTracking.fxml");
        TrackingController trackingController = loader.getController();
        trackingController.setTrackingConfigFile("../../conf/netlogger_tracking_paths.properties");

        primaryStage.setScene(new Scene(loader.getRoot(), 1000, 700));
        primaryStage.show();

        String[] applications = new String[]{"Federation", "DSPro", "ATAK", "Test"};

        new Thread(() -> {
            int counter = 0;
            while (true) {
//                String randomSource = applications[ThreadLocalRandom.current().nextInt(0, applications.length)];
//                String randomDest = applications[ThreadLocalRandom.current().nextInt(0, applications.length)];

                String randomSource = applications[1];
                String randomDest = applications[0];

                TrackingMeasure measure = new TrackingMeasure(randomSource, randomSource,
                        randomDest, randomDest,
                        String.valueOf(counter), String.valueOf(counter), "",
                        "track", "1", "1", Timestamp.newBuilder().build());
                trackingController.handleDataTrackingMeasure(measure);
                counter++;
                try {
                    Thread.sleep(2550);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }, "Tracking measure generation thread").start();
    }


    private static final Logger _logger = LoggerFactory.getLogger(Animator.class);
    private List<PathTransition> _animations;
}
