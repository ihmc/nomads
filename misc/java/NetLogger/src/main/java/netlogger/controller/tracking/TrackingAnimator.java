package netlogger.controller.tracking;

import javafx.animation.*;
import javafx.application.Platform;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.DoubleProperty;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.beans.property.SimpleDoubleProperty;
import javafx.geometry.Point2D;
import javafx.scene.paint.Color;
import javafx.scene.shape.*;
import javafx.util.Duration;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TrackingAnimator
{
    public TrackingAnimator (TrackingObjectPoolManager manager) {
        _pathsToSendOn = new ArrayList<>();
        _nonDuplicatePaths = new ArrayList<>();
        _pathTransitions = new ArrayList<>();

        _refreshRateProperty = new SimpleDoubleProperty(60);
        _pausedProperty = new SimpleBooleanProperty(false);


        Timeline runAnimations = new Timeline(new KeyFrame(Duration.seconds(1), event -> {
            Map<Path, SequentialTransition> pathMap = new HashMap<>();

            _pathsToSendOn.forEach(path -> pathMap.computeIfAbsent(path, k -> new SequentialTransition()));

            pathMap.values().forEach(sequentialTransition -> sequentialTransition.setDelay(Duration.millis(200)));

            // Add a new path transition if there's a new path that's been added
            pathMap.keySet()
                    .stream()
                    .filter(path -> !_nonDuplicatePaths.contains(path))
                    .forEach(path -> {
                        _nonDuplicatePaths.add(path);
                        _pathTransitions.add(getTransition(path));
                    });

            for (int i = 0; i < _pathsToSendOn.size(); i++) {
                Path path = _pathsToSendOn.get(i);
                SequentialTransition sequentialTransition = pathMap.get(path);
                Circle circleToSend = manager.requestCircle();
                circleToSend.setFill(_circleColors.get(i));

                PathTransition transition = new PathTransition();

                int index = _nonDuplicatePaths.indexOf(path);
                PathTransition baseTransition = _pathTransitions.get(index);

                transition.setDuration(baseTransition.getDuration());
                transition.setPath(path);
                transition.setNode(circleToSend);

                sequentialTransition.getChildren().add(transition);

                transition.setOnFinished(event1 -> {
                    manager.returnCircle(circleToSend);
                });
            }

            Platform.runLater(() -> {
                ParallelTransition transition = new ParallelTransition();
                transition.getChildren().addAll(pathMap.values());
                transition.play();
            });
        }));

        runAnimations.setCycleCount(Timeline.INDEFINITE);
        runAnimations.rateProperty().bind(_refreshRateProperty);

        _pausedProperty.addListener(((observable, oldValue, newValue) -> {
            if (newValue && runAnimations.getStatus() == Animation.Status.RUNNING) {
                runAnimations.pause();
            }

            if (!newValue && runAnimations.getStatus() == Animation.Status.PAUSED) {
                runAnimations.play();
            }
        }));
    }

    public void addPath (Path path, Color color) {
        _pathsToSendOn.add(path);
    }


    public DoubleProperty refreshRateProperty () {
        return _refreshRateProperty;
    }

    public BooleanProperty pausedPropertyProperty () {
        return _pausedProperty;
    }

    private PathTransition getTransition (Path path) {
        PathTransition pathTransition = new PathTransition();
        pathTransition.setPath(path);

        Point2D prevPoint = null;
        double totalDistance = 0;
        for (int i = 0; i < path.getElements().size() - 1; i++) {
            PathElement curr = path.getElements().get(i);
            PathElement next = path.getElements().get(i + 1);

            Point2D currPoint = getPoint(curr, prevPoint);
            Point2D nextPoint = getPoint(next, currPoint);

            totalDistance += currPoint.distance(nextPoint);

            prevPoint = currPoint;
        }

        pathTransition.setDuration(javafx.util.Duration.seconds(totalDistance / 150));
        return pathTransition;
    }

    private Point2D getPoint (PathElement val, Point2D prevPoint) {
        if (val instanceof MoveTo) {
            return new Point2D(((MoveTo) val).getX(), ((MoveTo) val).getY());
        }
        else if (val instanceof HLineTo) {
            return new Point2D(((HLineTo) val).getX(), prevPoint.getY());
        }
        else if (val instanceof VLineTo) {
            return new Point2D(prevPoint.getX(), ((VLineTo) val).getY());
        }

        return new Point2D(0, 0);
    }

    private List<Path> _pathsToSendOn;
    private List<Color> _circleColors;

    private final List<Path> _nonDuplicatePaths;
    private final List<PathTransition> _pathTransitions;

    private final DoubleProperty _refreshRateProperty;
    private final BooleanProperty _pausedProperty;


    private static final Logger _logger = LoggerFactory.getLogger(TrackingAnimator.class);
}
