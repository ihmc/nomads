package nats;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.jfoenix.controls.JFXButton;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.scene.control.Button;
import javafx.scene.control.ScrollPane;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.VBox;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class NatsTopicsChooser
{
    public void addTopicsToView () {
        for (String appName : _topicsByApp.keySet()) {
            FXMLLoader newLoader;
            try {
                newLoader = new FXMLLoader();
                newLoader.setLocation(getClass().getResource("/nats/NATSTopicsByAppList.fxml"));
                newLoader.load();
            } catch (IOException e) {
                e.printStackTrace();
                continue;
            }
            _topicVBox.getChildren().add(newLoader.getRoot());

            NatsTopicsList list = newLoader.getController();

            List<Topic> appTopics = _topicsByApp.get(appName);
            list.setName(appName);
            list.addTopics(appTopics);

            _appNameTopicsList.put(appName, list);
        }
    }

    public List<String> getChosenTopics () {
        List<String> chosenTopics = new ArrayList<>();
        _appNameTopicsList.values().forEach(natsTopicsList -> {
            chosenTopics.addAll(natsTopicsList.getChosenTopics());
        });
        return chosenTopics;
    }

    @FXML
    public void initialize () {
        _helper = new NatsTopicsHelper();
        _topicsByApp = new HashMap<>();
        _appNameTopicsList = new HashMap<>();

        _clearBtn.setOnMouseClicked(event -> {
            _appNameTopicsList.values().forEach(natsTopicsList -> {
                natsTopicsList.selectAll(false);
            });
        });
    }

    /**
     * Reads the json file from the specified path and parses the topics
     * @param path
     */
    public void parseTopicsFromPath (String path) {
        long start = System.currentTimeMillis();
        JsonElement val;

        try {
            val = new JsonParser().parse(new FileReader(path));
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return;
        }
        if (val == null) {
            return;
        }

        long parseTime = System.currentTimeMillis() - start;

        JsonArray appNames = val.getAsJsonArray();
        appNames.iterator().forEachRemaining(jsonElement -> {
            JsonObject appNameObject = jsonElement.getAsJsonObject();
            appNameObject.entrySet().forEach(stringJsonElementEntry -> {
                final String appName = stringJsonElementEntry.getKey();
                List<Topic> appTopics = _topicsByApp.computeIfAbsent(appName, n -> new ArrayList<>());

                JsonArray topicNameArray = stringJsonElementEntry.getValue().getAsJsonArray();

                topicNameArray.iterator().forEachRemaining(topicObjEle -> {
                    JsonObject topicObj = topicObjEle.getAsJsonObject();

                    String topicName = topicObj.get("name").getAsString();
                    String description = topicObj.get("description").getAsString();

                    appTopics.add(new Topic(topicName, description));
                });
            });
        });

        long end = System.currentTimeMillis();

        long total = end - start;

        _logger.info("Parse time {}", total);
    }

    public void defaultSelectTopics (List<String> topics){
        _appNameTopicsList.values().forEach(topicsList -> {
            topicsList.selectValues(topics);
        });
    }

    public void setButtonListener (EventHandler<MouseEvent> mouseEvent) {
        _saveBtn.setOnMouseClicked(mouseEvent);
    }

    private static final Logger _logger = LoggerFactory.getLogger(NatsTopicsChooser.class);
    private NatsTopicsHelper _helper;

    @FXML
    private BorderPane _borderPane;
    @FXML
    private ScrollPane _scrollPane;
    @FXML
    private VBox _topicVBox;
    @FXML
    private JFXButton _clearBtn;
    @FXML
    private Button _saveBtn;
    private Map<String, List<Topic>> _topicsByApp;

    private Map<String, NatsTopicsList> _appNameTopicsList;
}
