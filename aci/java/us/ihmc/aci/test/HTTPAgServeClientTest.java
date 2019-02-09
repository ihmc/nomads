package us.ihmc.aci.test;

import org.apache.log4j.PropertyConfigurator;
import us.ihmc.aci.newKernel.Utils;
import us.ihmc.aci.newKernel.logger.LoggerWrapper;
import us.ihmc.aci.newagserve.HTTPAgserveAdminClient;

import java.io.File;
import java.util.List;
import java.util.Properties;

/**
 * Test class for HTTPAgserveClient
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class HTTPAgServeClientTest
{
    public HTTPAgServeClientTest (String ip, String nodeUUID, int n)
    {
        if ((ip == null) || (nodeUUID == null)) {
            logger.error ("Either the ip or the node UUID have not been set");
            return;
        }

        try {
            HTTPAgserveAdminClient client = new HTTPAgserveAdminClient(ip);
            logger.info ("Client created");
            String serviceName = "us.ihmc.primenumbers.PrimeNumbersService";
            String acrFile = "data/PrimeNumbers/service/PrimeNumbersService.acr";
            client.deployServiceRemotely (nodeUUID, new File (acrFile).toURI().toURL()); // The deployService does also the registration

            String serviceInstanceUUID = client.activateServiceRemotely (nodeUUID, serviceName);
            logger.info ("[ACIAgServeClientTest] serviceInstanceUUID: " + serviceInstanceUUID);
            String instanceUUID = serviceInstanceUUID.replace ("acil://", "").replace ("|" + nodeUUID + "|", "");
            logger.info ("[ACIAgServeClientTest] instanceUUID: " + instanceUUID);

            List<Integer> results = (List<Integer>) client.invokeService (instanceUUID, "getPrimeNumbers", new Object[0], false);
            StringBuilder sb = new StringBuilder();
            sb.append ("[getPrimeNumbers] PRIME NUMBERS BEFORE 100: ");
            for (Integer i : results) {
                sb.append (i);
                sb.append (" ");
            }
            logger.info (sb.toString());

            Object[] params = new Object[1];
            params[0] = n;
            results = (List<Integer>) client.invokeService (instanceUUID, "getPrimeNumbersUpTo", params, false);
            sb = new StringBuilder();
            sb.append ("[getPrimeNumbersUpTo(" + params[0] + ") PRIME NUMBERS BEFORE " + params[0] + ": ");
            for (Integer i : results) {
                sb.append(i);
                sb.append (" ");
            }
            logger.info (sb.toString());
        }
        catch (Exception e) {
            logger.error ("Problem in running the test", e);
            System.exit (-1);
        }
    }

    public static void main (String[] args)
    {
        String ip = null;
        String node = null;
        int n = 100;

        int i=0;
        while (true) {
            if (args.length <= i) {
                break;
            }

            if (args[i].equals ("-ip")) {
                i++;
                if (args.length <= i) {
                    break;
                }
                ip = args[i];
            }
            else if (args[i].equals ("-node")) {
                i++;
                if (args.length <= i) {
                    break;
                }
                node = args[i];
            }
            else if (args[i].equals ("-n")) {
                i++;
                if (args.length <= i) {
                    break;
                }
                try {
                    n = Integer.parseInt (args[i]);
                }
                catch (NumberFormatException e) {
                    System.err.println ("The value assigned to n is not correct. Using default value.");
                }
            }
            else {
                System.err.println ("Wrong parameter " + args[i] + ". Ignoring it");
            }

            i++;
        }

        if ((ip == null) || (node == null)) {
            System.err.println ("Either the ip or the node UUID have not been set");
            return;
        }

        if (!(new File ("../../logs").exists())) {
            (new File ("../../logs")).mkdir();
        }
        Properties log4jProperties = Utils.getLog4jProperties ("../../conf/log4j.properties", "../../logs/");
        PropertyConfigurator.configure (log4jProperties);

        new HTTPAgServeClientTest (ip, node, n);
    }

    final LoggerWrapper logger = LoggerWrapper.getLogger (HTTPAgServeClientTest.class, false);
}
