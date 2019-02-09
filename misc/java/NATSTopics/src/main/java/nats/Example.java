package nats;

// Connect nodes at same level using level order
// traversal.
import javafx.scene.control.TreeItem;

import java.util.*;

public class Example
{
    private static MarkableTreeItem<String> _root = new MarkableTreeItem<>("netsupervisor");

    public static void handleTopic(String topicName)
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

    public static void markNodeByTopic(String topic){
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
                return;
            }

            parentNode = (MarkableTreeItem<String>)currNode;
            parentNode.mark();
        }
    }

    // Sets nextRight of all nodes of a tree
    static void connect (MarkableTreeItem<String> root) {
        Queue<MarkableTreeItem<String>> q = new LinkedList<>();
        q.add(root);

        // null marker to represent end of current level
        q.add(null);

        // Do Level order of tree using NULL markers
        while (!q.isEmpty()) {
            MarkableTreeItem<String> p = q.poll();
            if (p != null) {

                // next element in queue represents next
                // node at current Level
                p.setNextRight(q.peek());

                // push left and right children of current
                // node
                if (p.getChildren().size() > 0) {
                    List<MarkableTreeItem<String>> list = new ArrayList<>();

                    p.getChildren().forEach(stringTreeItem -> {
                        list.add((MarkableTreeItem<String>)stringTreeItem);
                    });

                    q.addAll(list);
                }
            }

            // if queue is not empty, push NULL to mark
            // nodes at this level are visited
            else if (!q.isEmpty())
                q.add(null);
        }
    }

    public static Stack<MarkableTreeItem<String>> getReversedBreadthTree () {
        Stack<MarkableTreeItem<String>> visitedNodes = new Stack<>();
        Queue<MarkableTreeItem<String>> items = new LinkedList<>();
        // No need to add the root to the stack since it's not a NATS topic qualifier
        items.add(_root);
        _root.setVisited(true);

        while (!items.isEmpty()){
            MarkableTreeItem<String> node = items.remove();
            MarkableTreeItem<String> child;
            while ((child = getUnvisitedChildNodeReverse(node)) != null){
                child.setVisited(true);
                items.add(child);
                visitedNodes.push(child);
            }
        }

        for (MarkableTreeItem<String> item : visitedNodes){
            item.setVisited(false);
        }

        return visitedNodes;
    }

    private static MarkableTreeItem<String> getUnvisitedChildNodeReverse (MarkableTreeItem<String> node){
        List<TreeItem<String>> reversedList = new ArrayList<>(node.getChildren());
        Collections.reverse(reversedList);
        for (TreeItem<String> child : reversedList){
            if (!((MarkableTreeItem<String>)child).isVisited()){
                return ((MarkableTreeItem<String>)child);
            }
        }
        return null;
    }

    /* Driver program to test above functions*/
    public static void main (String args[]) {
        handleTopic("summary.subnet.link_description");
        handleTopic("summary.group.link_description");
        handleTopic("summary.netproxy.link_description");
        handleTopic("summary.traffic");
        handleTopic("aggregate.topology.node");
        handleTopic("aggregate.topology.edge");
        connect(_root);

        markNodeByTopic("netsupervisor.summary.subnet.link_description");
        markNodeByTopic("netsupervisor.summary.netproxy.link_description");
        markNodeByTopic("netsupervisor.summary.group.link_description");
        markNodeByTopic("netsupervisor.summary.traffic");

       Stack<MarkableTreeItem<String>> stack = getReversedBreadthTree();
       while (!stack.isEmpty()){
            MarkableTreeItem<String> item = stack.pop();
            Map<String, Boolean> possibleValuesOnLevel = new HashMap<>();

            while (item.getNextRight() != null){
                item = item.getNextRight();

                possibleValuesOnLevel.putIfAbsent(item.getValue(), Boolean.TRUE);

                if (item.isMarked()) {
                    possibleValuesOnLevel.put(item.getValue(), Boolean.FALSE);
                }
            }

            boolean allMarked = true;
            for (String valuesOnLevel : possibleValuesOnLevel.keySet()){
                // All of these values are marked! We can do a * for the parent
                if (possibleValuesOnLevel.get(valuesOnLevel)) {

                }
                // Must do specific parent values for this value
                else {
                    allMarked = false;
                }
            }

            // We can do a > after the parent's parent
            if (allMarked){

            }


       }



    }
}