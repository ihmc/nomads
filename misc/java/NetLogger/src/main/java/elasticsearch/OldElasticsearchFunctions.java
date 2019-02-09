package elasticsearch;

import org.apache.http.HttpEntity;
import org.apache.http.entity.ContentType;
import org.apache.http.nio.entity.NStringEntity;
import org.elasticsearch.action.ActionListener;
import org.elasticsearch.action.admin.indices.close.CloseIndexRequest;
import org.elasticsearch.action.admin.indices.delete.DeleteIndexRequest;
import org.elasticsearch.action.bulk.BulkRequest;
import org.elasticsearch.action.bulk.BulkResponse;
import org.elasticsearch.client.Response;
import org.elasticsearch.client.RestHighLevelClient;
import org.elasticsearch.common.unit.TimeValue;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.Collections;

/**
 * Old functions previously used for elasticsearch. Not really used now, but helpful for guidance
 */
public class OldElasticsearchFunctions
{
    /**
     * Add data in the form of a bulk request
     *
     * @param request
     */
    private void addNewData (final String index, final BulkRequest request) {
        // Add the new request
        _client.bulkAsync(request, new ActionListener<BulkResponse>()
        {
            @Override
            public void onResponse (BulkResponse bulkItemResponses) {
                if (bulkItemResponses.hasFailures()) {
                    _logger.info("Error adding items! Attempting to change read_only_value");
                    setReadOnlyDeleteValue(index);
                }
            }

            @Override
            public void onFailure (Exception e) {
                e.printStackTrace();
            }
        });
    }

    /**
     * Delete an index from the database
     *
     * @param index
     */
    public void deleteIndex (final String index) {
        DeleteIndexRequest request = new DeleteIndexRequest(index);

        try {
            _client.indices().delete(request);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void sendDeleteBulkRequest (final String index, final BulkRequest bulkRequest) {
        // Add the new request
        _client.bulkAsync(bulkRequest, new ActionListener<BulkResponse>()
        {
            @Override
            public void onResponse (BulkResponse bulkItemResponses) {
                if (bulkItemResponses.hasFailures()) {
                    _logger.info("Error deleting items! Attempting to change read_only_value");
                    setReadOnlyDeleteValue(index);
                }
            }

            @Override
            public void onFailure (Exception e) {
                e.printStackTrace();
            }
        });
    }

    /**
     * Send a close request to this new index. This way data won't be able to be modified until an open request has been sent
     *
     * @param index
     */
    private void sendCloseRequestToIndex (final String index) {
        CloseIndexRequest request = new CloseIndexRequest(index);
        request.timeout(TimeValue.timeValueMinutes(2));

        try {
            _client.indices().close(request);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void setReadOnlyDeleteValue (String index) {
        String json =
                "{\n" +
                        "  \"index\": {\n" +
                        "    \"blocks\": {\n" +
                        "      \"read_only_allow_delete\": \"false\"\n" +
                        "    }\n" +
                        "  }\n" +
                        "}";
        HttpEntity entity = new NStringEntity(json, ContentType.APPLICATION_JSON);

        try {
            Response response = _client.getLowLevelClient().performRequest("PUT", index + "/_settings", Collections.emptyMap(), entity);
            int code = response.getStatusLine().getStatusCode();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    private RestHighLevelClient _client;

    private static final Logger _logger = LoggerFactory.getLogger(OldElasticsearchFunctions.class);
}
