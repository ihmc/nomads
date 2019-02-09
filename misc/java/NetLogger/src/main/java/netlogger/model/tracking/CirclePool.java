package netlogger.model.tracking;

import javafx.scene.shape.Circle;
import org.reactfx.EventSource;
import org.reactfx.Subscription;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Set;
import java.util.function.Consumer;

public class CirclePool
{
    public CirclePool () {
        _lock = new HashMap<>();
        _unlock = new HashMap<>();

        _circleEvents = new EventSource<>();
    }

    public Circle create () {
        Circle newCircle = new Circle();

        _circleEvents.push(newCircle);

        return newCircle;
    }


    public Subscription addListener (Consumer<Circle> listener) {
        return _circleEvents.subscribe(listener);
    }

    public synchronized Circle takeOut () {
        long now = System.currentTimeMillis();
        if (_unlock.size() > 0) {
            Set<Circle> e = _unlock.keySet();
            for (Circle t : e) {
                _unlock.remove(t);
                _lock.put(t, now);
                return t;
            }
        }

        // Create a new circle
        Circle newCircle = create();
        _lock.put(newCircle, now);
        return newCircle;
    }

    public synchronized void takeIn (Circle circle) {
        _lock.remove(circle);
        _unlock.put(circle, System.currentTimeMillis());
    }

    private HashMap<Circle, Long> _lock;
    private HashMap<Circle, Long> _unlock;

    private EventSource<Circle> _circleEvents;

    private static final Logger _logger = LoggerFactory.getLogger(CirclePool.class);
}
