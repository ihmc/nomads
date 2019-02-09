package elasticsearch;

import org.apache.http.HttpHost;
import org.elasticsearch.action.ActionListener;
import org.elasticsearch.action.admin.indices.create.CreateIndexRequest;
import org.elasticsearch.action.bulk.BulkRequest;
import org.elasticsearch.action.bulk.BulkResponse;
import org.elasticsearch.action.index.IndexRequest;
import org.elasticsearch.client.Response;
import org.elasticsearch.client.RestClient;
import org.elasticsearch.client.RestClientBuilder;
import org.elasticsearch.client.RestHighLevelClient;
import org.elasticsearch.common.xcontent.XContentType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Closeable;
import java.io.IOException;
import java.net.Socket;
import java.util.ArrayList;
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
    public ElasticsearchInsertManager () {
        _closeRequested = false;
        _sentOldBulkRequests = true;
        _itemsToSend = new ArrayList<>();

        _oldBulkRequestsStored = new BulkRequest();

        _indexRequestResistantTimer = new ResistantTimer<>();
        _indexRequestResistantTimer.setMaxItemCount(_maxItemCount.get());
    }

    /**
     * Periodically checks whether or not the Elasticsearch instance is still running
     *
     * @param host Hostname (or IP) of the elasticsearch
     * @param port Port on the host
     */
    public void checkConnectivity (String host, int port) {
        new Thread(() -> {
            long mostRecentPrintTime = 0;
            while (!_closeRequested) {
                // See if we can use the port and connect to it. If not, then there's no elasticsearch node here
                try {
                    Socket socket = new Socket(host, port);
                    socket.close();

                    _elasticsearchInstanceRunning = true;
                } catch (IOException e) {
                    if (System.currentTimeMillis() - mostRecentPrintTime > FAILURE_PRINT_STRING_TIME) {
                        _logger.error("No elasticsearch instance running on {}:{}", host, port);
                        mostRecentPrintTime = System.currentTimeMillis();
                    }
                    _elasticsearchInstanceRunning = false;
                }

                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

        }, "Elasticsearch connectivity check").start();
    }

    /**
     * Close the connection to the database. This should be explicitly called by any user of this class
     */
    @Override
    public void close () {
        _logger.info("Close requested for ElasticsearchInsertManager");
        _closeRequested = true;
        requestClose();
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
     * Builds the client
     *
     * @param host   Host ip to connect to
     * @param port   Port to connect to on host
     * @param scheme The scheme to use
     */
    public void init (String host, int port, String scheme) {
        RestClientBuilder lowLevelBuilder = ESClientBuilder.buildLowLevel(host, port, scheme);
        lowLevelBuilder.setFailureListener(new RestClient.FailureListener()
        {
            @Override
            public void onFailure (HttpHost host) {
                if (System.currentTimeMillis() - _mostRecentPrintTime > FAILURE_PRINT_STRING_TIME) {
                    _mostRecentPrintTime = System.currentTimeMillis();
                    _logger.error("Error connecting to elasticsearch instance. " +
                            "Check that an instance is running on " + host);
                }
            }
        });


        _client = ESClientBuilder.buildHighLevel(lowLevelBuilder);
    }

    /**
     * Can be called by a manager/application to see whether or not there is an elasticsearch instance running
     * on the specified port
     *
     * @return Returns whether or not the instance is running
     */
    public boolean isElasticsearchInstanceRunning () {
        return _elasticsearchInstanceRunning;
    }

    public void run () {
        while (!_closeRequested) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            if (!_indexRequestResistantTimer.itemsAreAccessible()) {
                continue;
            }

            sendCurrentItems();
        }
    }

    public void setIndex (String index) {
        _index = index;

        if (!indexExists(_index)) {
            createNewIndex(_index);
        }
    }

    /**
     * This method should only be used when also setting the index. Only one type per index will be allowed in
     * Elasticsearch 7.x (node must be configured in 6.x to have multiple measure.pojos per index)
     */
    public void setIndexType (String type) {
        _type = type;
    }

    /**
     * Insert the Map into the indexRequest after updating the request ID
     *
     * @param dataMap Data to store
     */
    public void storeData (Map<String, Object> dataMap) {
        if (_closeRequested) {
            return;
        }

        String id = UUID.randomUUID().toString();
        IndexRequest newRequest = createIndexRequest(_index, _type, id);

        newRequest.source(dataMap);
        storeIndexRequest(newRequest);
    }

    public void updateMaxCount (long count) {
        _indexRequestResistantTimer.setMaxItemCount(count);
    }

    public void updateMaxWaitTime (long waitTimeInMillis) {
        _indexRequestResistantTimer.setMaxWaitTime(waitTimeInMillis);
    }

    /**
     * Close the elasticsearch client after all the items have been sent
     */
    private void closeClient () {
        new Thread(() -> {
            while (!_canClose) {
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            if (_client == null) {
                return;
            }
            try {
                _client.close();
            } catch (IOException e) {
                e.printStackTrace();
            }

            _logger.info("Client closed");
        }, "Elasticsearch close waiting").start();
    }

    /**
     * Create a new index request
     *
     * @param index The index for the request
     * @param type  The type for the request
     * @param id    The id of the request
     * @return return newly created Index request
     */
    private IndexRequest createIndexRequest (String index, String type, String id) {
        return new IndexRequest(index, type, id);
    }

    /**
     * Close the connection whenever all the data has been stored
     */
    private void requestClose () {
        sendCurrentItems();
        closeClient();
    }

    private void runRepeatingRequestThread () {
        _hasBulkRequestThreadRunning = true;
        new Thread(() -> {
            while (!_sentOldBulkRequests) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException ignored) {
                }

                if (_elasticsearchInstanceRunning) {
                    sendOldBulkRequest();
                }
                if (_closeRequested) {
                    _canClose = true;
                    break;
                }
            }
            resetOldBulkRequest();
            _hasBulkRequestThreadRunning = false;
        }, "Elasticsearch request send/store thread").start();
    }

    /**
     * Send the bulk request to the elasticsearch instance. This method is called if the request fails to send.
     *
     * @param request Bulk request to send
     */
    private void sendBulkRequest (BulkRequest request) {
        _client.bulkAsync(request, new ActionListener<BulkResponse>()
        {
            @Override
            public void onResponse (BulkResponse bulkItemResponses) {
                if (bulkItemResponses.hasFailures()) {
                    _logger.error("Errors putting some data in");
                }
                else {
                    if (_closeRequested && _sentOldBulkRequests) {
                        _canClose = true;
                    }
                }
            }

            @Override
            public void onFailure (Exception e) {
                // If there's an elasticsearch instance running, set the maximum count higher on our resistant timer
                // higher so we don't flood elasticsearch with requests. This may prevent onFailure() from being called
                if (_elasticsearchInstanceRunning) {
                    _maxItemCount.set(_maxItemCount.get() * 2);
                    _indexRequestResistantTimer.setMaxItemCount(_maxItemCount.get());
                }
                if (!_closeRequested) {
                    // Store this bulk request for later
                    storeBulkRequestForLater(request);
                }
            }
        });
    }

    /**
     * Sends all the items in the resistant timer object to a bulk request to be sent to Elasticsearch
     */
    private void sendCurrentItems () {
        BulkRequest newRequest = new BulkRequest();
        _itemsToSend.addAll(_indexRequestResistantTimer.getAndClearItems());

        if (_itemsToSend.size() == 0) {
            if (_closeRequested && !_hasBulkRequestThreadRunning) {
                _canClose = true;
            }
            return;
        }

        for (IndexRequest indexRequest : _itemsToSend) {
            newRequest.add(indexRequest);
        }

        _itemsToSend.clear();

        // If we think the instance isn't running, let's store this for later
        if (!_elasticsearchInstanceRunning) {
            // Don't store this. Close has been requested and there's no instance running. Just forget the data
            if (!_closeRequested) {
                storeBulkRequestForLater(newRequest);
            }
            else if (!_hasBulkRequestThreadRunning) {
                _canClose = true;
            }
        }
        else {
            sendBulkRequest(newRequest);
        }
    }

    /**
     * Sends the failed bulk requests to Elasticsearch
     */
    private void sendOldBulkRequest () {
        _client.bulkAsync(_oldBulkRequestsStored, new ActionListener<BulkResponse>()
        {
            @Override
            public void onResponse (BulkResponse bulkItemResponses) {
                if (bulkItemResponses.hasFailures()) {
                    _logger.error("Errors putting some data in");
                }
                else {
                    _sentOldBulkRequests = true;

                    if (_closeRequested) {
                        _canClose = true;
                    }
                }
            }

            @Override
            public void onFailure (Exception e) {
                _sentOldBulkRequests = false;
            }
        });
    }

    /**
     * Take the index request, and put it into the time resistance list
     */
    private void storeIndexRequest (IndexRequest indexRequest) {
        _indexRequestResistantTimer.addItem(indexRequest);
    }

    private synchronized void resetOldBulkRequest () {
        _oldBulkRequestsStored = new BulkRequest();
    }

    private synchronized void storeBulkRequestForLater (BulkRequest request) {
        _logger.info("Storing bulk request for later sending");
        _oldBulkRequestsStored.add(request.requests());
        _sentOldBulkRequests = false;

        if (!_hasBulkRequestThreadRunning) {
            runRepeatingRequestThread();
        }
    }
    private boolean _canClose;
    private RestHighLevelClient _client;
    private boolean _closeRequested;
    private boolean _elasticsearchInstanceRunning;
    private boolean _hasBulkRequestThreadRunning;
    private String _index;
    private ResistantTimer<IndexRequest> _indexRequestResistantTimer;
    private List<IndexRequest> _itemsToSend;
    private AtomicLong _maxItemCount = new AtomicLong(100);
    private BulkRequest _oldBulkRequestsStored;
    private boolean _sentOldBulkRequests;
    private String _type;
    private static long _mostRecentPrintTime = 0;
    private static final long FAILURE_PRINT_STRING_TIME = 10000;
    private static final Logger _logger = LoggerFactory.getLogger(Main.class);

}
