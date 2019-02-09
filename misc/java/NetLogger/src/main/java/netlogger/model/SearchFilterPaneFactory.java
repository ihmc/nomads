package netlogger.model;

import netlogger.view.SearchFilterPane;

/**
 * This class creates panes that contain 1 text item
 */
public class SearchFilterPaneFactory
{

    public SearchFilterPaneFactory () {

    }

    /**
     * Create the pane
     *
     * @param string String of the text to put on the pane
     * @return The pane
     */
    public SearchFilterPane createFilterPane (String string) {
        return new SearchFilterPane(string);
    }
}
