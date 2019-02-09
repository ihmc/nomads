package netlogger.controller.tracking;

import javafx.geometry.Point2D;
import javafx.scene.paint.Color;
import javafx.scene.shape.HLineTo;
import javafx.scene.shape.MoveTo;
import javafx.scene.shape.Path;
import javafx.scene.shape.VLineTo;
import netlogger.util.tracking.TrackingConfigManager;
import netlogger.view.tracking.ModuleView;
import org.reactfx.EventSource;
import org.reactfx.Subscription;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Map;
import java.util.function.Consumer;

/**
 * This class calculates the path between modules center points using perpendicular lines and not intersecting other modules.
 * This calculation will be done whenever a new module view is used
 */
public class TrackingPathCalculator
{
    public TrackingPathCalculator () {
        _moduleViewsAppNames = new HashMap<>();
        _pathsWithNames = new HashMap<>();

        _paths = new EventSource<>();
        _removedPaths = new EventSource<>();

    }

    public void setTrackingConfigPath (String path) {
        _trackingConfigManager = new TrackingConfigManager(path);
    }

    /**
     * Add a listener for when new paths are added into the event stream
     *
     * @param consumer
     */
    public Subscription addAddListener (Consumer<Path> consumer) {
        return _paths.subscribe(consumer);
    }

    public Subscription addRemoveListener (Consumer<Path> consumer) {
        return _removedPaths.subscribe(consumer);
    }

    public void addNewModuleView (ModuleView moduleView, String appName) {
        moduleView.layoutXProperty().addListener((observable, oldValue, newValue) -> {
            calculatePathsFor(moduleView, appName);
        });
        moduleView.layoutYProperty().addListener((observable, oldValue, newValue) -> {
            calculatePathsFor(moduleView, appName);
        });

        _moduleViewsAppNames.put(appName, moduleView);
    }

    /**
     * Get the path between two module views.
     *
     * @param app1 First
     * @param app2 Second
     * @return Path between
     */
    public Path getPath (String app1, String app2) {
        String storedPathID = generatePathID(app1, app2);
        return _pathsWithNames.get(storedPathID);
    }

    /**
     * Creates a naive path to the other stack pane and stores it for future reference
     *
     * @param moduleView1
     * @param moduleView2
     */
    private Path calculatePathBetween (ModuleView moduleView1, ModuleView moduleView2) {
        double angle = getAngleBetweenCenters(moduleView1, moduleView2);

        Path path = new Path();

        Point2D ourCenter = getCenter(moduleView1);

        Point2D theirCenter = getCenter(moduleView2);

        Point2D midPoint = new Point2D(
                (ourCenter.getX() + theirCenter.getX()) * 0.5,
                (ourCenter.getY() + theirCenter.getY()) * 0.5);

        // Put this in the center of our stackpane/rectangle
        path.getElements().add(new MoveTo(ourCenter.getX(), ourCenter.getY()));


        // Below or Above us
        if ((angle >= 45.0 && angle < 135.0) || (angle >= 225.0 && angle < 315.0)) {
            // Go vertical
            path.getElements().add(new VLineTo(midPoint.getY()));
            // Go horizontal
            path.getElements().add(new HLineTo(theirCenter.getX()));
            // Finish vertical
            path.getElements().add(new VLineTo(theirCenter.getY()));
        }

        // To the left of us
        else {
            // Go horizontal
            path.getElements().add(new HLineTo(midPoint.getX()));
            // Go horizontal
            path.getElements().add(new VLineTo(theirCenter.getY()));
            // Finish vertical
            path.getElements().add(new HLineTo(theirCenter.getX()));
        }

        return path;
    }

    private void calculatePathsFor (ModuleView updatedView, String appName) {
        for (String key : _moduleViewsAppNames.keySet()) {
            ModuleView view1 = _moduleViewsAppNames.get(key);
            if (view1.equals(updatedView)) {
                continue;
            }

            generatePath(view1, updatedView, key, appName);
            generatePath(updatedView, view1, appName, key);
        }
    }

    private void generatePath (ModuleView view1, ModuleView view2, String name1, String name2) {
        Path path = calculatePathBetween(view1, view2);
        path.setStroke(Color.web("#7e7e7e"));
        path.setStrokeWidth(1);

        String pathKey = generatePathID(name1, name2);

        // If this is already in here, we need to signal it's being replaced so the view can be updated
        if (_pathsWithNames.containsKey(pathKey)) {
            Path removingPath = _pathsWithNames.get(pathKey);
            _removedPaths.push(removingPath);
        }

        _paths.push(path);
        _pathsWithNames.put(pathKey, path);

        if (_trackingConfigManager != null) {
            _trackingConfigManager.storePath(name1, name2, path);
        }
    }

    /**
     * Get the angle between the two centers of the stack panes to determine its position relative to the first
     *
     * @return Radians of rotation
     */
    private double getAngleBetweenCenters (ModuleView view1, ModuleView view2) {
        double centerX1 = view1.getLayoutX() + view1.getWidth() / 2;
        double centerY1 = view1.getLayoutY() + view1.getHeight() / 2;
        double centerX2 = view2.getLayoutX() + view2.getWidth() / 2;
        double centerY2 = view2.getLayoutY() + view2.getHeight() / 2;

        Point2D point1 = new Point2D(centerX1, centerY1);
        Point2D point2 = new Point2D(centerX2, centerY2);
        Point2D point3 = point2.subtract(point1);
        Point2D point4 = new Point2D(1, 0);

        return point4.angle(point3);
    }

    /**
     * Get the center of the view
     *
     * @param view View to checkout
     * @return Point of center
     */
    private Point2D getCenter (ModuleView view) {
        return new Point2D(view.getLayoutX() + view.getWidth() * 0.5,
                view.getLayoutY() + view.getHeight() * 0.5);
    }

    private String generatePathID (String name1, String name2) {
        return name1 + "to" + name2;
    }

    private TrackingConfigManager _trackingConfigManager;
    private Map<String, ModuleView> _moduleViewsAppNames;
    private EventSource<Path> _paths;
    private EventSource<Path> _removedPaths;
    private Map<String, Path> _pathsWithNames;

    private static final Logger _logger = LoggerFactory.getLogger(TrackingPathCalculator.class);

}
