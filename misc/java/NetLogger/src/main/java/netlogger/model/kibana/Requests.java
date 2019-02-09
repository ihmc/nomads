package netlogger.model.kibana;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Requests
{
    // Returns the string appended to the URL of the kibana instance to GET visualizations
    public static String getVisualizationsRequestString () {
        return "/api/saved_objects/?type=visualization&per_page=1000&page=1&search_fields=title%5E3&search_fields=description";
    }

    public static String getElasticsearchSearchRequestString () {
        return "/elasticsearch/_msearch";
    }

    private static final Logger _logger = LoggerFactory.getLogger(Requests.class);
}
