package nats;

import javafx.collections.ObservableList;
import javafx.scene.control.TreeItem;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;

public class NatsTopicsHelper
{
    public NatsTopicsHelper (){
        _root = new MarkableTreeItem<>("root");
    }

    public NatsTopicsHelper (MarkableTreeItem<String> root){
        _root = root;
    }

    public void handleTopic(String topicName)
    {
        StringTokenizer tokenizer = new StringTokenizer(topicName, ".");
        TreeItem<String> parentNode = _root;
        while (tokenizer.hasMoreTokens()) {
            String nextTopic = tokenizer.nextToken();
            TreeItem<String> currNode = null;
            for(TreeItem<String> childNode : parentNode.getChildren()){
                if (childNode.getValue().equals(nextTopic)){
                    currNode = childNode;
                    break;
                }
            }

            if (currNode == null){
                currNode = new MarkableTreeItem<>(nextTopic);
                parentNode.getChildren().add(currNode);
            }
            parentNode = currNode;
        }
    }

    public void addTopicsBeneathNode(MarkableTreeItem<String> rootNode, String currentTopicString, List<String> topicsList)
    {
        currentTopicString += rootNode.getValue();
        currentTopicString += ".";

        for (TreeItem<String> child : rootNode.getChildren()){
            if (child.getChildren().isEmpty()){
                String temp = currentTopicString;
                currentTopicString += child.getValue();
                topicsList.add(currentTopicString);
                currentTopicString = temp;
            }
            else {
                addTopicsBeneathNode((MarkableTreeItem<String>)child, currentTopicString, topicsList);
            }
        }
    }

    public void markNodeByTopic(String topic){
        StringTokenizer tokenizer = new StringTokenizer(topic, ".");
        MarkableTreeItem<String> parentNode = _root;
        while (tokenizer.hasMoreTokens()) {
            String nextTopic = tokenizer.nextToken();
            TreeItem<String> currNode = null;
            for(TreeItem<String> childNode : parentNode.getChildren()){
                if (childNode.getValue().equals(nextTopic)){
                    currNode = childNode;
                    break;
                }
            }

            if (currNode == null){
                _logger.error("Curr node was null? Only possible if user inputs a node and it isn't updated in the tree");
                return;
            }

            parentNode = (MarkableTreeItem<String>)currNode;
            parentNode.mark();
        }
    }


    /**
     * Compares topics based on NATS naming conventions to determine if the topics are 'the same'
     * @param topic1
     * @param topic2
     * @return
     */
    public static boolean compareTopics(String topic1, String topic2){
        if (topic1.equals(topic2)){
            return true;
        }

        StringTokenizer topic1Tokenizer = new StringTokenizer(topic1, ".");
        StringTokenizer topic2Tokenizer = new StringTokenizer(topic2, ".");

        while (topic1Tokenizer.hasMoreTokens() && topic2Tokenizer.hasMoreTokens()){
            String topic1Level = topic1Tokenizer.nextToken();
            String topic2Level = topic2Tokenizer.nextToken();

            if (topic1Level.equals(">") || topic2Level.equals(">")){
                return true;
            }


            if (topic1Level.equals("*") || topic2Level.equals("*")){
                continue;
            }


            if (!topic1Level.equals(topic2Level)){
                return false;
            }
        }

        return true;
    }

    /**
     * Topics are valid as long as the first index is not ">" and there's no text after ">"
     * @param topic
     * @return
     */
    public boolean isValidTopic(String topic){
        if (topic.indexOf(">") == 0){
            return false;
        }

        return topic.indexOf(">") == topic.length() - 1;
    }


    public List<String> createWildCardTopics() {
        List<String> vals = new ArrayList<>();
        String currTopic = "";

        Stack<MarkableTreeItem<String>> items = new Stack<>();
        items.add(_root);
        _root.setVisited(true);
        while (!items.isEmpty()) {
            MarkableTreeItem<String> node = items.peek();
            if (node.isMarked()){
                currTopic += node.getValue() + ".";
            }

            MarkableTreeItem<String> child = getUnvisitedChildNode(node);
            if (child != null) {
                child.setVisited(true);
                items.push(child);
            }
            else {
                vals.add(currTopic.substring(0, currTopic.length() - 1));
                currTopic = "";
                items.pop();
            }
        }

        return vals;
    }


    public void getReversedBreadthTree (Stack<MarkableTreeItem<String>> visitedNodes) {
        Queue<MarkableTreeItem<String>> items = new LinkedList<>();
        // No need to add the root to the stack since it's not a NATS topic qualifier
        items.add(_root);
        _root.setVisited(true);

        while (!items.isEmpty()){
            MarkableTreeItem<String> node = items.remove();
            MarkableTreeItem<String> child;
            while ((child = getUnvisitedChildNode(node)) != null){
                child.setVisited(true);
                items.add(child);
                visitedNodes.push(child);
            }
        }

        for (MarkableTreeItem<String> item : visitedNodes){
            item.setVisited(false);
        }
    }

    private MarkableTreeItem<String> getUnvisitedChildNode(MarkableTreeItem<String> node){
        for (TreeItem<String> child : node.getChildren()){
            if (!((MarkableTreeItem<String>)child).isVisited()){
                return ((MarkableTreeItem<String>)child);
            }
        }
        return null;
    }


    private ObservableList<TreeItem<String>> getSiblingsOfNode(MarkableTreeItem<String> node){
        return node.getParent().getChildren();
    }



    public void buildTopicFromParent (MarkableTreeItem<String> ancestor, StringBuilder builder)
    {
        // Check if this is the second child. Root child doesn't show (so we don't start nats topics at the second children)
        if (isSecondChild(ancestor)) {
            builder.append(ancestor.getValue()).append(".");
            return;
        }

        buildTopicFromParent((MarkableTreeItem<String>)ancestor.getParent(), builder);
        builder.append(ancestor.getValue()).append(".");
    }

    private boolean isSecondChild (MarkableTreeItem<String> node)
    {
        return node.getParent().equals(_root);
    }

    public MarkableTreeItem<String> getRootNode () {
        return _root;
    }

    private MarkableTreeItem<String> _root;


    private static final Logger _logger = LoggerFactory.getLogger(NatsTopicsHelper.class);
}
