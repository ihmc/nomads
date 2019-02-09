package elasticsearch;

import org.apache.http.HttpHost;
import org.elasticsearch.client.RestClient;
import org.elasticsearch.client.RestClientBuilder;
import org.elasticsearch.client.RestHighLevelClient;

/**
 * Elasticsearch client builder. Builds client based off of config constants
 */
public class ESClientBuilder
{

    protected static RestClientBuilder buildLowLevel (String host, int port, String scheme) {
        HttpHost newHost = new HttpHost(host, port, scheme);

        return RestClient.builder(newHost);
    }

    protected static RestHighLevelClient buildHighLevel (RestClientBuilder lowLevel) {
        return new RestHighLevelClient(lowLevel);
    }
}
