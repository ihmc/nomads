package elasticsearch;

import org.apache.http.HttpHost;
import org.apache.http.client.config.RequestConfig;
import org.elasticsearch.action.search.*;
import org.elasticsearch.client.RestClient;
import org.elasticsearch.client.RestClientBuilder;
import org.elasticsearch.client.RestHighLevelClient;
import org.elasticsearch.common.unit.TimeValue;
import org.elasticsearch.search.Scroll;
import org.elasticsearch.search.SearchHit;
import org.elasticsearch.search.builder.SearchSourceBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;

import static org.elasticsearch.index.query.QueryBuilders.matchAllQuery;

public class ElasticsearchSearchManager
{
    public ElasticsearchSearchManager () {

    }

    public void init (String host, int port, String scheme) {
        RestClientBuilder lowLevelBuilder = ESClientBuilder.buildLowLevel(host, port, scheme);
        lowLevelBuilder.setRequestConfigCallback(new RestClientBuilder.RequestConfigCallback()
        {
            @Override
            public RequestConfig.Builder customizeRequestConfig (RequestConfig.Builder requestConfigBuilder) {
                return requestConfigBuilder.setConnectTimeout(5000).setSocketTimeout(60000);
            }
        });
        lowLevelBuilder.setFailureListener(new RestClient.FailureListener()
        {
            @Override
            public void onFailure (HttpHost host) {
                _logger.error("Failure");
            }
        });

        _client = ESClientBuilder.buildHighLevel(lowLevelBuilder);
    }

    public void setUpSearch (String index) {
        SearchRequest searchRequest = new SearchRequest(index);
        searchRequest.scroll(_scroll);
        SearchSourceBuilder searchSourceBuilder = new SearchSourceBuilder();
        searchSourceBuilder.query(matchAllQuery());
        searchSourceBuilder.size(10000);
        searchRequest.source(searchSourceBuilder);

        _searchResponse = null;
        try {
            _searchResponse = _client.search(searchRequest);
        } catch (IOException e) {
            e.printStackTrace();
        }

        _scrollID = _searchResponse.getScrollId();
    }

    public SearchHit[] getCurrentSearchHits () {
        return _searchResponse.getHits().getHits();
    }

    public void searchNext () {
        SearchScrollRequest scrollRequest = new SearchScrollRequest(_scrollID);
        scrollRequest.scroll(_scroll);
        try {
            _searchResponse = _client.searchScroll(scrollRequest);
        } catch (RuntimeException | IOException e) {
            e.printStackTrace();
        }

        _scrollID = _searchResponse.getScrollId();
    }

    public boolean clearScroll () {
        ClearScrollRequest clearScrollRequest = new ClearScrollRequest();
        clearScrollRequest.addScrollId(_scrollID);
        try {
            ClearScrollResponse clearScrollResponse = _client.clearScroll(clearScrollRequest);
            return clearScrollResponse.isSucceeded();
        } catch (IOException e) {
            return false;
        }
    }

    private RestHighLevelClient _client;
    private final Scroll _scroll = new Scroll(TimeValue.timeValueMinutes(2L));
    private SearchResponse _searchResponse;
    private String _scrollID;


    private static final Logger _logger = LoggerFactory.getLogger(ElasticsearchSearchManager.class);
}
