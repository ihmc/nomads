package netlogger.controller;

import javafx.application.Platform;
import javafx.fxml.FXML;
import javafx.scene.layout.VBox;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import javafx.scene.text.Text;
import javafx.scene.text.TextFlow;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Set;
import java.util.concurrent.atomic.AtomicLong;

public class SubjectCounter implements Runnable
{
    @FXML
    public void initialize(){
        _topicCountMap = new HashMap<>();
    }

    @Override
    public void run () {
        while (!_terminationRequested) {
            try {
                Thread.sleep(3000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Platform.runLater(() -> {
                _subjectsVBox.getChildren().clear();
                Set<String> topicsList = _topicCountMap.keySet();
                for (String topic : topicsList) {
                    AtomicLong counter = _topicCountMap.get(topic);
                    TextFlow textFlow = new TextFlow();
                    Text subjectNameText = new Text(topic + ": ");
                    subjectNameText.setFont(Font.font("Helvetica", FontWeight.BOLD, 12));

                    textFlow.getChildren().add(subjectNameText);
                    textFlow.getChildren().add(new Text("" + counter.get()));

                    _subjectsVBox.getChildren().add(textFlow);
                }
            });
        }
    }

    public void requestTermination () {
        _terminationRequested = true;
    }

    public void subjectReceived (String subject) {

        AtomicLong count = _topicCountMap.get(subject);
        if (count == null) {
            count = new AtomicLong(0);
            _topicCountMap.put(subject, count);
        }


        count.incrementAndGet();
    }


    private boolean _terminationRequested;
    private HashMap<String, AtomicLong> _topicCountMap;
    @FXML private VBox _subjectsVBox;

    private static final Logger _logger = LoggerFactory.getLogger(SubjectCounter.class);
}
