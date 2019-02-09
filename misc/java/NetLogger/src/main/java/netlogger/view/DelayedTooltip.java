package netlogger.view;

import javafx.animation.KeyFrame;
import javafx.animation.Timeline;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.scene.control.Tooltip;
import javafx.util.Duration;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DelayedTooltip extends Tooltip
{

    public void setDuration (int d) {
        _duration = d;
    }

    public boolean isHoveringPrimary () {
        return _isHoveringPrimary;
    }

    public boolean isHoveringSecondary () {
        return _isHoveringSecondary;
    }

    public void setHoveringTargetPrimary (boolean val) {
        _isHoveringPrimary = val;
    }

    //Usually you will use the tooltip here so enter tooltip.getGraphic() for the node.
    public void setHoveringTargetSecondary (boolean val) {
        _isHoveringSecondary = val;
    }

    @Override
    public void hide () {
        if (_isHoveringPrimary || _isHoveringSecondary) {
            Timeline timeline = new Timeline();
            KeyFrame key = new KeyFrame(Duration.millis(_duration));
            timeline.getKeyFrames().add(key);
            timeline.setOnFinished(t -> DelayedTooltip.super.hide());
            timeline.play();
        }
        else {
            DelayedTooltip.super.hide();
        }
    }

    private int _duration = 0;
    private boolean _isHoveringPrimary;
    private boolean _isHoveringSecondary;
    private static final Logger _logger = LoggerFactory.getLogger(DelayedTooltip.class);
}
