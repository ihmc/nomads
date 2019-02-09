package netlogger.model.messages;

import netlogger.model.StringMeasure;

import java.util.ArrayList;
import java.util.List;

public class MeasureFilter
{
    public MeasureFilter () {
        _currentSearchStrings = new ArrayList<>();
        _topicFilterStrings = new ArrayList<>();
    }


    /**
     * Add a new search string to the list and refilter all objects
     *
     * @param searchString
     */
    public void addNewSearchString (String searchString) {
        _currentSearchStrings.add(searchString);
    }

    /**
     * Remove a search string and refilter objects
     *
     * @param stringToRemove
     */
    public void removeSearchString (String stringToRemove) {
        _currentSearchStrings.remove(stringToRemove);
    }

    public void addTopicFilterString (String topic) {
        if (!_topicFilterStrings.contains(topic)) {
            _topicFilterStrings.add(topic);
        }
    }

    public void removeTopicFilterString (String topicToRemove) {
        _topicFilterStrings.remove(topicToRemove);
    }


    /**
     * Search for objects
     *
     * @param measure
     * @return
     */
    public boolean search (StringMeasure measure) {
        if (_currentSearchStrings.size() == 0) {
            return true;
        }

        for (String searchVal : _currentSearchStrings) {
            if (!measure.getString().contains(searchVal) &&
                    !measure.getTimestamp().contains(searchVal)) {
                return false;
            }
        }
        return true;
    }

    public boolean checkTopic (StringMeasure measure) {
        if (_topicFilterStrings.size() == 0) {
            return true;
        }

        for (String topicName : _topicFilterStrings) {
            if (measure.getNatsTopic().equals(topicName)) {
                return true;
            }
        }
        return false;
    }

    private List<String> _currentSearchStrings;
    private List<String> _topicFilterStrings;
}
