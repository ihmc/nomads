package nats;

import com.jfoenix.controls.JFXCheckBox;
import javafx.application.Platform;
import javafx.fxml.FXML;
import javafx.scene.control.CheckBox;
import javafx.scene.control.Label;
import javafx.scene.control.Tooltip;
import javafx.scene.image.ImageView;
import javafx.scene.layout.VBox;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public class NatsTopicsList
{
    public void addTopics (List<Topic> topics) {

        Platform.runLater(() -> {
            AtomicInteger countSelected = new AtomicInteger(0);
            topics.forEach(topic -> {
                String topicName = topic.getName().orElse("?");
                JFXCheckBox checkBox = new JFXCheckBox(topicName);
                checkBox.setMnemonicParsing(false);
                checkBox.selectedProperty().addListener((observable, oldValue, newValue) -> {
                    if (newValue) {
                        _chosenTopics.add(topicName);
                        if (countSelected.incrementAndGet() == topics.size()) {
                            _selectAllCheck.setSelected(true);
                        }
                    }
                    else {
                        _chosenTopics.remove(topicName);

                        countSelected.decrementAndGet();
                        if (_selectAllCheck.isSelected()) {
                            _selectAllCheck.setSelected(false);
                        }
                    }
                });
                checkBox.setTooltip(new Tooltip(topic.getDescription().orElse("?")));

                _checkBoxList.add(checkBox);

                _appTopicVBox.getChildren().add(checkBox);
            });
        });
    }

    public List<String> getChosenTopics () {
        if (_chosenTopics.size() == _checkBoxList.size() && (_checkBoxList.size() != 0)){
            return Collections.singletonList(_nameLabel.getText() + ".>");
        }
        return _chosenTopics;
    }

    @FXML
    public void initialize () {
        _checkBoxList = new ArrayList<>();
        _chosenTopics = new ArrayList<>();

        _selectAllCheck.setOnMouseClicked(event -> {
            if (_selectAllCheck.isSelected()) {
                selectAll(true);
            }
            else {
                selectAll(false);
            }
        });
    }

    public void selectAll (final boolean val) {
        _checkBoxList.forEach(checkBox -> {
            checkBox.setSelected(val);
        });
    }

    public void selectValues(List<String> selectedValues){
        Platform.runLater(() -> {
            _checkBoxList.forEach(checkBox -> {
                for (String val : selectedValues) {
                    if (NatsTopicsHelper.compareTopics(val, checkBox.getText())) {
                        checkBox.setSelected(true);
                    }
                }
            });
        });

        ImageView imageView = new ImageView();
        imageView.setVisible(false);
    }

    public void setName (String name) {
        _nameLabel.setText(name);
    }

    private static final Logger _logger = LoggerFactory.getLogger(NatsTopicsList.class);
    private List<CheckBox> _checkBoxList;
    private List<String> _chosenTopics;
    @FXML
    private Label _nameLabel;
    @FXML
    private JFXCheckBox _selectAllCheck;
    @FXML
    private VBox _appTopicVBox;
}
