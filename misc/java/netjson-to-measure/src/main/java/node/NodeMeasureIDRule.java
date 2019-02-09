package node;

import org.jeasy.rules.annotation.Action;
import org.jeasy.rules.annotation.Condition;
import org.jeasy.rules.annotation.Fact;
import org.jeasy.rules.annotation.Rule;
import org.jeasy.rules.api.Facts;
import org.jetbrains.annotations.NotNull;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Rule(name = "Node measure id rule", description = "Rule for when a node measure should have an id")
public class NodeMeasureIDRule implements org.jeasy.rules.api.Rule
{
    public String getId () {
        return _id;
    }

    @Condition
    public boolean setID(@Fact("_id") String nodeID) {
        _id = nodeID;
        return true;
    }

    @Action
    public void doSomeStuff(){
        System.out.println("It's json, convert to measure");
    }

    @Override
    public String getName () {
        return "NodeMeasureIDRule";
    }

    @Override
    public String getDescription () {
        return null;
    }

    @Override
    public int getPriority () {
        return 0;
    }

    @Override
    public boolean evaluate (Facts facts) {
        return false;
    }

    @Override
    public void execute (Facts facts) throws Exception {

    }

    @Override
    public int compareTo (@NotNull org.jeasy.rules.api.Rule o) {
        return 0;
    }

    private String _id;
    private static final Logger _logger = LoggerFactory.getLogger(NodeMeasureIDRule.class);
}
