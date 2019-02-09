package elasticsearch;

import org.apache.http.HttpEntity;
import org.apache.http.HttpHost;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.entity.ContentType;
import org.apache.http.nio.entity.NStringEntity;
import org.elasticsearch.action.admin.indices.create.CreateIndexRequest;
import org.elasticsearch.client.*;
import org.elasticsearch.common.xcontent.XContentType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Closeable;
import java.io.IOException;
import java.util.Collections;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class ElasticsearchIndexStoreManager implements Closeable
{
    public ElasticsearchIndexStoreManager () {

    }

    @Override
    public void close () throws IOException {
        _client.close();
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

    /**
     * Get all elements of the old index, and store it into another user-specified index.
     *
     * @param newIndexName Name of the new index to store to
     */
    public void storeDataToIndex (String oldIndexName, String newIndexName) {
        _finished = false;
        if (!indexExists(newIndexName)) {
            createNewIndex(newIndexName);
        }

        String json =
                "{\n" +
                        "  \"source\": {\n" +
                        "    \"index\": \"" + oldIndexName + "\"\n" +
                        "  },\n" +
                        "  \"dest\": {\n" +
                        "    \"index\": \"" + newIndexName + "\"\n" +
                        "  }\n" +
                        "}";
        HttpEntity entity = new NStringEntity(json, ContentType.APPLICATION_JSON);

        _client.getLowLevelClient().performRequestAsync("POST", "_reindex", Collections.emptyMap(), entity, new ResponseListener()
        {
            @Override
            public void onSuccess (Response response) {
                _logger.info("Success");
                clearDocumentsFromIndex(oldIndexName);
            }

            @Override
            public void onFailure (Exception exception) {
                exception.printStackTrace();
            }
        });

    }

    public void clearDocumentsFromIndex (String index) {
        String json = "{\n" +
                "    \"query\" : { \n" +
                "        \"match_all\" : {}\n" +
                "    }\n" +
                "}";
        HttpEntity entity = new NStringEntity(json, ContentType.APPLICATION_JSON);
        _client.getLowLevelClient().performRequestAsync("POST", index + "/_delete_by_query", Collections.emptyMap(), entity, new ResponseListener()
        {
            @Override
            public void onSuccess (Response response) {
                _logger.info("Success clearing!");
                _finished = true;
                notifyCV();
            }

            @Override
            public void onFailure (Exception exception) {
                exception.printStackTrace();
            }
        });

    }

    /**
     * Check if the index exists
     *
     * @param index name of index
     * @return Whether index exists
     */
    public boolean indexExists (String index) {
        try {
            Response response = _client.getLowLevelClient().performRequest("HEAD", index);
            int code = response.getStatusLine().getStatusCode();

            // code == 200 means index exists, code == 404 means it doesn't
            return code == 200;
        } catch (IOException e) {
            e.printStackTrace();
        }

        return false;
    }

    /**
     * Create a new index
     *
     * @param index name of the index
     */
    public void createNewIndex (String index) {
        CreateIndexRequest createIndexRequest = new CreateIndexRequest(index);
        createIndexRequest.mapping("measure",
                "  {\n" +
                        "    \"measure\": {\n" +
                        "      \"properties\": {\n" +
                        "        \"timestamp\": {\n" +
                        "          \"type\": \"date\"\n" +
                        "        },\n" +
                        "        \"stored_timestamp\": {\n" +
                        "          \"type\": \"date\"\n" +
                        "        }\n" +
                        "      }\n" +
                        "    }\n" +
                        "  }",
                XContentType.JSON);

        try {
            _client.indices().create(createIndexRequest);
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    public void awaitFinish () {
        // Check if we're finished already--highly unlikely but still should check
        if (_finished) {
            return;
        }

        _lock.lock();
        try {
            _finishedCV.await();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        _lock.unlock();
    }

    private void notifyCV () {
        _lock.lock();
        _finishedCV.signal();
        _lock.unlock();
    }

    // High level client. May not be especially needed here since we generally use the low level client anyway to perform
    // these harder tasks
    private RestHighLevelClient _client;

    private final Lock _lock = new ReentrantLock();
    private final Condition _finishedCV = _lock.newCondition();
    private boolean _finished = false;

    private static final Logger _logger = LoggerFactory.getLogger(ElasticsearchIndexStoreManager.class);
}
