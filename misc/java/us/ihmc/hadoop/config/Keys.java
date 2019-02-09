package us.ihmc.hadoop.config;

/**
 * Keys.java
 *
 * Class <code>Keys</code> contains hadoop-utils configuration keys.
 */
public class Keys
{
    //rabbitmq
    public final static String RABBITMQ_HOST = "rabbitmq.host";
    public final static String RABBITMQ_EXCHANGE_NAME = "rabbitmq.exchange.name";
    public final static String RABBITMQ_GET_ROUTING_KEY = "rabbitmq.get.routing.key";
    public final static String RABBITMQ_GET_RESPONSE_ROUTING_KEY = "rabbitmq.get.response.routing.key";
    public final static String RABBITMQ_WRITE_ENTITY_ROUTING_KEY = "rabbitmq.write.entity.routing.key";
    public final static String RABBITMQ_WRITE_TIME_ROUTING_KEY = "rabbitmq.write.time.routing.key";

    //accumulo
    public final static String ACCUMULO_INSTANCE_NAME = "accumulo.instance.name";
    public final static String ACCUMULO_USER_NAME = "accumulo.user.name";
    public final static String ACCUMULO_USER_PASSWORD = "accumulo.user.password";
    public final static String ACCUMULO_TABLE_ENTITY_IN = "accumulo.table.entity.in";
    public final static String ACCUMULO_TABLE_ENTITY_OUT = "accumulo.table.entity.out";
    public final static String ACCUMULO_TABLE_TIME_OUT = "accumulo.table.time.out";
    public final static String ACCUMULO_SCAN_INTERVAL = "accumulo.scan.interval";
    //zookeeper
    public final static String ZOOKEEPER_HOSTS = "zookeeper.hosts";
}
