package netlogger.controller.tracking;

import javafx.animation.*;
import javafx.scene.shape.Circle;
import javafx.util.Duration;
import netlogger.model.tracking.CirclePool;
import org.reactfx.EventSource;
import org.reactfx.Subscription;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.function.Consumer;

/**
 * Because animations use a "moveTo()" call at the startTimeline, it's possible to reuse circles without instantiating them
 * This way we also are able to manager who gets what circles based on some determining factors.
 */
public class TrackingObjectPoolManager
{
    public TrackingObjectPoolManager () {
        _circlePool = new CirclePool();
    }

    /**
     * Gets a circle from the circle pool
     *
     * @return A circle
     */
    public Circle requestCircle () {
        Circle circle = _circlePool.takeOut();
        return circle;
    }

    /**
     * Returns a circle to the circle pool and removes it from view
     *
     * @param circleToReturn Circle to recycle
     */
    public void returnCircle (Circle circleToReturn) {

        playTransitionsForRemovingCircle(circleToReturn);
    }

    private void playTransitionsForRemovingCircle (Circle circle) {
        FadeTransition fadeTransition = new FadeTransition();
        fadeTransition.setNode(circle);
        fadeTransition.setDuration(Duration.millis(1000));

        fadeTransition.setToValue(0.0);
        fadeTransition.setFromValue(1.0);

        Timeline timeline = new Timeline();
        KeyValue kv = new KeyValue(circle.radiusProperty(), 0);
        KeyFrame kf = new KeyFrame(Duration.millis(1000), kv);
        timeline.getKeyFrames().add(kf);

        ParallelTransition parallelTransition = new ParallelTransition(fadeTransition, timeline);
        parallelTransition.play();

        parallelTransition.setOnFinished(event -> {
            _circlePool.takeIn(circle);
        });
    }

    public Subscription addListener (Consumer<Circle> listener) {
        return _circlePool.addListener(listener);
    }

    private CirclePool _circlePool;
    private EventSource<Circle> _circleEvents;

    private static final Logger _logger = LoggerFactory.getLogger(TrackingObjectPoolManager.class);
}
