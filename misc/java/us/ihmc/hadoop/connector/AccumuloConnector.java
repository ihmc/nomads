package us.ihmc.hadoop.connector;

import com.google.common.reflect.TypeToken;
import com.google.gson.Gson;
import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.QueueingConsumer;
import com.rabbitmq.client.ShutdownSignalException;
import org.apache.accumulo.core.client.*;
import org.apache.accumulo.core.client.Scanner;
import org.apache.accumulo.core.client.security.tokens.PasswordToken;
import org.apache.accumulo.core.data.Key;
import org.apache.accumulo.core.data.Mutation;
import org.apache.accumulo.core.data.Range;
import org.apache.accumulo.core.data.Value;
import org.apache.accumulo.core.security.Authorizations;
import org.apache.accumulo.core.security.ColumnVisibility;
import org.apache.hadoop.io.Text;
import us.ihmc.hadoop.config.Keys;
import us.ihmc.util.Config;

import java.io.IOException;
import java.lang.reflect.Type;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;

public class AccumuloConnector implements Runnable
{
    private String _user;
    private String _password;
    private final String _instanceName;
    private final String _zooServers;
    private Instance _instance;
    private final Authorizations _auths;
    private final Map<String, EntityData> _entities;
    private AtomicBoolean _done;

    //rabbitMQ and GSon
    protected String _exchangeName;
    protected Channel _channel;
    protected QueueingConsumer _consumer;
    protected BatchWriter _entityInWriter;
    protected BatchWriter _entityOutWriter;
    protected BatchWriter _timeWriter;
    private String _tableEntityIn;
    private String _tableEntityOut;
    private String _tableTimeOut;

    private final static long MEM_BUF = 1000000L; // bytes to store before sending a batch
    private final static long TIMEOUT = 1000L; // milliseconds to wait before sending
    private final static int NUM_THREADS = 10;
    private String _getRoutingKey;
    private String _getResponseRoutingKey;
    private String _writeEntityRoutingKey;
    private String _writeTimeRoutingKey;
    private final Gson _gson;
    private Connector _conn;

    public AccumuloConnector (String instanceName, String zooServers, String tableEntityIn, String tableEntityOut,
                              String tableTimeOut)
    {
        _instanceName = instanceName;
        _zooServers = zooServers;
        _tableEntityIn = tableEntityIn;
        _tableEntityOut = tableEntityOut;
        _tableTimeOut = tableTimeOut;
        _instance = new ZooKeeperInstance(instanceName, zooServers);
        _auths = new Authorizations();
        _entities = new HashMap<String, EntityData>();
        _gson = new Gson();
        _done = new AtomicBoolean(false);
    }

    @SuppressWarnings("deprecation")
    public void connect (String user, String password) throws AccumuloSecurityException, AccumuloException,
            TableNotFoundException
    {
        _user = user;
        _password = password;
        _conn = _instance.getConnector(user, new PasswordToken(password));
        _entityInWriter = _conn.createBatchWriter(_tableEntityIn, MEM_BUF, TIMEOUT, NUM_THREADS);
        _entityOutWriter = _conn.createBatchWriter(_tableEntityOut, MEM_BUF, TIMEOUT, NUM_THREADS);
        _timeWriter = _conn.createBatchWriter(_tableTimeOut, MEM_BUF, TIMEOUT, NUM_THREADS);
    }

    /**
     * @param done set when processing is done
     */
    public void setDone (boolean done)
    {
        this._done.set(done);
    }

    public void setupRabbitMQ (String host, String exchangeName, String getKey,
                               String getResponseKey, String writeEntityKey, String writeTimeKey) throws IOException
    {
        System.out.println("Setting up RabbitMQ");
        ConnectionFactory connFactory = new ConnectionFactory();
        connFactory.setHost(host);
        Connection conn = connFactory.newConnection();
        _channel = conn.createChannel();
        _exchangeName = exchangeName;
        _channel.exchangeDeclare(exchangeName, "direct");
        String queueName = _channel.queueDeclare().getQueue();

        _getRoutingKey = getKey;
        _getResponseRoutingKey = getResponseKey;
        _writeEntityRoutingKey = writeEntityKey;
        _writeTimeRoutingKey = writeTimeKey;
        // Bind that queue to the exchange Node will be writing to in RabbitMQ
        _channel.queueBind(queueName, exchangeName, _getRoutingKey);
        _channel.queueBind(queueName, exchangeName, _writeTimeRoutingKey);
        _channel.queueBind(queueName, exchangeName, _writeEntityRoutingKey);

        //create consumer for the queue
        _consumer = new QueueingConsumer(_channel);
        _channel.basicConsume(queueName, false, this._consumer);
        System.out.println("RabbitMQ setup successful!");
    }

    @SuppressWarnings("deprecation")
    @Override
    public void run ()
    {
        while (!_done.get()) {

            QueueingConsumer.Delivery delivery;
            try {
                delivery = _consumer.nextDelivery();
                String routingKey = delivery.getEnvelope().getRoutingKey();
                AMQP.BasicProperties props = delivery.getProperties();
                AMQP.BasicProperties replyProps = new AMQP.BasicProperties();
                replyProps.setCorrelationId(props.getCorrelationId());
                EntityData eData;
                System.out.println("Got the key: " + routingKey);

                try {
                    if (routingKey.equals(_writeEntityRoutingKey)) {
                        //Reflection
                        Type listType = new TypeToken<ArrayList<List<String>>>()
                        {
                        }.getType();
                        ArrayList<List<String>> entitiesTable = _gson.fromJson(new String(delivery.getBody()),
                                listType);
                        for (List<String> entityList : entitiesTable) {
                            EntityData entity = EntityData.fromStringList(entityList);
                            writeEntitiesOut(entity); //write updates
                            writeEntitiesIn(entity);  //delete originals
                        }
                    }
                    else if (routingKey.equals(_getRoutingKey)) {
                        scanTable(_tableEntityIn);
                    }
                    else if (routingKey.equals(_writeTimeRoutingKey)) {
                        String sessionTime = _gson.fromJson(new String(delivery.getBody()), String.class);
                        writeTime(sessionTime);
                    }
                    else {
                        System.out.println("Received unknown routing key:" + routingKey);
                    }

                }
                catch (TableNotFoundException e) {
                    System.out.println("Table not found");
                    e.printStackTrace();
                }
                catch (MutationsRejectedException e) {
                    System.out.println("Mutation rejected");
                    e.printStackTrace();
                }
                catch (AccumuloSecurityException e) {
                    System.out.println("Security exception found");
                    e.printStackTrace();
                }
                catch (AccumuloException e) {
                    System.out.println("Accumulo exception found");
                    e.printStackTrace();
                }
                catch (IndexOutOfBoundsException e) {
                    System.out.println("Received wrong entity in a String format");
                    e.printStackTrace();
                }
                catch (NumberFormatException e) {
                    System.out.println("Received unidentified format for confidence");
                    e.printStackTrace();
                }
            }
            catch (ShutdownSignalException e) {
                System.out.println("Caught ShutdownSignalException, stopping...");
                e.printStackTrace();
                _done.set(true);
            }
            catch (InterruptedException e) {
                System.out.println("Caught InterruptedException, stopping...");
                e.printStackTrace();
                _done.set(true);
            }
        }

        closeWriters();

        System.out.println("Exiting...");
    }

    protected void closeWriters ()
    {
        try {
            _entityInWriter.close();
            _entityOutWriter.close();
            _timeWriter.close();
        }
        catch (MutationsRejectedException e) {
            System.out.println("Unable to do clean close of the writer!");
            e.printStackTrace();
        }
    }

    private void writeEntitiesOut (EntityData eData) throws TableNotFoundException, AccumuloException,
            AccumuloSecurityException
    {
        Mutation mutation = eData.toUpdateMutation();
        _entityOutWriter.addMutation(mutation);
        System.out.println("Written mutation of entity " + eData.toString() + " to table " + _tableEntityOut);
    }

    private void writeEntitiesIn (EntityData eData) throws TableNotFoundException, AccumuloException,
            AccumuloSecurityException
    {
        Mutation mutation = eData.toDeleteMutation();
        _entityInWriter.addMutation(mutation);
        System.out.println("Written delete mutation of entity " + eData.toString() + " to table " + _tableEntityIn);
    }

    private void writeTime (String time) throws TableNotFoundException, AccumuloException,
            AccumuloSecurityException
    {
        Mutation mutation = new Mutation(new Text(UUID.randomUUID().toString()));
        mutation.put(new Text("sessionTime"), new Text(time), new ColumnVisibility(), System.currentTimeMillis(),
                new Value("".getBytes()));
        _timeWriter.addMutation(mutation);
        System.out.println("Written time " + time + " on table " + _tableTimeOut);
    }


    public List<List<String>> scanTable (String table) throws TableNotFoundException
    {
        System.out.println("=== Scanning table: " + table);
        Scanner scan = _conn.createScanner(table, _auths);
        IsolatedScanner iScan = new IsolatedScanner(scan);
        iScan.setRange(new Range());
        for (Map.Entry<Key, Value> entry : scan) {
            EntityData eData;
            String row = entry.getKey().getRow().toString();

            if (_entities.containsKey(row)) {
                if (_entities.get(row).isComplete())
                    continue;
                eData = _entities.get(row);
            }
            else {
                eData = new EntityData();
                eData.setId(row);
                _entities.put(row, eData);
            }

            String familyCol = entry.getKey().getColumnFamily().toString();
            EntityData.Family family;
            try {
                family = EntityData.Family.valueOf(familyCol);
            }
            catch (IllegalArgumentException e) {
                System.out.println("Unrecognized family column, ignoring");
                continue;
            }
            String qualifierCol = entry.getKey().getColumnQualifier().toString();
            switch (family) {
                case name:
                    eData.setName(qualifierCol);
                    break;
                case appears_in:
                    eData.setAppearsIn(qualifierCol);
                    break;
                case type:
                    Value v = entry.getValue();
                    if (v == null)
                        continue;
                    double confidence;
                    try {
                        confidence = Double.parseDouble(v.toString());

                    }
                    catch (NumberFormatException e) {
                        System.out.println("Unable to parse confidence value for type:" + qualifierCol);
                        continue;
                    }
                    eData.getTypes().add(new EntityType(qualifierCol, confidence));
                    break;
                case confidence:
                    break;
                case context:
                    eData.setContext(qualifierCol);
                    break;
            }
        }

        System.out.println("Found " + _entities.size() + " entities");
        if (_entities.isEmpty()) {
            System.out.println("No entities found, not publishing to AMQP");
            return null;
        }
        System.out.println("=== Scanning table: " + table + " finished ===");

        ArrayList<List<String>> entitiesTable = new ArrayList<List<String>>();
        for (EntityData e : _entities.values()) {
            entitiesTable.add(e.toStringList());
        }

        System.out.println("Pushing " + entitiesTable.size() + " entities");
        AMQP.BasicProperties publishProps = new AMQP.BasicProperties();

        try {
            _channel.basicPublish(_exchangeName, _getResponseRoutingKey, publishProps,
                    _gson.toJson(entitiesTable).getBytes());
        }
        catch (IOException e) {
            System.out.println("IO error, unable to publish on exchange: " + _exchangeName + " with key: " +
                    _getResponseRoutingKey);
            e.printStackTrace();
        }

        //clear HashMap
        _entities.clear();

        return entitiesTable;
    }

    public static void main (String[] args)
    {
        if (args == null || args.length == 0 || args[0] == null) {
            System.out.println("Configuration file not found. Needed hadoop-utils.properties as an argument");
            return;
        }
        Config.loadConfig(args[0]);

        final AccumuloConnector ac = new AccumuloConnector(Config.getStringValue(Keys.ACCUMULO_INSTANCE_NAME),
                Config.getStringValue(Keys.ZOOKEEPER_HOSTS),
                Config.getStringValue(Keys.ACCUMULO_TABLE_ENTITY_IN),
                Config.getStringValue(Keys.ACCUMULO_TABLE_ENTITY_OUT),
                Config.getStringValue(Keys.ACCUMULO_TABLE_TIME_OUT));
        try {
            ac.setupRabbitMQ(Config.getStringValue(Keys.RABBITMQ_HOST, "localhost"),
                    Config.getStringValue(Keys.RABBITMQ_EXCHANGE_NAME, "accumulo"),
                    Config.getStringValue(Keys.RABBITMQ_GET_ROUTING_KEY, "get-accumulo"),
                    Config.getStringValue(Keys.RABBITMQ_GET_RESPONSE_ROUTING_KEY, "get-response-accumulo"),
                    Config.getStringValue(Keys.RABBITMQ_WRITE_ENTITY_ROUTING_KEY, "to-accumulo-entity"),
                    Config.getStringValue(Keys.RABBITMQ_WRITE_TIME_ROUTING_KEY, "to-accumulo-time"));
            ac.connect(Config.getStringValue(Keys.ACCUMULO_USER_NAME),
                    Config.getStringValue(Keys.ACCUMULO_USER_PASSWORD));
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (AccumuloException e) {
            e.printStackTrace();
        }
        catch (TableNotFoundException e) {
            e.printStackTrace();
        }
        catch (AccumuloSecurityException e) {
            e.printStackTrace();
        }

        //start RabbitMQ processing Thread
        final Thread t = new Thread(ac);
        t.start();

        Runtime.getRuntime().addShutdownHook(new Thread()
        {
            public void run ()
            {
                try {
                    ac.setDone(true);
                    ac.closeWriters();
                    t.join(1000);
                }
                catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });

        while (t.isAlive()) {
            try {
                Thread.sleep(1000);
            }
            catch (InterruptedException e) {
                e.printStackTrace();
                return;
            }
        }
    }

    @SuppressWarnings("deprecation")
    public void writeSample () throws TableNotFoundException, MutationsRejectedException
    {
        Text rowID = new Text("example01");
        Text colFam = new Text("name");
        Text colQual = new Text("United Nations");
        ColumnVisibility colVis = new ColumnVisibility();
        long timestamp = System.currentTimeMillis();

        Value value = new Value("test".getBytes());

        Mutation mutation = new Mutation(rowID);
        mutation.put(colFam, colQual, colVis, timestamp, value);

        long memBuf = 1000000L; // bytes to store before sending a batch
        long timeout = 1000L; // milliseconds to wait before sending
        int numThreads = 10;

        BatchWriter writer = _conn.createBatchWriter(Config.getStringValue(Keys.ACCUMULO_TABLE_ENTITY_OUT), memBuf,
                timeout, numThreads);
        writer.addMutation(mutation);
        writer.close();
        System.out.println("Written mutation of size " + mutation.size() + " to table " + Config.getStringValue(Keys
                .ACCUMULO_TABLE_ENTITY_OUT));
    }
}
