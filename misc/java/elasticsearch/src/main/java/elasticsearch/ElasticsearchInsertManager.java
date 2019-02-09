package elasticsearch;

import org.apache.http.HttpHost;
import org.elasticsearch.action.ActionListener;
import org.elasticsearch.action.bulk.BulkRequest;
import org.elasticsearch.action.bulk.BulkResponse;
import org.elasticsearch.action.index.IndexRequest;
import org.elasticsearch.client.RestClient;
import org.elasticsearch.client.RestClientBuilder;
import org.elasticsearch.client.RestHighLevelClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Closeable;
import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicLong;

/**
 * The Elasticsearch Insert class is used to insert data into elastic.
 * The class requires that elastic is already running on the machine so that
 * the ESClientBuilder can buildHighLevel a client.
 */
public class ElasticsearchInsertManager implements Closeable
{
    /**
     * Build the client using different variables
     * @param host Host ip to connect to
     * @param port Port to connect to on host
     * @param scheme The scheme to use
     */
    public ElasticsearchInsertManager (String host, int port, String scheme)
    {
        _closeRequested = false;

        RestClientBuilder lowLevelBuilder = ESClientBuilder.buildLowLevel(host, port, scheme);
        lowLevelBuilder.setFailureListener(new RestClient.FailureListener() {

            @Override
            public void onFailure(HttpHost host)
            {
                if (System.currentTimeMillis() - _mostRecentPrintTime > FAILURE_PRINT_STRING_TIME) {
                    _mostRecentPrintTime = System.currentTimeMillis();
                    _logger.error("Error connecting to elasticsearch instance. " +
                            "Check that an instance is running on " + host);
                }
            }
        });

        _indexRequestResistantTimer = new ResistantTimer<>();
        _indexRequestResistantTimer.setMaxItemCount(_maxItemCount.get());

        _client = ESClientBuilder.buildHighLevel(lowLevelBuilder);

    }

    /**
     * Close the connection to the database. This should be explicitly called by any user of this class
     */
    public void close()
    {
        _closeRequested = true;
        requestClose();
    }

    /**
     * Close the connection whenever all the data has been stored
     */
    private void requestClose()
    {
        if (_latestRequest == null){
            closeClient();
            return;
        }

        while (!_latestRequest.isTransactionComplete())
        {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        closeClient();
    }

    private void closeClient()
    {
        try {
            _client.close();
        }
        catch(IOException e){
            e.printStackTrace();
        }
    }

    public void setIndex(String index)
    {
        _index = index;
    }

    /**
     * This method should only be used when also setting the index. Only one type per index will be allowed in
     * Elasticsearch 7.x (node must be configured in 6.x to have multiple types)
     */
    public void setIndexType (String type)
    {
        _type = type;
    }

    /**
     * Insert the Map into the indexRequest after updating the request ID
     * @param dataMap
     */
    public void storeData(Map<String, Object> dataMap)
    {
        if (_closeRequested) {
            return;
        }

        String id = UUID.randomUUID().toString();
        IndexRequest newRequest = createIndexRequest(_index, _type, id);

        newRequest.source(dataMap);

        IndexRequestFinish indexRequestFinish = new IndexRequestFinish();
        indexRequestFinish.offer(newRequest);
        _latestRequest = indexRequestFinish;

        storeIndexRequest(indexRequestFinish);
    }

    public void run()
    {
        long elapsedTime;
        long startTime = System.currentTimeMillis();
        while (!_closeRequested) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            elapsedTime = System.currentTimeMillis() - startTime;
            long waitTime = _indexRequestResistantTimer.calculateWaitTime();
            if (!(elapsedTime > waitTime)) {
                continue;
            }
            startTime = System.currentTimeMillis();

            BulkRequest newRequest = new BulkRequest();
            List<IndexRequestFinish> indexRequests = _indexRequestResistantTimer.getAndClearItems();

            _indexRequestResistantTimer.clearList();
            if (indexRequests.size() == 0){
                continue;
            }
            _logger.debug("Time elapsed between last sending: {} for count of items: {}", elapsedTime, indexRequests.size());

            for (IndexRequestFinish indexRequestFinish : indexRequests) {
                newRequest.add(indexRequestFinish.getIndexRequest());
            }

            _client.bulkAsync(newRequest, new ActionListener<BulkResponse>(){

                @Override
                public void onResponse (BulkResponse bulkItemResponses) {

                }

                @Override
                public void onFailure (Exception e) {
                    _maxItemCount.set(_maxItemCount.get() * 2);
                    _indexRequestResistantTimer.setMaxItemCount(_maxItemCount.get());
                }
            });
        }
    }

    /**
     * Take the index request, and put it into the time resistance list
     */
    private void storeIndexRequest(IndexRequestFinish indexRequest)
    {
        _indexRequestResistantTimer.addItem(indexRequest);
    }

    /**
     * Create a new index request
     * @param index The index for the request
     * @param type The type for the request
     * @param id The id of the request
     * @return
     */
    private IndexRequest createIndexRequest(String index, String type, String id)
    {
        return new IndexRequest(index, type, id);
    }

    private String _index;
    private String _type;
    private RestHighLevelClient _client;
    private boolean _closeRequested;
    private IndexRequestFinish _latestRequest;
    private static long _mostRecentPrintTime = 0;
    private ResistantTimer<IndexRequestFinish> _indexRequestResistantTimer;

    private AtomicLong _maxItemCount = new AtomicLong(100);

    private static final long FAILURE_PRINT_STRING_TIME = 5000;

    private static final Logger _logger = LoggerFactory.getLogger(Main.class);

}
