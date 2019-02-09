package elasticsearch;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ThreadLocalRandom;

public class Main
{
    public static void main (String[] args) {

        ElasticsearchInsertManager newInsertUser = new ElasticsearchInsertManager();
        newInsertUser.init("128.49.235.115", 9200, "http");


        newInsertUser.setIndex("rttexperiment");
        newInsertUser.setIndexType("measure");

        new Thread(() -> {
            while (true) {
                Map<String, Object> values = new HashMap<>();

                float rtt = ThreadLocalRandom.current().nextFloat() * 230;

                values.put("sensor_ip", "128.49.235.153");
                values.put("src_ip", "128.49.235.153");
                values.put("dest_ip", "128.49.235.115");
                values.put("src_port", "0");
                values.put("dest_port", "0");
                values.put("subject", "rtt");
                values.put("natsTopic", "netsupervisor.aggregated.rtt");
                values.put("rtt_value", rtt);
                values.put("timestamp", new Date());

                newInsertUser.storeData(values);

                try {
                    Thread.sleep(4000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

        }, "Send thread").start();

    }

    private static final Logger _logger = LoggerFactory.getLogger(Main.class);
}
