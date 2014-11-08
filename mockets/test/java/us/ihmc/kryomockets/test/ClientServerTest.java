package us.ihmc.kryomockets.test;

import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import us.ihmc.kryomockets.Client;
import us.ihmc.kryomockets.Connection;
import us.ihmc.kryomockets.Listener;
import us.ihmc.kryomockets.Server;
import us.ihmc.kryomockets.util.Data;
import us.ihmc.kryomockets.util.DataType;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;
import java.util.UUID;


/**
 * ClientServerTest.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ClientServerTest
{

    private final static int DEFAULT_TIMEOUT = 5000;
    private final static Logger LOG = Logger.getLogger(ClientServerTest.class);

    public static void main (String[] args)
    {
        Properties log4jProp = getLog4jProperties("../conf/kryomockets.log4j.properties");
        PropertyConfigurator.configure(log4jProp);
        //Log.set(LEVEL_TRACE);

        LOG.info("Starting test");
        if (args == null || args[0] == null) {
            printUsage();
            return;
        }

        int mocketsPort = 0, msgNum;
        String ipAddress;
        Server server;
        Client client;

        try {
            mocketsPort = Integer.parseInt(args[2]);

            if (args[0].equals("-client")) {
                ipAddress = args[1];
                msgNum = Integer.parseInt(args[3]);
                client = new Client();

                //register Data object
                client.getKryo().register(Data.class);
                client.getKryo().register(byte[].class);
                client.getKryo().register(DataType.class);
                client.start();
                client.connect(DEFAULT_TIMEOUT, ipAddress, mocketsPort);

                client.addListener(new Listener()
                {
                    public void received (Connection connection, Object object)
                    {
                        if (object instanceof Data) {
                            Data response = (Data) object;
                            LOG.info("CLIENT - Received message: " + response.getText());
                        }
                    }
                });


                Data request = new Data(UUID.randomUUID().toString());
                for (int i = 0; i < msgNum; i++) {
                    request.setText("Here is the request: " + UUID.randomUUID());
                    client.sendMockets(request);
                }

                while (true) {
                    Thread.sleep(5);
                }

            }
            else if (args[0].equals("-server")) {
                server = new Server();

                //register Data object
                server.getKryo().register(Data.class);
                server.getKryo().register(byte[].class);
                server.getKryo().register(DataType.class);
                server.start();
                LOG.info("SERVER - Accepting Mockets connections on port " + mocketsPort);
                server.bind(mocketsPort);

                //add listener
                server.addListener(new Listener()
                {
                    public void received (Connection connection, Object object)
                    {
                        if (object instanceof Data) {
                            Data request = (Data) object;
                            LOG.info("SERVER - Received message: " + request.getText());

                            Data response = new Data(UUID.randomUUID().toString());
                            response.setText("I received your: " + request.getText());
                            connection.sendMockets(response);
                        }
                    }
                });

            }
            else {
                printUsage();
            }
        }
        catch (NumberFormatException e) {
            printUsage();
        }
        catch (NullPointerException e) {
            printUsage();
        }
        catch (IOException e) {
            LOG.error("Unable to use Mockets port:" + mocketsPort, e);
        }
        catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static Properties getLog4jProperties (String configFilePath)
    {
        Properties log4jProperties = new Properties();
        try {
            log4jProperties.load(new FileInputStream(configFilePath));
            String logFileName = log4jProperties.getProperty("log4j.appender.rollingFile.File");
            Date day = new Date();
            String formattedDate = new SimpleDateFormat("yyyyMMddhhmm").format(day);
            log4jProperties.setProperty("log4j.appender.rollingFile.File", String.format("../logs/%s-%s",
                    formattedDate, logFileName));
        }
        catch (FileNotFoundException e) {
            LOG.error("Unable to load log4j configuration, file not found.");
            e.printStackTrace();
        }
        catch (IOException e) {
            LOG.error("Unable to load log4j configuration, error while I/O on disk.");
            e.printStackTrace();
        }
        return log4jProperties;
    }

    public static void printUsage ()
    {
        LOG.warn("Usage: ClientServerTest -client | -server <ip> <tcp_port> <mockets_port>");
    }
}
