package netlogger.util.tracking;

import javafx.scene.shape.*;
import netlogger.util.ConfigFileManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

/**
 * Manager for storing paths between points in a config file. This way little computation will be needed after the first time
 */
public class TrackingConfigManager
{
    public TrackingConfigManager (String configPath) {
        _configFileManager = new ConfigFileManager();
        _configFileManager.init(configPath);
    }

    /**
     * Store a path between points
     *
     * @param point1 First point name
     * @param point2 Second point name
     * @param path   Path between the points
     */
    public void storePath (String point1, String point2, Path path) {

        String propertyKey = createPropertyKey(point1, point2);
        String valueKey = createValueKey(path);

        _configFileManager.updateConfig(propertyKey, valueKey);
    }

    /**
     * Create a property name using the points after alphabetizing them
     * Alphabetizing will mean only 1 entry will be created no matter what order the points are passed in
     *
     * @param point1 First point name
     * @param point2 Second point name
     * @return String value of the property name
     */
    private String createPropertyKey (String point1, String point2) {

        return point1 + "." + "to." + point2;
    }

    /**
     * Create a CSV of the path
     *
     * @param path Path to use
     * @return String CSV
     */
    private String createValueKey (Path path) {
        StringBuilder pathString = new StringBuilder();

        double prevX = 0;
        double prevY = 0;
        for (PathElement pathElement : path.getElements()) {
            if (pathElement instanceof HLineTo) {
                prevX = ((HLineTo) pathElement).getX();
            }
            else if (pathElement instanceof VLineTo) {
                prevY = ((VLineTo) pathElement).getY();
            }
            else if (pathElement instanceof MoveTo) {
                prevX = ((MoveTo) pathElement).getX();
                prevY = ((MoveTo) pathElement).getY();
            }

            pathString.append(prevX).append(",").append(prevY);

            // If this isn't the last path element, put a comma. We don't want a comma at the end of our string
            if (path.getElements().indexOf(pathElement) != path.getElements().size() - 1) {
                pathString.append(",");
            }

        }

        return pathString.toString();
    }

    /**
     * Get the path from the config file
     *
     * @param point1 First point
     * @param point2 Second point
     * @return Found path
     */
    private Path getPathBetweenPoints (String point1, String point2) {

        String propertyKey = createPropertyKey(point1, point2);

        String coordinateString = _configFileManager.getStringValue(propertyKey);


        StringTokenizer tokenizer = new StringTokenizer(coordinateString, ",");
        List<String> coordinates = new ArrayList<>();
        coordinates.add(tokenizer.nextToken());
        while (tokenizer.hasMoreElements()) {
            coordinates.add(tokenizer.nextToken());
        }

        Path path = new Path();
        double prevX = 0;
        double prevY = 0;

        for (int i = 0; i < coordinates.size(); i += 2) {
            double x;
            double y;

            x = Double.valueOf(coordinates.get(i));
            y = Double.valueOf(coordinates.get(i + 1));

            if (i == 0) {
                path.getElements().add(new MoveTo(x, y));
            }
            else {
                if (x == prevX) {
                    path.getElements().add(new VLineTo(y));
                }
                else if (y == prevY) {
                    path.getElements().add(new HLineTo(x));
                }
            }

            prevX = x;
            prevY = y;
        }

        return path;
    }

    private ConfigFileManager _configFileManager;
    private static final Logger _logger = LoggerFactory.getLogger(TrackingConfigManager.class);
}
