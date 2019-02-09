package netlogger.model;

import com.jfoenix.controls.JFXButton;
import com.jfoenix.controls.JFXTextField;
import elasticsearch.ElasticsearchIndexStoreManager;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.embed.swing.SwingFXUtils;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Cursor;
import javafx.scene.Scene;
import javafx.scene.SnapshotParameters;
import javafx.scene.control.Alert;
import javafx.scene.control.Label;
import javafx.scene.control.Tooltip;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.image.WritableImage;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import javafx.scene.web.WebEngine;
import javafx.scene.web.WebView;
import javafx.stage.FileChooser;
import javafx.stage.Screen;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import netlogger.Launcher;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;

public class KibanaLauncher extends Application
{
    public KibanaLauncher (String url, double x, double y) {
        _kibanaUrl = url;
        _stageX = x;
        _stageY = y;

        // Assumed port is 9200
        String hostPort = _kibanaUrl.split("http://")[1];
        String host = hostPort.split(":")[0];
        int port = 9200;

        _manager = new ElasticsearchIndexStoreManager();
        _manager.init(host, port, "http");
    }

    @Override
    public void start (Stage primaryStage) throws Exception {
        _stage = primaryStage;

        _browserRegion = new BrowserRegion();
        _browserRegion.load(_kibanaUrl);
        final double width = Screen.getPrimary().getVisualBounds().getWidth() - 20;
        final double height = Screen.getPrimary().getVisualBounds().getHeight() - 20;

        VBox vBox = new VBox();
        vBox.setPrefHeight(height);
        HBox functionalityHBox = createHBox();
        final double hboxHeight = 50;
        final double browserHeight = vBox.getPrefHeight() - hboxHeight;

        functionalityHBox.setPrefHeight(hboxHeight);
        _browserRegion.resizeBrowser(width, browserHeight);

        vBox.getChildren().addAll(functionalityHBox, _browserRegion);

        _stage.initStyle(StageStyle.DECORATED);
        _stage.setTitle("Kibana");
        _stage.setScene(new Scene(vBox));
        _stage.setX(_stageX);
        _stage.setY(_stageY);
        _stage.setMaximized(true);

        _stage.heightProperty().addListener((observable, oldValue, newValue) -> {
            _browserRegion.setNewHeight(newValue.doubleValue() - hboxHeight - 20);
        });

        _stage.widthProperty().addListener((observable, oldValue, newValue) -> {
            _browserRegion.setNewWidth(newValue.doubleValue() - 20);
        });

        _stage.show();
    }


    private HBox createHBox () {
        HBox hBox = new HBox();
        JFXButton screenshotButton = new JFXButton("Take picture");
        JFXTextField screenshotNameText = new JFXTextField();
        screenshotButton.setOnMouseClicked(event -> {
            getKibanaSnapshot(screenshotNameText.getText());
        });

        screenshotButton.setStyle("-fx-background-color: #7399c6; -fx-text-fill: #fff;\n");

        JFXButton saveAndClearCurrentData = new JFXButton();
        saveAndClearCurrentData.setCursor(Cursor.HAND);
        Image image = new Image(Launcher.class.getResourceAsStream("save-icon-50.png"));
        ImageView imageView = new ImageView(image);
        imageView.setFitHeight(16);
        imageView.setFitWidth(16);
        saveAndClearCurrentData.setGraphic(imageView);
        saveAndClearCurrentData.setTooltip(new Tooltip("Save the _data to an index specified in the text field to the right"));
        saveAndClearCurrentData.setStyle("-fx-background-color: #7399c6;\n");


        JFXTextField newIndexNameText = new JFXTextField();
        saveAndClearCurrentData.setOnMouseClicked(event -> {
            if (newIndexNameText.getText().isEmpty()) {
                Alert alert = new Alert(Alert.AlertType.CONFIRMATION);
                alert.setTitle("Notice");
                alert.setHeaderText("Fill in the text box for the name of saved _data");
                alert.setContentText("You need to set what type of data this should be saved as (duplicate names not allowed)");
                alert.show();
                return;
            }
            else {
                Alert alert = new Alert(Alert.AlertType.CONFIRMATION);
                alert.setTitle("Notice");
                alert.setHeaderText("This will delete all previous data in the ");
                saveData(newIndexNameText.getText().toLowerCase());
            }
        });

        hBox.setPadding(new Insets(0, 0, 0, 20));
        hBox.setSpacing(10);
        hBox.getChildren().addAll(screenshotButton, screenshotNameText, saveAndClearCurrentData, newIndexNameText);
        return hBox;
    }

    private void saveData (String newIndexName) {
        if (_manager.indexExists(newIndexName)) {
            Alert alert = new Alert(Alert.AlertType.ERROR);
            alert.setTitle("Error");
            alert.setHeaderText("Name in use");
            alert.setContentText("The index " + newIndexName + " is already in use. Please use a different one");
            alert.show();
            return;
        }
        new Thread(() -> {
            _manager.storeDataToIndex("stockbridge", newIndexName);
            _manager.awaitFinish();
            // Close the manager after it has finished
            try {
                _manager.close();
            } catch (IOException e) {
                e.printStackTrace();
            }


            Platform.runLater(() -> {
                Alert alert = new Alert(Alert.AlertType.INFORMATION);
                alert.setTitle("Finished");
                alert.setHeaderText("Elasticsearch copy finished");
                alert.show();
            });

        }, "Store to table thread").start();
    }

    public void close () throws IOException {
        _manager.close();
    }

    private void getKibanaSnapshot (String screenshotName) {
        BufferedImage bufferedImage = new BufferedImage(1000, 600, BufferedImage.TYPE_INT_ARGB);

        WritableImage screenCapture = _browserRegion.snapshot(new SnapshotParameters(), null);
        BufferedImage image = SwingFXUtils.fromFXImage(screenCapture, bufferedImage);

        Stage saveImageStage = new Stage();
        VBox vbox = new VBox();
        Label name = new Label(screenshotName);
        name.setFont(Font.font("Helvetica", FontWeight.BOLD, 18));
        JFXButton cancelBtn = new JFXButton("Cancel");
        JFXButton saveBtn = new JFXButton("Save");

        cancelBtn.setOnAction(event -> {
            saveImageStage.close();
        });

        saveBtn.setOnAction(event -> {
            FileChooser fileChooser = new FileChooser();
            fileChooser.setTitle("Save image");
            fileChooser.getExtensionFilters().addAll(
                    new FileChooser.ExtensionFilter("PNG", "*.png"));

            fileChooser.setInitialFileName(screenshotName);
            File file = fileChooser.showSaveDialog(saveImageStage);
            if (file != null) {
                try {
                    String extension = "";
                    int index = file.getCanonicalPath().lastIndexOf('.');
                    if (index > 0) {
                        extension = file.getCanonicalPath().substring(index + 1);
                    }

                    ImageIO.write(image, extension, file);
                } catch (IOException ignored) {
                }

                saveImageStage.close();
            }
        });

        vbox.setSpacing(10);
        ImageView imageView = new ImageView(screenCapture);
        imageView.setFitWidth(1000);
        imageView.setFitHeight(600);
        imageView.setPreserveRatio(true);

        vbox.getChildren().addAll(name, imageView, new HBox(cancelBtn, saveBtn));
        vbox.setAlignment(Pos.CENTER);
        vbox.setPadding(new Insets(20, 0, 0, 0));

        saveImageStage.setTitle(screenshotName);
        saveImageStage.initStyle(StageStyle.UTILITY);
        saveImageStage.setScene(new Scene(vbox, imageView.getFitWidth(), imageView.getFitHeight() + 50));
        saveImageStage.show();
    }

    public static void main (String[] args) {
        launch(args);
    }

    private class BrowserRegion extends Region
    {
        private WebView _browser = new WebView();
        private WebEngine _webEngine = _browser.getEngine();

        public BrowserRegion () {
            getChildren().add(_browser);
        }

        public void setNewHeight (double newHeight) {
            resizeBrowser(_browser.getPrefWidth(), newHeight);
        }

        public void setNewWidth (double newWidth) {
            resizeBrowser(newWidth, _browser.getPrefHeight());
        }

        private void resizeBrowser (double width, double height) {
            _browser.setPrefSize(width, height);
            _browser.resize(width, height);
        }

        public void load (String url) {
            _webEngine.load(url);
        }
    }

    private double _stageX;
    private double _stageY;
    private String _kibanaUrl;
    private Stage _stage;
    private ElasticsearchIndexStoreManager _manager;
    private BrowserRegion _browserRegion;
    private static final Logger _logger = LoggerFactory.getLogger(KibanaLauncher.class);
}
