package netlogger.model.tracking;

import com.sun.javafx.collections.ObservableMapWrapper;
import javafx.collections.MapChangeListener;
import javafx.scene.paint.Color;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.concurrent.ThreadLocalRandom;

public class CircleColorByType
{
    private CircleColorByType () {
        _colors = new ObservableMapWrapper<>(new HashMap<>());
    }

    public static CircleColorByType getInstance () {
        if (_instance == null) {
            _instance = new CircleColorByType();
        }
        return _instance;
    }

    public synchronized Color checkType (String type) {
        return _colors.computeIfAbsent(type, k -> {
            // Gives a darker pastel than 255
            final int baseRed = 200;
            final int baseGreen = 200;
            final int baseBlue = 200;

            // Create pastel colors by dividing the base color + random by 2
            final int red = (baseRed + ThreadLocalRandom.current().nextInt(0, 256)) / 2;
            final int green = (baseGreen + ThreadLocalRandom.current().nextInt(0, 256)) / 2;
            final int blue = (baseBlue + ThreadLocalRandom.current().nextInt(0, 256)) / 2;

            Color newColor = Color.rgb(red, green, blue);
            _colors.put(type, newColor);
            return newColor;
        });
    }

    public synchronized void updateColorForType (String type, Color color) {
        _colors.put(type, color);
    }

    public void addListener (MapChangeListener<String, Color> listener) {
        _colors.addListener(listener);
    }


    private static CircleColorByType _instance;
    private ObservableMapWrapper<String, Color> _colors;
    private static final Logger _logger = LoggerFactory.getLogger(CircleColorByType.class);
}
