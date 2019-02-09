package netlogger.util;

import javafx.scene.control.IndexRange;
import netlogger.model.StyledString;
import netlogger.model.messages.TextValue;
import netlogger.model.settings.SettingField;
import netlogger.model.settings.Updatable;
import netlogger.util.settings.SettingsManager;

import java.util.*;

/**
 * This class is used to set the startTimeline and end position of characters that need
 * to have their color, or font style changed
 */
public class StylingManager implements Updatable
{
    public StylingManager () {
        _changedColor = false;
        _highlightStrings = new ArrayList<>();

        SettingsManager.getInstance().getSettingsBridge().addUpdatableClass(this);

        _highlightColor = "#ffeb3b";
    }


    public void addString (String newString) {
        _highlightStrings.add(newString);
    }

    public void clearValues () {
        _highlightStrings.clear();
    }

    private List<IndexRange> getOverlappingRanges (List<IndexRange> range1List, List<IndexRange> range2List) {

        List<IndexRange> overLappingRange = new ArrayList<>();

        int range1index = 0;
        int range2Index = 0;

        while (range1index < range1List.size() && range2Index < range2List.size()) {
            IndexRange range1 = range1List.get(range1index);
            IndexRange range2 = range2List.get(range2Index);

            IndexRange midLine = getMidPoints(range1, range2);

            if (range1.getEnd() < range2.getEnd()) {
                range1index++;
            }
            else {
                range2Index++;
            }

            if (range1.getEnd() >= range2.getStart() && range2.getEnd() >= range1.getStart()) {
                overLappingRange.add(midLine);
            }
        }

        List<IndexRange> filteredRanges = new ArrayList<>();

        for (int i = 0; i < overLappingRange.size(); i++) {
            IndexRange curr = overLappingRange.get(i);
            if (i != overLappingRange.size() - 1) {
                IndexRange next = overLappingRange.get(i + 1);
                if (curr.getEnd() == next.getStart()) {
                    IndexRange newRange = new IndexRange(curr.getStart(), next.getEnd());
                    filteredRanges.add(newRange);
                }
                else {
                    filteredRanges.add(curr);
                }
            }
            else {
                filteredRanges.add(curr);
            }

        }

        return filteredRanges;
    }

    private List<IndexRange> removeOverlaps (List<IndexRange> permanentRanges, List<IndexRange> mutableRanges) {
        List<IndexRange> fixedList = new ArrayList<>();

        int range1index = 0;
        int range2Index = 0;

        IndexRange permanentRange = permanentRanges.get(0);
        IndexRange mutableRange = mutableRanges.get(0);
        while (range1index < permanentRanges.size() && range2Index < mutableRanges.size()) {

            permanentRange = permanentRanges.get(range1index);

            mutableRange = mutableRanges.get(range2Index);


            IndexRange midLine = getMidPoints(permanentRange, mutableRange);

            if (mutableRange.getStart() >= permanentRange.getStart() &&
                    mutableRange.getEnd() <= permanentRange.getEnd()) {
                // Perm:           2------------------------------4
                // Mutable:     1-----------------------------3
                // Midline:        2--------------------------3
                // End:          ----
                fixedList.add(new IndexRange(mutableRange.getStart(), midLine.getStart()));
            }
            else if (mutableRange.getStart() <= permanentRange.getEnd() && mutableRange.getEnd() >= permanentRange.getEnd()) {
                // Perm:     1----------------3
                // Mutable:               2-----------------------------4
                // Midline:               2---3
                // End:                       --------------------------
                fixedList.add(new IndexRange(midLine.getEnd(), mutableRange.getEnd()));
            }
            else if (mutableRange.getStart() <= permanentRange.getStart() && mutableRange.getEnd() >= permanentRange.getEnd()) {
                // Perm:           2---------------------3
                // Mutable:     1-----------------------------4
                // Midline:        2---------------------3
                // End:          ----                   ------
                fixedList.add(new IndexRange(mutableRange.getStart(), midLine.getStart()));
                fixedList.add(new IndexRange(midLine.getEnd(), mutableRange.getEnd()));
            }
        }

        // Have to filter list in case we get ranges like so:
        // Perm:    ------------------       --------------     --
        // Mutable:  ------------------------------------------------
        // Midline1: ----------------
        // End1:                    ---------------------------------
        // Midline2:                         --------------
        // End2:     -------------------------            -----------
        // Midline3:                                            --
        // End3:     ------------------------------------------- ----
        // Midlines:                ----------            ------ ----
        List<IndexRange> filteredRanges = new ArrayList<>();

        for (IndexRange range1 : fixedList) {
            for (IndexRange range2 : fixedList) {
                IndexRange midLine = getMidPoints(range1, range2);

            }
        }

        for (int i = 0; i < fixedList.size() - 1; i++) {
            IndexRange curr = fixedList.get(i);
            IndexRange next = fixedList.get(i + 1);

            IndexRange midLine = getMidPoints(curr, next);

            if (curr.getEnd() < next.getEnd()) {
                range1index++;
            }
            else {
                range2Index++;
            }

            if (curr.getEnd() >= next.getStart() && next.getEnd() >= curr.getStart()) {
                filteredRanges.add(midLine);
            }
        }

        return filteredRanges;
    }

    public void removeString (String stringToRemove) {
        _highlightStrings.remove(stringToRemove);
    }

    public void setValues (StyledString measure) {
        for (TextValue value : measure.getTextValues()) {
            if (value.equals(measure.getTimestamp())) {
                value.addNewBoldRange(0, value.getText().length());
            }

            highlight(value);
            bold(value, measure.getKeywords());
            highlightAndBold(value);
        }
    }


    @Override
    public void update (HashMap<String, Object> settings) {
        changeHighlightColor((String) settings.get(SEARCH_HIGHLIGHT_COLOR_TEXT));
    }

    /**
     * Set the range of characters to be bold
     *
     * @param textValue
     */
    private void bold (TextValue textValue, List<String> keywords) {
        String text = textValue.getText();

        StringTokenizer stringTokenizer = new StringTokenizer(text, " ");
        while (stringTokenizer.hasMoreTokens()) {
            String val = stringTokenizer.nextToken();

            if (val.contains(":") && Character.isUpperCase(val.charAt(0))) {
                if (keywords.contains(val.split(":")[0])) {
                    int start = text.indexOf(val);
                    int end = start + val.length();

                    textValue.addNewBoldRange(start, end);
                }
            }
        }

        textValue.setBoldRanges(smartMerge(textValue.getBoldRanges()));
    }

    private void changeHighlightColor (String newColor) {
        _changedColor = false;
        _highlightColor = newColor;
    }

    private static IndexRange getMidPoints (IndexRange first, IndexRange second) {

        int[] array = new int[]{first.getStart(), first.getEnd(), second.getStart(), second.getEnd()};
        Arrays.sort(array);

        return new IndexRange(array[1], array[2]);
    }

    /**
     * Set the range of characters to be highlighted based on specific keywords
     */
    private void highlight (TextValue textValue) {
        String measureString = textValue.getText();
        for (String keyword : _highlightStrings) {
            int start = -1;
            do {
                start = measureString.indexOf(keyword, start + 1);
                int end = start + keyword.length();
                if (start != -1) {
                    textValue.addNewHighlightRange(start, end);
                }
            }
            while (start != -1);
        }

        textValue.setHighlightRanges(smartMerge(textValue.getHighlightRanges()));

        if (!_changedColor) {
            textValue.setHighlightColor(_highlightColor);
            _changedColor = true;
        }
    }

    /**
     * Set the range of characters that should be both highlighted AND bold
     *
     * @param textValue
     */
    private void highlightAndBold (TextValue textValue) {
        List<IndexRange> ranges = getOverlappingRanges(textValue.getBoldRanges(), textValue.getHighlightRanges());
        textValue.setBoldHighlightRanges(ranges);
    }

    /**
     * Quick sorts the index ranges based on their startTimeline
     *
     * @param indexRanges Ranges to sort
     * @param low         Start
     * @param high        End
     */
    private void quickSort (List<IndexRange> indexRanges, int low, int high) {
        if (indexRanges == null || indexRanges.size() == 0)
            return;

        if (low >= high)
            return;

        // pick the pivot
        int middle = low + (high - low) / 2;
        int pivot = indexRanges.get(middle).getStart();

        // make left < pivot and right > pivot
        int i = low, j = high;
        while (i <= j) {
            while (indexRanges.get(i).getStart() < pivot) {
                i++;
            }

            while (indexRanges.get(j).getStart() > pivot) {
                j--;
            }

            if (i <= j) {
                Collections.swap(indexRanges, i, j);
                i++;
                j--;
            }
        }

        // recursively sort two sub parts
        if (low < j)
            quickSort(indexRanges, low, j);

        if (high > i)
            quickSort(indexRanges, i, high);
    }

    /**
     * If there's any overlapping/continuous ranges, (0-7, 2-4 are overlapping, or similarly 0-4, 4-7 are continuous),
     * merge them.
     *
     * @param indexRanges
     * @return
     */
    private List<IndexRange> smartMerge (List<IndexRange> indexRanges) {
        Stack<IndexRange> stackRanges = new Stack<>();

        if (indexRanges.size() == 0) {
            return indexRanges;
        }

        // Sort the ranges to be in numerical order
        quickSort(indexRanges, 0, indexRanges.size() - 1);

        stackRanges.push(indexRanges.get(0));

        for (int i = 1; i < indexRanges.size(); i++) {
            IndexRange top = stackRanges.peek();
            // If these aren't overlapping, then put the index range onto the stack
            if (top.getEnd() < indexRanges.get(i).getStart()) {
                stackRanges.push(indexRanges.get(i));
            }

            // Otherwise, if the end of the top one is less than the current one, we can increase the range
            // to be the startTimeline of the top and the end of the current one
            else if (top.getEnd() < indexRanges.get(i).getEnd()) {
                IndexRange newTop = new IndexRange(top.getStart(), indexRanges.get(i).getEnd());
                stackRanges.pop();
                stackRanges.push(newTop);
            }
            // No need for else since top value takes care of the end of the range (may be greater than or equal to)
        }

        return new ArrayList<>(stackRanges);
    }

    private static final String SEARCH_HIGHLIGHT_COLOR_TEXT = "Search highlighting color";
    private List<String> _highlightStrings;
    private boolean _changedColor;
    @SettingField(settingText = SEARCH_HIGHLIGHT_COLOR_TEXT, configText = ConfigConstants.HIGHLIGHT_COLOR,
            defaultValue = "#ffeb3b")
    private String _highlightColor;
}
