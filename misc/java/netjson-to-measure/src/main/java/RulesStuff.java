import node.NodeMeasureIDRule;
import org.jeasy.rules.api.Facts;
import org.jeasy.rules.api.Rules;
import org.jeasy.rules.api.RulesEngine;
import org.jeasy.rules.core.DefaultRulesEngine;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class RulesStuff
{
    public static void main(String[] args)
    {
        Facts facts = new Facts();
        facts.put("id", "172.16.40.24");

        NodeMeasureIDRule nodeMeasureIDRule = new NodeMeasureIDRule();
        Rules rules = new Rules();
        rules.register(nodeMeasureIDRule);

        RulesEngine rulesEngine = new DefaultRulesEngine();
        rulesEngine.fire(rules, facts);

    }
    private static final Logger _logger = LoggerFactory.getLogger(RulesStuff.class);
}
