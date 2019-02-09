package netlogger.model;

import netlogger.model.messages.TextValue;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class StyledString
{
    public StyledString (StringMeasure measure) {
        _timestamp = new TextValue();
        _string = new TextValue();

        _textValues = new ArrayList<>();

        _keywords = measure.getKeyList();

        _timestamp.setText(measure.getTimestamp());
        _string.setText(measure.getString());

        _textValues.addAll(Arrays.asList(_timestamp, _string));
    }


    public List<TextValue> getTextValues () {
        return _textValues;
    }

    public TextValue getTimestamp () {
        return _timestamp;
    }

    public TextValue getString () {
        return _string;
    }

    public List<String> getKeywords () {
        return _keywords;
    }

    private TextValue _timestamp;
    private TextValue _string;

    private List<String> _keywords;
    private List<TextValue> _textValues;
}
