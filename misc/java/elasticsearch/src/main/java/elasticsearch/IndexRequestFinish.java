package elasticsearch;

import org.elasticsearch.action.index.IndexRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class IndexRequestFinish
{
    public IndexRequestFinish()
    {
        _transactionComplete = false;
    }

    public void offer(IndexRequest indexRequest)
    {
        _indexRequest = indexRequest;
    }

    public IndexRequest getIndexRequest () {
        return _indexRequest;
    }

    public synchronized void setTransactionComplete (boolean transactionComplete) {
        _transactionComplete = transactionComplete;
    }

    public synchronized boolean isTransactionComplete () {
        return _transactionComplete;
    }

    private IndexRequest _indexRequest;
    private boolean _transactionComplete;
    private static final Logger _logger = LoggerFactory.getLogger(IndexRequestFinish.class);
}
