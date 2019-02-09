package netlogger.util;

import netlogger.model.StyledString;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class CustomMeasureToString
{
    public static String getStringFromMeasure (StyledString message) {
        return message.getTimestamp().getText() + "\n" + message.getString().getText();
    }

    private static final Logger _logger = LoggerFactory.getLogger(CustomMeasureToString.class);
}
