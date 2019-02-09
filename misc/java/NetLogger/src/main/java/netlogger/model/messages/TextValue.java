package netlogger.model.messages;

import javafx.scene.control.IndexRange;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

public class TextValue
{
    public TextValue () {
        _boldRanges = new ArrayList<>();
        _highlightRanges = new ArrayList<>();
        _boldHighlightRanges = new ArrayList<>();
    }

    public void setHighlightColor (String webColor) {
        _highlightColor = webColor;
    }

    public String getHighlightColor () {
        return _highlightColor;
    }

    public void addNewBoldRange (int from, int to) {
        _boldRanges.add(new IndexRange(from, to));
    }

    public void addNewHighlightRange (int from, int to) {
        _highlightRanges.add(new IndexRange(from, to));
    }

    public void addNewBoldHighlightRange (int from, int to) {
        _boldHighlightRanges.add(new IndexRange(from, to));
    }

    public void clearBoldRanges () {
        _boldRanges.clear();
    }

    public void clearHighlightRanges () {
        _highlightRanges.clear();
    }

    public List<IndexRange> getBoldRanges () {
        return _boldRanges;
    }

    public List<IndexRange> getHighlightRanges () {
        return _highlightRanges;
    }

    public List<IndexRange> getBoldHighlightRanges () {
        return _boldHighlightRanges;
    }

    public String getText () {
        return _text;
    }

    public void setHighlightRanges (List<IndexRange> newRanges) {
        _highlightRanges = newRanges;
    }

    public void setBoldRanges (List<IndexRange> boldRanges) {
        _boldRanges = boldRanges;
    }

    public void setBoldHighlightRanges (List<IndexRange> boldHighlightRanges) {
        _boldHighlightRanges = boldHighlightRanges;
    }

    public void setText (String text) {
        _text = text;
    }

    private String _text;
    private List<IndexRange> _boldRanges;
    private List<IndexRange> _highlightRanges;


    private List<IndexRange> _boldHighlightRanges;

    private static String _highlightColor;

    private static final Logger _logger = LoggerFactory.getLogger(TextValue.class);
}
