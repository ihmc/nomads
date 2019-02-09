package netlogger.controller;

import com.jfoenix.controls.JFXTreeView;
import javafx.scene.control.*;
import netlogger.model.messages.MeasureFilter;
import netlogger.view.JFXCheckBoxTreeCell;

import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

public class NATSTopicsManager
{
    public NATSTopicsManager (JFXTreeView<String> hierarchyView, Runnable functionToCall) {
        _resetViewFunction = functionToCall;

        _topicTreeView = hierarchyView;
        _topicTreeView.setRoot(new CheckBoxTreeItem<>("topics"));
        _topicTreeView.setShowRoot(false);
        _topicTreeView.getSelectionModel().setSelectionMode(SelectionMode.SINGLE);

        _topicTreeView.setCellFactory(JFXCheckBoxTreeCell.forTreeView());
    }

    public void setMeasureFilter (MeasureFilter measureFilter) {
        _measureFilter = measureFilter;
    }

    private void filterSingleLevelTopic (TreeItem<String> treeItem) {
        StringBuilder topicBuilder = new StringBuilder();

        if (!isRootChild(treeItem)) {
            buildStringForNodeHierarchy(treeItem.getParent(), topicBuilder);
        }

        _measureFilter.addTopicFilterString(topicBuilder.toString() + treeItem.getValue());
        _resetViewFunction.run();
    }

    private void unfilterSingleLevelTopic (TreeItem<String> treeItem){
        StringBuilder topicBuilder = new StringBuilder();

        if (!isRootChild(treeItem)) {
            buildStringForNodeHierarchy(treeItem.getParent(), topicBuilder);
        }

        _measureFilter.removeTopicFilterString(topicBuilder.toString() + treeItem.getValue());
        _resetViewFunction.run();
    }

    public void handleTopic (String topicName) {
        StringTokenizer tokenizer = new StringTokenizer(topicName, ".");
        TreeItem<String> parentNode = _topicTreeView.getRoot();
        while (tokenizer.hasMoreTokens()) {
            String nextTopic = tokenizer.nextToken();
            TreeItem<String> currNode = null;
            for (TreeItem<String> childNode : parentNode.getChildren()) {
                if (childNode.getValue().equals(nextTopic)) {
                    currNode = childNode;
                    break;
                }
            }

            if (currNode == null) {
                currNode = new CheckBoxTreeItem<>(nextTopic);
                addCheckBoxListener((CheckBoxTreeItem<String>) currNode);
                parentNode.getChildren().add(currNode);
            }
            parentNode = currNode;
        }
    }

    private void addCheckBoxListener(final CheckBoxTreeItem<String> node) {


        node.selectedProperty().addListener((observable, oldValue, newValue) -> {
            if (newValue) {
                filterSingleLevelTopic(node);
            }
            else {
                unfilterSingleLevelTopic(node);
            }
        });
    }

    private void addTopicsBeneathNode (TreeItem<String> rootNode, String currentTopicString, List<String> topicsList) {
        currentTopicString += rootNode.getValue();
        currentTopicString += ".";

        for (TreeItem<String> child : rootNode.getChildren()) {
            if (child.getChildren().isEmpty()) {
                String temp = currentTopicString;
                currentTopicString += child.getValue();
                topicsList.add(currentTopicString);
                currentTopicString = temp;
            }
            else {
                addTopicsBeneathNode(child, currentTopicString, topicsList);
            }
        }
    }


    private void buildStringForNodeHierarchy (TreeItem<String> ancestor, StringBuilder builder) {
        if (isRootChild(ancestor)) {
            builder.append(ancestor.getValue()).append(".");
            return;
        }

        buildStringForNodeHierarchy(ancestor.getParent(), builder);
        builder.append(ancestor.getValue()).append(".");
    }

    private boolean isRootChild (TreeItem<String> node) {
        return node.getParent().equals(_topicTreeView.getRoot());
    }

    private Runnable _resetViewFunction;
    private JFXTreeView<String> _topicTreeView;
    private MeasureFilter _measureFilter;

    private final String _checkMarkString = Character.toString((char) 0x2713);
}
