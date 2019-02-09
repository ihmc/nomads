package nats;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Optional;

public class Topic{
    public Topic(String name, String description){
        _name = name;
        _description = description;
    }

    public Optional<String> getName(){
        return Optional.ofNullable(_name);
    }

    public Optional<String> getDescription(){
        return Optional.ofNullable(_description);
    }

    private String _name;
    private String _description;

    private static final Logger _logger = LoggerFactory.getLogger(Topic.class);
}
